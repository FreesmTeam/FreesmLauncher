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
 *
 */

#include "LogParser.h"

#include <QRegularExpression>
#include "MessageLevel.h"

using namespace Qt::Literals::StringLiterals;

void LogParser::appendLine(QAnyStringView data)
{
    if (!m_partialData.isEmpty()) {
        m_buffer = QString(m_partialData);
        m_buffer.append("\n");
        m_buffer.append(data.toString());
        m_partialData.clear();
    } else {
        m_buffer.append(data.toString());
    }
}

std::optional<LogParser::Error> LogParser::getError()
{
    return m_error;
}

MessageLevel::Enum LogParser::parseLogLevel(const QString& level)
{
    auto test = level.trimmed().toUpper();
    if (test == "TRACE") {
        return MessageLevel::Trace;
    } else if (test == "DEBUG") {
        return MessageLevel::Debug;
    } else if (test == "INFO") {
        return MessageLevel::Info;
    } else if (test == "WARN") {
        return MessageLevel::Warning;
    } else if (test == "ERROR") {
        return MessageLevel::Error;
    } else if (test == "FATAL") {
        return MessageLevel::Fatal;
    } else {
        return MessageLevel::Unknown;
    }
}

std::optional<LogParser::LogEntry> LogParser::parseAttributes()
{
    LogParser::LogEntry entry{
        "",
        MessageLevel::Info,
    };
    auto attributes = m_parser.attributes();

    for (const auto& attr : attributes) {
        auto name = attr.name();
        auto value = attr.value();
        if (name == "logger"_L1) {
            entry.logger = value.trimmed().toString();
        } else if (name == "timestamp"_L1) {
            if (value.trimmed().isEmpty()) {
                m_parser.raiseError("log4j:Event Missing required attribute: timestamp");
                return {};
            }
            entry.timestamp = QDateTime::fromSecsSinceEpoch(value.trimmed().toLongLong());
        } else if (name == "level"_L1) {
            entry.levelText = value.trimmed().toString();
            entry.level = parseLogLevel(entry.levelText);
        } else if (name == "thread"_L1) {
            entry.thread = value.trimmed().toString();
        }
    }
    if (entry.logger.isEmpty()) {
        m_parser.raiseError("log4j:Event Missing required attribute: logger");
        return {};
    }

    return entry;
}

void LogParser::setError()
{
    m_error = {
        m_parser.errorString(),
        m_parser.error(),
    };
}

void LogParser::clearError()
{
    m_error = {};  // clear previous error
}

bool isPotentialLog4JStart(QStringView buffer)
{
    static QString target = QStringLiteral("<log4j:event");
    if (buffer.isEmpty() || buffer[0] != '<') {
        return false;
    }
    auto bufLower = buffer.toString().toLower();
    return target.startsWith(bufLower) || bufLower.startsWith(target);
}

std::optional<LogParser::ParsedItem> LogParser::parseNext()
{
    clearError();

    if (m_buffer.isEmpty()) {
        return {};
    }

    if (m_buffer.trimmed().isEmpty()) {
        m_buffer.clear();
        return {};
    }

    // check if we have a full xml log4j event
    bool isCompleteLog4j = false;
    m_parser.clear();
    m_parser.setNamespaceProcessing(false);
    m_parser.addData(m_buffer);
    if (m_parser.readNextStartElement()) {
        if (m_parser.qualifiedName() == "log4j:Event"_L1) {
            int depth = 1;
            bool eod = false;
            while (depth > 0 && !eod) {
                auto tok = m_parser.readNext();
                switch (tok) {
                    case QXmlStreamReader::TokenType::StartElement: {
                        depth += 1;
                    } break;
                    case QXmlStreamReader::TokenType::EndElement: {
                        depth -= 1;
                    } break;
                    case QXmlStreamReader::TokenType::EndDocument: {
                        eod = true;  // break outer while loop
                    } break;
                    default: {
                        // no op
                    }
                }
                if (m_parser.hasError()) {
                    break;
                }
            }

            isCompleteLog4j = depth == 0;
        }
    }

    if (isCompleteLog4j) {
        return parseLog4J();
    } else {
        if (isPotentialLog4JStart(m_buffer)) {
            m_partialData = QString(m_buffer);
            return LogParser::Partial{ QString(m_buffer) };
        }

        int start = 0;
        auto bufView = QStringView(m_buffer);
        while (start < bufView.length()) {
            if (qsizetype pos = bufView.right(bufView.length() - start).indexOf('<'); pos != -1) {
                auto slicestart = start + pos;
                auto slice = bufView.right(bufView.length() - slicestart);
                if (isPotentialLog4JStart(slice)) {
                    if (slicestart > 0) {
                        auto text = m_buffer.left(slicestart);
                        m_buffer = m_buffer.right(m_buffer.length() - slicestart);
                        if (!text.trimmed().isEmpty()) {
                            return LogParser::PlainText{ text };
                        }
                    }
                    m_partialData = QString(m_buffer);
                    return LogParser::Partial{ QString(m_buffer) };
                }
                start = slicestart + 1;
            } else {
                break;
            }
        }

        // no log4j found, all plain text
        auto text = QString(m_buffer);
        m_buffer.clear();
        if (text.trimmed().isEmpty()) {
            return {};
        } else {
            return LogParser::PlainText{ text };
        }
    }
}

QList<LogParser::ParsedItem> LogParser::parseAvailable()
{
    QList<LogParser::ParsedItem> items;
    bool doNext = true;
    while (doNext) {
        auto item_ = parseNext();
        if (m_error.has_value()) {
            return {};
        }
        if (item_.has_value()) {
            auto item = item_.value();
            if (std::holds_alternative<LogParser::Partial>(item)) {
                break;
            } else {
                items.push_back(item);
            }
        } else {
            doNext = false;
        }
    }
    return items;
}

std::optional<LogParser::ParsedItem> LogParser::parseLog4J()
{
    m_parser.clear();
    m_parser.setNamespaceProcessing(false);
    m_parser.addData(m_buffer);

    m_parser.readNextStartElement();
    if (m_parser.qualifiedName() == "log4j:Event"_L1) {
        auto entry_ = parseAttributes();
        if (!entry_.has_value()) {
            setError();
            return {};
        }
        auto entry = entry_.value();

        bool foundMessage = false;
        int depth = 1;

        while (!m_parser.atEnd()) {
            auto tok = m_parser.readNext();
            switch (tok) {
                case QXmlStreamReader::TokenType::StartElement: {
                    depth += 1;
                    if (m_parser.qualifiedName() == "log4j:Message"_L1) {
                        QString message;
                        bool messageComplete = false;

                        while (!messageComplete) {
                            auto tok = m_parser.readNext();

                            switch (tok) {
                                case QXmlStreamReader::TokenType::Characters: {
                                    message.append(m_parser.text());
                                } break;
                                case QXmlStreamReader::TokenType::EndElement: {
                                    if (m_parser.qualifiedName() == "log4j:Message"_L1) {
                                        messageComplete = true;
                                    }
                                } break;
                                case QXmlStreamReader::TokenType::EndDocument: {
                                    return {};  // parse fail
                                } break;
                                default: {
                                    // no op
                                }
                            }

                            if (m_parser.hasError()) {
                                return {};
                            }
                        }

                        entry.message = message;
                        foundMessage = true;
                        depth -= 1;
                    }
                    break;
                    case QXmlStreamReader::TokenType::EndElement: {
                        depth -= 1;
                        if (depth == 0 && m_parser.qualifiedName() == "log4j:Event"_L1) {
                            if (foundMessage) {
                                auto consumed = m_parser.characterOffset();
                                if (consumed > 0 && consumed <= m_buffer.length()) {
                                    m_buffer = m_buffer.right(m_buffer.length() - consumed);

                                    if (!m_buffer.isEmpty() && m_buffer.trimmed().isEmpty()) {
                                        // only whitespace, dump it
                                        m_buffer.clear();
                                    }
                                }
                                clearError();
                                return entry;
                            }
                            m_parser.raiseError("log4j:Event Missing required attribute: message");
                            setError();
                            return {};
                        }
                    } break;
                    case QXmlStreamReader::TokenType::EndDocument: {
                        return {};
                    } break;
                    default: {
                        // no op
                    }
                }
            }

            if (m_parser.hasError()) {
                return {};
            }
        }
    }

    throw std::runtime_error("unreachable: already verified this was a complete log4j:Event");
}

MessageLevel::Enum LogParser::guessLevel(const QString& line, MessageLevel::Enum level)
{
    static const QRegularExpression LINE_WITH_LEVEL("^\\[(?<timestamp>[0-9:]+)\\] \\[[^/]+/(?<level>[^\\]]+)\\]");
    auto match = LINE_WITH_LEVEL.match(line);
    if (match.hasMatch()) {
        // New style logs from log4j
        QString timestamp = match.captured("timestamp");
        QString levelStr = match.captured("level");
        if (levelStr == "INFO")
            level = MessageLevel::Info;
        if (levelStr == "WARN")
            level = MessageLevel::Warning;
        if (levelStr == "ERROR")
            level = MessageLevel::Error;
        if (levelStr == "FATAL")
            level = MessageLevel::Fatal;
        if (levelStr == "TRACE" || levelStr == "DEBUG")
            level = MessageLevel::Debug;
    } else {
        // Old style forge logs
        if (line.contains("[INFO]") || line.contains("[CONFIG]") || line.contains("[FINE]") || line.contains("[FINER]") ||
            line.contains("[FINEST]"))
            level = MessageLevel::Info;
        if (line.contains("[SEVERE]") || line.contains("[STDERR]"))
            level = MessageLevel::Error;
        if (line.contains("[WARNING]"))
            level = MessageLevel::Warning;
        if (line.contains("[DEBUG]"))
            level = MessageLevel::Debug;
    }
    if (level != MessageLevel::Unknown)
        return level;

    if (line.contains("overwriting existing"))
        return MessageLevel::Fatal;

    return MessageLevel::Info;
}
