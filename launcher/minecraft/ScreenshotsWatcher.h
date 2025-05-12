#pragma once

#include <QObject>
#include <memory>

class QFileSystemWatcher;
class QString;
class QObject;
class MinecraftInstance;

class ScreenshotsWatcher : public QObject {
    Q_OBJECT
   public:
    explicit ScreenshotsWatcher(const QString& path);

   private slots:
    void dirUpdated();

   private:
    std::unique_ptr<QFileSystemWatcher> m_watcher;
    const QString m_path;
};
