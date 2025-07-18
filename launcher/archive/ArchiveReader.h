// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 Trial97 <alexandru.tripon97@gmail.com>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */
#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QStringList>
#include <memory>

struct archive;
struct archive_entry;
namespace MMCZip {
class ArchiveReader {
   public:
    using ArchivePtr = std::unique_ptr<struct archive, int (*)(struct archive*)>;
    ArchiveReader(QString fileName) : m_archivePath(fileName) {}
    virtual ~ArchiveReader() = default;

    QStringList getFiles();
    QString getZipName();
    bool collectFiles(bool onlyFiles = true);
    bool exists(const QString& filePath) const;

    class File {
       public:
        File();
        virtual ~File() = default;

        QString filename();
        bool isFile();
        QDateTime dateTime();
        const char* error();

        QByteArray readAll(int* outStatus = nullptr);
        bool skip();
        bool writeFile(archive* out, QString targetFileName = "", bool notBlock = false);

       private:
        int readNextHeader();

       private:
        friend ArchiveReader;
        ArchivePtr m_archive;
        archive_entry* m_entry;
    };

    std::unique_ptr<File> goToFile(QString filename);
    bool parse(std::function<bool(File*)>);
    bool parse(std::function<bool(File*, bool&)>);

   private:
    QString m_archivePath;

    QStringList m_fileNames = {};
};
}  // namespace MMCZip