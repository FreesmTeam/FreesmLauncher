#include "UrlUtils.h"

bool UrlUtils::isLocalhost(const QUrl& url)
{
    return url.host() == "localhost" || url.host() == "127.0.0.1" || url.host() == "::1";
}

bool UrlUtils::isUnsafe(const QUrl& url)
{
    return !url.isEmpty() && url.scheme() == "http" && !isLocalhost(url);
}
