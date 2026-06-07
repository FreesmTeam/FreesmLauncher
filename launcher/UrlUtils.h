#pragma once

#include <QString>
#include <QUrl>

namespace UrlUtils {
bool isLocalhost(const QUrl& url);
bool isUnsafe(const QUrl& url);
void upgradeToHTTPS(QUrl& url);
QUrl httpFromUserInput(QString userInput, QString* errorString);
}  // namespace UrlUtils
