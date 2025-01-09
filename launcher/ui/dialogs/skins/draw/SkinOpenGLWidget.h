#pragma once

#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QVector2D>
#include "minecraft/skins/SkinModel.h"
#include "ui/dialogs/skins/draw/Scene.h"

class SkinOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

   public:
    SkinOpenGLWidget(QWidget* parent = nullptr) : QOpenGLWidget(parent), QOpenGLFunctions() {}
    virtual ~SkinOpenGLWidget();

    void updateScene(SkinModel* skin);
    void updateCape(const QImage& cape);

   protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* event) override;

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void initShaders();

    GLuint generateChessboardTexture(int width, int height, int tileSize);
    void renderBackground();

   private:
    QOpenGLShaderProgram m_program;
    opengl::Scene* m_scene = nullptr;

    QMatrix4x4 m_projection;

    QVector2D m_mousePosition;

    bool m_isMousePressed = false;
    int m_rotationX = 0, m_rotationY = 0;

    // background
    GLuint m_chessboardTexture;
};
