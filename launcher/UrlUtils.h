#pragma once

#include <QUrl>

namespace UrlUtils {
bool isLocalhost(const QUrl& url);
bool isUnsafe(const QUrl& url);
}
