// SPDX-FileCopyrightText: 2025 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include <QTest>

#include <QList>
#include <QObject>
#include <QString>

#include <optional>

#include <FileSystem.h>
#include <MessageLevel.h>
#include <logs/LogParser.h>

class XmlLogParseTest : public QObject {
    Q_OBJECT

   private slots:

    void parseXml_data()
    {
        QString source = QFINDTESTDATA("testdata/TestLogs");

        QString shortXml = QString::fromUtf8(FS::read(FS::PathCombine(source, "vanilla-1.21.5.xml.log")));
        QString shortText = QString::fromUtf8(FS::read(FS::PathCombine(source, "vanilla-1.21.5.text.log")));

        QString longXml = QString::fromUtf8(FS::read(FS::PathCombine(source, "TerraFirmaGreg-Modern-forge.xml.log")));
        QString longText = QString::fromUtf8(FS::read(FS::PathCombine(source, "TerraFirmaGreg-Modern-forge.text.log")));
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("xml");
        QTest::newRow("short-vanilla") << shortText << shortXml;
        QTest::newRow("long-forge") << longText << longXml;
    }

    void parseXml() { QFETCH(QString, ) }

   private:
    LogParser m_parser;

    QList<std::pair<MessageLevel::Enum, QString>> parseLines(const QStringList& lines)
    {
        QList<std::pair<MessageLevel::Enum, QString>> out;
        for (const auto& line : lines)
            m_parser.appendLine(line);
        auto items = m_parser.parseAvailable();
        for (const auto& item : items) {
            if (std::holds_alternative<LogParser::LogEntry>(item)) {
                auto entry = std::get<LogParser::LogEntry>(item);
                auto msg = QString("[%1] [%2/%3] [%4]: %5")
                               .arg(entry.timestamp.toString("HH:mm:ss"))
                               .arg(entry.thread)
                               .arg(entry.levelText)
                               .arg(entry.logger)
                               .arg(entry.message);
                msg = censorPrivateInfo(msg);
                out.append(std::make_pair(entry.level, msg));
            } else if (std::holds_alternative<LogParser::PlainText>(item)) {
                auto msg = std::get<LogParser::PlainText>(item).message;
                level = m_instance->guessLevel(msg, level);
                msg = censorPrivateInfo(msg);
                out.append(std::make_pair(entry.level, msg));
            }
        }
        return out;
    }
};
