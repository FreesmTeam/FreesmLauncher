#include <QClipboard>
#include <QFileSystemWatcher>
#include <QImage>
#include <QThread>

#include "Application.h"
#include "MinecraftInstance.h"

#include "ScreenshotsWatcher.h"

ScreenshotsWatcher::ScreenshotsWatcher(const QString& path) : QObject(), m_path(path)
{
    auto watcher = new QFileSystemWatcher({ path });

    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &ScreenshotsWatcher::dirUpdated);

    m_watcher = std::make_unique<QFileSystemWatcher>(watcher);
}

void ScreenshotsWatcher::dirUpdated()
{
    QDir d(m_path, "*.png", QDir::Time, QDir::Files);
    QString path = m_path + '/' + d[0];

    auto clipboard = Application::clipboard();
    if (clipboard == nullptr) {
        qDebug() << "Clipboard ptr is null, aborting copying";
        return;
    }

    QImage img;
    bool loaded = img.load(path);
    for (int i = 0; i < 5 && !loaded; i++) {
        QThread::msleep(200);
        loaded = img.load(path);
    }
    if (!loaded) {
        qDebug() << "Image wasn't loaded in 1 sec, aborting copying";
        return;
    }

    clipboard->setImage(img);
}
