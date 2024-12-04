// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2024 Kenneth Chew <79120643+kthchew@users.noreply.github.com>
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

#include "SecurityBookmarkFileAccess.h"

#include <Foundation/Foundation.h>
#include <QByteArray>
#include <QUrl>

QByteArray SecurityBookmarkFileAccess::urlToSecurityScopedBookmark(const QUrl& url)
{
    if (!url.isLocalFile())
        return {};

    NSError* error = nil;
    NSURL* nsurl = [url.toNSURL() absoluteURL];
    if ([m_paths objectForKey:[nsurl path]]) {
        return QByteArray::fromNSData(m_paths[[nsurl path]]);
    }
    [m_activeURLs addObject:nsurl];
    NSData* bookmark = [nsurl bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
                       includingResourceValuesForKeys:nil
                                        relativeToURL:nil
                                                error:&error];
    if (error) {
        return {};
    }
    m_paths[[nsurl path]] = bookmark;
    m_bookmarks[bookmark] = nsurl;
    [m_activeURLs addObject:nsurl];

    return QByteArray::fromNSData(bookmark);
}

SecurityBookmarkFileAccess::SecurityBookmarkFileAccess()
{
    m_bookmarks = [NSMutableDictionary new];
    m_paths = [NSMutableDictionary new];
    m_activeURLs = [NSMutableSet new];
}

SecurityBookmarkFileAccess::~SecurityBookmarkFileAccess()
{
    for (NSURL* url : m_activeURLs) {
        [url stopAccessingSecurityScopedResource];
    }
    [m_bookmarks release];
    [m_paths release];
    [m_activeURLs release];
}

QByteArray SecurityBookmarkFileAccess::pathToSecurityScopedBookmark(const QString& path)
{
    return urlToSecurityScopedBookmark(QUrl::fromLocalFile(path));
}

NSURL* SecurityBookmarkFileAccess::securityScopedBookmarkToNSURL(QByteArray& bookmark, bool& isStale)
{
    NSError* error = nil;
    BOOL localStale = NO;
    NSURL* nsurl = [NSURL URLByResolvingBookmarkData:bookmark.toNSData()
                                             options:NSURLBookmarkResolutionWithSecurityScope
                                       relativeToURL:nil
                                 bookmarkDataIsStale:&localStale
                                               error:&error];
    if (error) {
        return nil;
    }
    isStale = localStale;
    if (isStale) {
        NSData* nsBookmark = [nsurl bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
                             includingResourceValuesForKeys:nil
                                              relativeToURL:nil
                                                      error:&error];
        if (error) {
            return nil;
        }
        m_paths[[nsurl path]] = nsBookmark;
        m_bookmarks[nsBookmark] = nsurl;
        bookmark = QByteArray::fromNSData(nsBookmark);
    }

    return nsurl;
}

QUrl SecurityBookmarkFileAccess::securityScopedBookmarkToURL(QByteArray& bookmark, bool& isStale)
{
    if (bookmark.isEmpty())
        return {};

    NSURL* url = securityScopedBookmarkToNSURL(bookmark, isStale);
    if (!url)
        return {};

    return QUrl::fromNSURL(url);
}

bool SecurityBookmarkFileAccess::startUsingSecurityScopedBookmark(QByteArray& bookmark, bool& isStale)
{
    NSURL* url = [m_bookmarks objectForKey:bookmark.toNSData()] ? m_bookmarks[bookmark.toNSData()] : securityScopedBookmarkToNSURL(bookmark, isStale);
    if ([m_activeURLs containsObject:url])
        return false;

    if ([url startAccessingSecurityScopedResource]) {
        [m_activeURLs addObject:url];
        return true;
    }
    return false;
}

void SecurityBookmarkFileAccess::stopUsingSecurityScopedBookmark(QByteArray& bookmark)
{
    if (![m_bookmarks objectForKey:bookmark.toNSData()])
        return;
    NSURL* url = m_bookmarks[bookmark.toNSData()];

    if ([m_activeURLs containsObject:url]) {
        [url stopAccessingSecurityScopedResource];
        [m_activeURLs removeObject:url];
        [m_bookmarks removeObjectForKey:bookmark.toNSData()];
        [m_paths removeObjectForKey:[url path]];
    }
}
