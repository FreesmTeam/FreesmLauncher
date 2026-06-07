#include <QObject>

#include "UrlUtils.h"

bool UrlUtils::isLocalhost(const QUrl& url)
{
    return url.host() == "localhost" || url.host() == "127.0.0.1" || url.host() == "::1";
}

bool UrlUtils::isUnsafe(const QUrl& url)
{
    return !url.isEmpty() && url.scheme() == "http" && !isLocalhost(url);
}

void UrlUtils::upgradeToHTTPS(QUrl& url)
{
    if (isUnsafe(url)) {
        url.setScheme("https");
    }
}

QUrl UrlUtils::httpFromUserInput(QString userInput, QString* errorString)
{
    if (errorString) {
        errorString->clear();
    }

    userInput = userInput.trimmed();

    bool httpScheme = userInput.startsWith("http://", Qt::CaseInsensitive);
    bool httpsScheme = userInput.startsWith("https://", Qt::CaseInsensitive);

    if (userInput.contains("://") && !httpsScheme && !httpScheme) {
        if (errorString) {
            *errorString = QObject::tr("Invalid URL scheme");
        }
        return {};
    }

    QUrl deducedUrl = QUrl::fromUserInput(userInput);

    if (!deducedUrl.isValid() || deducedUrl.isLocalFile() || deducedUrl.host().isEmpty()) {
        if (errorString) {
            *errorString = QObject::tr("Invalid URL");
        }
        return {};
    }

    if (!httpsScheme && !httpScheme) {
        deducedUrl.setScheme("https");
    }

    return deducedUrl;
}