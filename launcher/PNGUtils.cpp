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

#include <QtEndian>
#include <array>

#include "PNGUtils.h"

bool PNGUtils::isCompletePNG(QByteArrayView bytes)
{
    using chunkLengthType = quint32;
    constexpr qsizetype chunkLengthSize = 4, chunkTypeSize = 4, chunkCRCSize = 4, pngHeaderSize = 8;
    constexpr qsizetype overheadSize = chunkLengthSize + chunkTypeSize + chunkCRCSize;
    constexpr std::array<char, pngHeaderSize> pngHeader{ '\x89', 'P', 'N', 'G', '\r', '\n', '\x1a', '\n' };
    constexpr std::array<char, chunkCRCSize> iendCRC{ '\xAE', '\x42', '\x60', '\x82' };
    static_assert(sizeof(chunkLengthType) == chunkLengthSize);

    const qsizetype size = bytes.size();
    if (!bytes.startsWith(pngHeader)) {
        return false;
    }

    qsizetype i = pngHeaderSize;
    while (i + chunkLengthSize + chunkTypeSize + chunkCRCSize <= size) {
        chunkLengthType len = qFromBigEndian<chunkLengthType>(bytes.sliced(i, chunkLengthSize).constData());
        if (len > size - i - overheadSize) {
            return false;
        }

        auto typeBytes = bytes.sliced(i + chunkLengthSize, chunkTypeSize);
        if ((typeBytes == "IHDR" && i != pngHeaderSize) || (i == pngHeaderSize && typeBytes != "IHDR")) {
            return false;
        }
        if (typeBytes == "IEND" && len == 0) {
            auto crcBytes = bytes.sliced(i + chunkLengthSize + chunkTypeSize + len, chunkCRCSize);
            return crcBytes == QByteArrayView(iendCRC) && i + overheadSize == size;
        }

        i += overheadSize + len;
    }

    return false;
}
