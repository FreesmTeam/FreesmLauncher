#pragma once

#include <QRegularExpression>
#include <QString>

using Filter = std::function<bool(const QString&)>;

namespace Filters {
inline Filter inverse(Filter filter)
{
    return [filter = std::move(filter)](const QString& src) { return !filter(src); };
}

inline Filter contains(QString pattern)
{
    return [pattern = std::move(pattern)](const QString& src) { return src.contains(pattern); };
}

inline Filter equals(QString pattern)
{
    return [pattern = std::move(pattern)](const QString& src) { return src == pattern; };
}

inline Filter equalsAny(QStringList patterns = {})
{
    return [patterns = std::move(patterns)](const QString& src) { return patterns.isEmpty() || patterns.contains(src); };
}

inline Filter equalsOrEmpty(QString pattern)
{
    return [pattern = std::move(pattern)](const QString& src) { return src.isEmpty() || src == pattern; };
}

inline Filter regexp(QRegularExpression pattern)
{
    return [pattern = std::move(pattern)](const QString& src) { return pattern.match(src).hasMatch(); };
}
}  // namespace Filters
