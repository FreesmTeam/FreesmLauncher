#include "ui/dialogs/skins/draw/SkinOpenGLWidget.h"

#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QVector2D>
#include <QVector3D>
#include "minecraft/skins/SkinModel.h"
#include "ui/dialogs/skins/SkinManageDialog.h"
#include "ui/dialogs/skins/draw/Scene.h"

SkinOpenGLWidget::~SkinOpenGLWidget()
{
    // Make sure the context is current when deleting the texture
    // and the buffers.
    makeCurrent();
    delete m_scene;
    glDeleteTextures(1, &m_chessboardTexture);
    doneCurrent();
}

void SkinOpenGLWidget::mousePressEvent(QMouseEvent* e)
{
    // Save mouse press position
    m_mousePosition = QVector2D(e->position());
    m_isMousePressed = true;
}
void SkinOpenGLWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isMousePressed) {
        int dx = event->x() - m_mousePosition.x();
        int dy = event->y() - m_mousePosition.y();

        // Update rotation angles based on mouse movement
        m_rotationX += dy;
        m_rotationY += dx;

        m_mousePosition = QVector2D(event->position());
        update();  // Trigger a repaint
    }
}
void SkinOpenGLWidget::mouseReleaseEvent(QMouseEvent* e)
{
    m_isMousePressed = false;
}

void SkinOpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0, 0, 1, 1);

    initShaders();

    m_chessboardTexture = generateChessboardTexture(512, 512, 16);

    QImage skin, cape;
    bool slim = false;
    if (auto p = dynamic_cast<SkinManageDialog*>(parent()); p) {
        if (auto s = p->getSelectedSkin()) {
            skin = s->getTexture();
            slim = s->getModel() == SkinModel::SLIM;
            cape = p->capes().value(s->getCapeId(), {});
        }
    }

    m_scene = new opengl::Scene(skin, slim, cape);
    glEnable(GL_TEXTURE_2D);
}

void SkinOpenGLWidget::initShaders()
{
    // Compile vertex shader
    if (!m_program.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, R"(// Copyright (C) 2024 The Qt Company Ltd.
 // SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
 #ifdef GL_ES
 // Set default precision to medium
 precision mediump int;
 precision mediump float;
 #endif

 uniform mat4 mvp_matrix;
 uniform mat4 model_matrix;

 attribute vec4 a_position;
 attribute vec2 a_texcoord;

 varying vec2 v_texcoord;

 void main()
 {
    // Calculate vertex position in screen space
    gl_Position = mvp_matrix * model_matrix * a_position;

    // Pass texture coordinate to fragment shader
    // Value will be automatically interpolated to fragments inside polygon faces
    v_texcoord = a_texcoord;
 }
 )"))
        close();

    // Compile fragment shader
    if (!m_program.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, R"(// Copyright (C) 2024 The Qt Company Ltd.
 // SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
 #ifdef GL_ES
 // Set default precision to medium
 precision mediump int;
 precision mediump float;
 #endif

 uniform sampler2D texture;

 varying vec2 v_texcoord;

 void main()
 {
    // Set fragment color from texture
    //  gl_FragColor = texture2D(texture, v_texcoord);
    vec4 texColor = texture2D(texture, v_texcoord);
    if (texColor.a < 0.1) discard; // Optional: Discard fully transparent pixels
    gl_FragColor = texColor;
 }
 )"))
        close();

    // Link shader pipeline
    if (!m_program.link())
        close();

    // Bind shader pipeline for use
    if (!m_program.bind())
        close();
}

void SkinOpenGLWidget::resizeGL(int w, int h)
{
    // Calculate aspect ratio
    qreal aspect = qreal(w) / qreal(h ? h : 1);

    const qreal zNear = .1, zFar = 1000., fov = 45;

    // Reset projection
    m_projection.setToIdentity();

    // Set perspective projection
    m_projection.perspective(fov, aspect, zNear, zFar);
}

void SkinOpenGLWidget::paintGL()
{
    // Clear color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable back face culling
    glEnable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderBackground();

    m_program.bind();

    // Calculate model view transformation
    QMatrix4x4 matrix;
    matrix.translate(0.0, 6.0, -50.);
    matrix.rotate(m_rotationX, 1.0f, 0.0f, 0.0f);
    matrix.rotate(m_rotationY, 0.0f, 1.0f, 0.0f);

    // Set modelview-projection matrix
    m_program.setUniformValue("mvp_matrix", m_projection * matrix);

    m_scene->draw(&m_program);
    m_program.release();
}

void SkinOpenGLWidget::updateScene(SkinModel* skin)
{
    if (skin && m_scene) {
        m_scene->setMode(skin->getModel() == SkinModel::SLIM);
        m_scene->setSkin(skin->getTexture());
        update();
    }
}
void SkinOpenGLWidget::updateCape(const QImage& cape)
{
    if (m_scene) {
        m_scene->setCapeVisible(!cape.isNull());
        m_scene->setCape(cape);
        update();
    }
}

GLuint SkinOpenGLWidget::generateChessboardTexture(int width, int height, int tileSize)
{
    std::vector<unsigned char> textureData(width * height * 3);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            bool isWhite = ((x / tileSize) % 2) == ((y / tileSize) % 2);
            unsigned char color = isWhite ? 100 : 50;

            int index = (y * width + x) * 3;
            textureData[index] = color;      // Red
            textureData[index + 1] = color;  // Green
            textureData[index + 2] = color;  // Blue
        }
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}

void SkinOpenGLWidget::renderBackground()
{
    glDepthMask(GL_FALSE);  // Disable depth buffer writing
    glBindTexture(GL_TEXTURE_2D, m_chessboardTexture);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -0.5f);  // Bottom-left
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -0.5f);  // Bottom-right
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, -0.5f);  // Top-right
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, 1.0f, -0.5f);  // Top-left
    glEnd();
    glDepthMask(GL_TRUE);  // Re-enable depth buffer writing
}
