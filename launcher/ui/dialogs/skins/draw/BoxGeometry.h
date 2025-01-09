#pragma once

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QVector3D>

namespace opengl {
class BoxGeometry : protected QOpenGLFunctions {
   public:
    BoxGeometry(QVector3D size, QVector3D position);
    BoxGeometry(QVector3D size, QVector3D position, QPoint uv, QVector3D textureDim, QSize textureSize = { 64, 64 });
    virtual ~BoxGeometry();

    void draw(QOpenGLShaderProgram* program);

    void initGeometry(float u, float v, float width, float height, float depth, float textureWidth = 64, float textureHeight = 64);
    void rotate(float angle, const QVector3D& vector);

   private:
    QOpenGLBuffer m_vertexBuf;
    QOpenGLBuffer m_indexBuf;
    QVector3D m_size;
    QVector3D m_position;
    QMatrix4x4 m_matrix;
};
}  // namespace opengl
