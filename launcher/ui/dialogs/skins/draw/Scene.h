#pragma once

#include "ui/dialogs/skins/draw/BoxGeometry.h"

#include <QOpenGLTexture>
namespace opengl {
class Scene {
   public:
    Scene(const QImage& skin, bool slim, const QImage& cape);
    virtual ~Scene();

    void draw(QOpenGLShaderProgram* program);
    void setSkin(const QImage& skin);
    void setCape(const QImage& cape);
    void setMode(bool slim);
    void setCapeVisible(bool visible);

   private:
    QVector<BoxGeometry*> m_staticComponents;
    QVector<BoxGeometry*> m_normalArms;
    QVector<BoxGeometry*> m_slimArms;
    BoxGeometry* m_cape = nullptr;
    QOpenGLTexture* m_skinTexture = nullptr;
    QOpenGLTexture* m_capeTexture = nullptr;
    bool m_slim = false;
    bool m_capeVisible = false;
};
}  // namespace opengl