/******************************************************************************
 * Copyright (C) 2017 Robin Burchell <robin+git@viroteck.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ******************************************************************************/

#include "../../../backend/block.h"
#include <QtTest/QtTest>

#include "../../../backend/screen.h"
#include "../../../backend/screen_data.h"
#include "../../../backend/cursor.h"

class tst_Screen : public QObject
{
    Q_OBJECT

private slots:
    void construct();
};

void tst_Screen::construct()
{
    Screen s;

    // Ensure sanity for the screen
    QCOMPARE(s.width(), 80);
    QCOMPARE(s.height(), 25);

    // Ensure sanity for the cursor
    QVERIFY(s.currentCursor()->x() >= 0);
    QVERIFY(s.currentCursor()->x() < s.width());
    QVERIFY(s.currentCursor()->y() >= 0);
    QVERIFY(s.currentCursor()->y() < s.height());
    QVERIFY(s.currentCursor()->visible());
}

#include <tst_screen.moc>
QTEST_MAIN(tst_Screen);
