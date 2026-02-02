// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Freesm Launcher - Minecraft Launcher
 *  Copyright (C) 2026 so5iso4ka <so5iso4ka@icloud.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <memory>

#include "ByteArraySink.h"

#include "Head.h"


Net::Head::Ptr Net::Head::makeHeaderPairs(QUrl url, Options options)
{
    auto dl = makeShared<Head>();
    dl->m_url = url;
    dl->setObjectName(QString("HEAD:") + url.toString());
    dl->m_options = options;
    dl->m_sink = std::make_unique<ByteArraySink>(std::make_shared<QByteArray>());
    return dl;
}

QNetworkReply* Net::Head::getReply(QNetworkRequest& request)
{
    return m_network->head(request);
}
