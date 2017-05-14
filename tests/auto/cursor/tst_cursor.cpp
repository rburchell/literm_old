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

class tst_Cursor : public QObject
{
    Q_OBJECT

private slots:
    void construct();
    void stayWithinBounds();
    void moveDownAndUp();
    void moveRightAndLeft();
};

void tst_Cursor::construct()
{
    Screen s(0, true /* testMode */);
    Cursor *cur = s.currentCursor();

    // Ensure sanity for the screen. If this fails, the rest of the tests are
    // going to have a bad day, so it makes sense to check this here I think.
    QCOMPARE(s.width(), 80);
    QCOMPARE(s.height(), 25);

    // Cursor should start at the origin.
    QCOMPARE(cur->x(), 0);
    QCOMPARE(cur->y(), 0);
    QCOMPARE(cur->new_x(), 0);
    QCOMPARE(cur->new_y(), 0);
}

struct CursorRestorer
{
    CursorRestorer(Cursor *cursor) : m_cursor(cursor) {}
    ~CursorRestorer()
    {
        m_cursor->moveOrigin();
        QCOMPARE(m_cursor->new_x(), 0);
        QCOMPARE(m_cursor->new_y(), 0);
    }

    Cursor *m_cursor = 0;
};

void tst_Cursor::stayWithinBounds()
{
    Screen s(0, true /* testMode */);
    Cursor *cur = s.currentCursor();

    // up off-by-one
    cur->moveUp(); // no-op, hopefully
    QCOMPARE(cur->new_x(), 0);
    QCOMPARE(cur->new_y(), 0);

    // up off-by-many
    cur->moveUp(10); // no-op, hopefully
    QCOMPARE(cur->new_x(), 0);
    QCOMPARE(cur->new_y(), 0);

    // left off-by-one
    cur->moveLeft(); // no-op, hopefully
    QCOMPARE(cur->new_x(), 0);
    QCOMPARE(cur->new_y(), 0);

    // left off-by-many
    cur->moveLeft(10); // no-op, hopefully
    QCOMPARE(cur->new_x(), 0);
    QCOMPARE(cur->new_y(), 0);

    {
        CursorRestorer restore(cur);
        // right off-by-one
        cur->moveRight(s.width() - 1);
        QCOMPARE(cur->new_x(), s.width() - 1);
        QCOMPARE(cur->new_y(), 0);
        cur->moveRight(1); // no-op, hopefully
        QCOMPARE(cur->new_x(), s.width() - 1);
        QCOMPARE(cur->new_y(), 0);
    }

    {
        CursorRestorer restore(cur);

        // right off-by-many
        cur->moveRight(s.width());
        QCOMPARE(cur->new_x(), s.width() - 1);
        QCOMPARE(cur->new_y(), 0);
    }

    {
        CursorRestorer restore(cur);

        // down off-by-one
        cur->moveDown(s.height() - 1);
        QCOMPARE(cur->new_x(), 0);
        QCOMPARE(cur->new_y(), s.height() - 1);
        cur->moveDown(1); // no-op, hopefully
        QCOMPARE(cur->new_x(), 0);
        QCOMPARE(cur->new_y(), s.height() - 1);
    }

    {
        CursorRestorer restore(cur);

        // down off-by-many
        cur->moveDown(s.height());
        QCOMPARE(cur->new_x(), 0);
        QCOMPARE(cur->new_y(), s.height() - 1);
    }
}

void tst_Cursor::moveDownAndUp()
{
    Screen s(0, true /* testMode */);
    Cursor *cur = s.currentCursor();

    cur->moveDown();
    QCOMPARE(cur->new_x(), 0);
    QCOMPARE(cur->new_y(), 1);
    cur->moveDown();
    QCOMPARE(cur->new_x(), 0);
    QCOMPARE(cur->new_y(), 2);

    // back up
    cur->moveUp();
    QCOMPARE(cur->new_x(), 0);
    QCOMPARE(cur->new_y(), 1);
    cur->moveUp();
    QCOMPARE(cur->new_x(), 0);
    QCOMPARE(cur->new_y(), 0);
}

void tst_Cursor::moveRightAndLeft()
{
    Screen s(0, true /* testMode */);
    Cursor *cur = s.currentCursor();

    cur->moveRight();
    QCOMPARE(cur->new_x(), 1);
    QCOMPARE(cur->new_y(), 0);
    cur->moveRight();
    QCOMPARE(cur->new_x(), 2);
    QCOMPARE(cur->new_y(), 0);

    // back left
    cur->moveLeft();
    QCOMPARE(cur->new_x(), 1);
    QCOMPARE(cur->new_y(), 0);
    cur->moveLeft();
    QCOMPARE(cur->new_x(), 0);
    QCOMPARE(cur->new_y(), 0);
}

#include <tst_cursor.moc>
QTEST_MAIN(tst_Cursor);
