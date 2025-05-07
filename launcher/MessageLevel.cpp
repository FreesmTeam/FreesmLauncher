#include "MessageLevel.h"

MessageLevel::Enum MessageLevel::getLevel(const QString& levelName)
{
    QString name = levelName.toUpper();
    if (name == "LAUNCHER")
        return MessageLevel::Launcher;
    else if (name == "TRACE")
        return MessageLevel::Trace;
    else if (name == "DEBUG")
        return MessageLevel::Debug;
    else if (name == "INFO")
        return MessageLevel::Info;
    else if (name == "MESSAGE")
        return MessageLevel::Message;
    else if (name == "WARNING" || name == "WARN")
        return MessageLevel::Warning;
    else if (name == "ERROR")
        return MessageLevel::Error;
    else if (name == "FATAL")
        return MessageLevel::Fatal;
    // Skip PrePost, it's not exposed to !![]!
    // Also skip StdErr and StdOut
    else
        return MessageLevel::Unknown;
}

MessageLevel::Enum MessageLevel::fromLine(QString& line)
{
    // Level prefix
    int endmark = line.indexOf("]!");
    if (line.startsWith("!![") && endmark != -1) {
        auto level = MessageLevel::getLevel(line.left(endmark).mid(3));
        line = line.mid(endmark + 2);
        return level;
    }
    return MessageLevel::Unknown;
}
