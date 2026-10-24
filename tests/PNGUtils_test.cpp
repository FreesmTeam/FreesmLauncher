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

#include <QByteArray>
#include <QByteArrayView>
#include <QFile>
#include <QObject>
#include <QTest>

#include "PNGUtils.h"

class PNGUtilsTest : public QObject {
    Q_OBJECT

   private slots:
    void test_Trivial()
    {
        QVERIFY(!PNGUtils::isCompletePNG(QByteArrayView{}));
        QVERIFY(!PNGUtils::isCompletePNG(QByteArrayView{ "123" }));
        QVERIFY(!PNGUtils::isCompletePNG(QByteArrayView{ "1234567890abcdef" }));
    }

    void test_File()
    {
        QFile png(QFINDTESTDATA("testdata/PNGUtils/kitteh.png"));
        QVERIFY(png.open(QIODevice::ReadOnly));

        const QByteArray data = png.readAll();
        const qsizetype size = data.size();

        QVERIFY(PNGUtils::isCompletePNG(data));
        QVERIFY(!PNGUtils::isCompletePNG(data.first(size - 1)));
        QVERIFY(!PNGUtils::isCompletePNG(data.last(size - 1)));
        QVERIFY(!PNGUtils::isCompletePNG(data + '\0'));
        QVERIFY(!PNGUtils::isCompletePNG('\0' + data));
        QVERIFY(!PNGUtils::isCompletePNG(data.repeated(2)));

        auto badCRC = data;
        auto badIHDR = data;
        badCRC.back() ^= 1;
        badIHDR.replace(12, 4, "IDAT");

        QVERIFY(!PNGUtils::isCompletePNG(badCRC));
        QVERIFY(!PNGUtils::isCompletePNG(badIHDR));

        for (qsizetype i = 0; i < size - 1; ++i) {
            QVERIFY(!PNGUtils::isCompletePNG(QByteArrayView{ data }.first(i)));
        }
    }
};

QTEST_GUILESS_MAIN(PNGUtilsTest)

#include "PNGUtils_test.moc"
