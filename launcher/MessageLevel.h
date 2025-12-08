#pragma once

#include <qlogging.h>
#include <QString>

/**
 * @brief the MessageLevel Enum
 * defines what level a log message is
 */
enum class MessageLevel {
    Unknown,  /**< No idea what this is or where it came from */
    StdOut,   /**< Undetermined stderr messages */
    StdErr,   /**< Undetermined stdout messages */
    Launcher, /**< Launcher Messages */
    Trace,    /**< Trace Messages */
    Debug,    /**< Debug Messages */
    Info,     /**< Info Messages */
    Message,  /**< Standard Messages */
    Warning,  /**< Warnings */
    Error,    /**< Errors */
    Fatal,    /**< Fatal Errors */
};
MessageLevel messageLevelFromName(const QString& levelName);
MessageLevel messageLevelFromQtMsgType(QtMsgType type);

/* Get message level from a line. Line is modified if it was successful. */
MessageLevel messageLevelFromLine(QString& line);

/* Get message level from a line from the launcher log. Line is modified if it was successful. */
MessageLevel messageLevelFromLauncherLine(QString& line);
