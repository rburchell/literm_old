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

class tst_Parser : public QObject
{
    Q_OBJECT

private slots:
    void setColor_data();
    void setColor();

    void xtermIndexed_data();
    void xtermIndexed();
};

void tst_Parser::setColor_data()
{
    QTest::addColumn<int>("fgSGR");
    QTest::addColumn<ColorPalette::Color>("fgColor");
    QTest::addColumn<bool>("fgBold");

    QTest::addColumn<int>("bgSGR");
    QTest::addColumn<ColorPalette::Color>("bgColor");
    QTest::addColumn<bool>("bgBold");

    // Normal colors.
    QTest::newRow("black/white")   << 30 << ColorPalette::Black   << false <<  47 << ColorPalette::White    << false;
    QTest::newRow("red/cyan")      << 31 << ColorPalette::Red     << false <<  46 << ColorPalette::Cyan     << false;
    QTest::newRow("green/magenta") << 32 << ColorPalette::Green   << false <<  45 << ColorPalette::Magenta  << false;
    QTest::newRow("yellow/blue")   << 33 << ColorPalette::Yellow  << false <<  44 << ColorPalette::Blue     << false;
    QTest::newRow("blue/yellow")   << 34 << ColorPalette::Blue    << false <<  43 << ColorPalette::Yellow   << false;
    QTest::newRow("magenta/green") << 35 << ColorPalette::Magenta << false <<  42 << ColorPalette::Green    << false;
    QTest::newRow("cyan/red")      << 36 << ColorPalette::Cyan    << false <<  41 << ColorPalette::Red      << false;
    QTest::newRow("white/black")   << 37 << ColorPalette::White   << false <<  40 << ColorPalette::Black    << false;

    // Bold FG, normal BG.
    QTest::newRow("Bblack/white")   << 90 << ColorPalette::Black   << true <<  47 << ColorPalette::White    << false;
    QTest::newRow("Bred/cyan")      << 91 << ColorPalette::Red     << true <<  46 << ColorPalette::Cyan     << false;
    QTest::newRow("Bgreen/magenta") << 92 << ColorPalette::Green   << true <<  45 << ColorPalette::Magenta  << false;
    QTest::newRow("Byellow/blue")   << 93 << ColorPalette::Yellow  << true <<  44 << ColorPalette::Blue     << false;
    QTest::newRow("Bblue/yellow")   << 94 << ColorPalette::Blue    << true <<  43 << ColorPalette::Yellow   << false;
    QTest::newRow("Bmagenta/green") << 95 << ColorPalette::Magenta << true <<  42 << ColorPalette::Green    << false;
    QTest::newRow("Bcyan/red")      << 96 << ColorPalette::Cyan    << true <<  41 << ColorPalette::Red      << false;
    QTest::newRow("Bwhite/black")   << 97 << ColorPalette::White   << true <<  40 << ColorPalette::Black    << false;

    // Normal FG, bold BG.
    QTest::newRow("black/Bwhite")   << 30 << ColorPalette::Black   << false << 107 << ColorPalette::White   << true;
    QTest::newRow("red/Bcyan")      << 31 << ColorPalette::Red     << false << 106 << ColorPalette::Cyan    << true;
    QTest::newRow("green/Bmagenta") << 32 << ColorPalette::Green   << false << 105 << ColorPalette::Magenta << true;
    QTest::newRow("yellow/Bblue")   << 33 << ColorPalette::Yellow  << false << 104 << ColorPalette::Blue    << true;
    QTest::newRow("blue/Byellow")   << 34 << ColorPalette::Blue    << false << 103 << ColorPalette::Yellow  << true;
    QTest::newRow("magenta/Bgreen") << 35 << ColorPalette::Magenta << false << 102 << ColorPalette::Green   << true;
    QTest::newRow("cyan/Bred")      << 36 << ColorPalette::Cyan    << false << 101 << ColorPalette::Red     << true;
    QTest::newRow("white/Bblack")   << 37 << ColorPalette::White   << false << 100 << ColorPalette::Black   << true;

    // Bold FG, bold BG.
    QTest::newRow("Bblack/Bwhite")   << 90 << ColorPalette::Black   << true << 107 << ColorPalette::White   << true;
    QTest::newRow("Bred/Bcyan")      << 91 << ColorPalette::Red     << true << 106 << ColorPalette::Cyan    << true;
    QTest::newRow("Bgreen/Bmagenta") << 92 << ColorPalette::Green   << true << 105 << ColorPalette::Magenta << true;
    QTest::newRow("Byellow/Bblue")   << 93 << ColorPalette::Yellow  << true << 104 << ColorPalette::Blue    << true;
    QTest::newRow("Bblue/Byellow")   << 94 << ColorPalette::Blue    << true << 103 << ColorPalette::Yellow  << true;
    QTest::newRow("Bmagenta/Bgreen") << 95 << ColorPalette::Magenta << true << 102 << ColorPalette::Green   << true;
    QTest::newRow("Bcyan/Bred")      << 96 << ColorPalette::Cyan    << true << 101 << ColorPalette::Red     << true;
    QTest::newRow("Bwhite/Bblack")   << 97 << ColorPalette::White   << true << 100 << ColorPalette::Black   << true;
}

// Test the "basic" indexed SGR color attributes
void tst_Parser::setColor()
{
    QFETCH(int, fgSGR);
    QFETCH(ColorPalette::Color, fgColor);
    QFETCH(bool, fgBold);

    QFETCH(int, bgSGR);
    QFETCH(ColorPalette::Color, bgColor);
    QFETCH(bool, bgBold);

    Screen s;
    Parser p(&s);

    QByteArray data;

    // Check default state
    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().foreground), s.colorPalette()->defaultForeground());
    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().background), s.colorPalette()->defaultBackground());

    data = QString::asprintf("\033[%d;%dm", fgSGR, bgSGR).toUtf8();
    p.addData(data);

    // Check expected state
    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().foreground), s.colorPalette()->color(fgColor, fgBold));
    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().background), s.colorPalette()->color(bgColor, bgBold));

    // Check reset state
    data = QString("\033[39;49m").toUtf8();
    p.addData(data);

    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().foreground), s.colorPalette()->defaultForeground());
    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().background), s.colorPalette()->defaultBackground());
}

void tst_Parser::xtermIndexed_data()
{
    QTest::addColumn<int>("fgSGR");
    QTest::addColumn<ColorPalette::Color>("fgColor");
    QTest::addColumn<bool>("fgBold");

    QTest::addColumn<int>("bgSGR");
    QTest::addColumn<ColorPalette::Color>("bgColor");
    QTest::addColumn<bool>("bgBold");

    // Normal colors.
    QTest::newRow("black/white")     << 0  << ColorPalette::Black   << false <<  7 << ColorPalette::White    << false;
    QTest::newRow("red/cyan")        << 1  << ColorPalette::Red     << false <<  6 << ColorPalette::Cyan     << false;
    QTest::newRow("green/magenta")   << 2  << ColorPalette::Green   << false <<  5 << ColorPalette::Magenta  << false;
    QTest::newRow("yellow/blue")     << 3  << ColorPalette::Yellow  << false <<  4 << ColorPalette::Blue     << false;
    QTest::newRow("blue/yellow")     << 4  << ColorPalette::Blue    << false <<  3 << ColorPalette::Yellow   << false;
    QTest::newRow("magenta/green")   << 5  << ColorPalette::Magenta << false <<  2 << ColorPalette::Green    << false;
    QTest::newRow("cyan/red")        << 6  << ColorPalette::Cyan    << false <<  1 << ColorPalette::Red      << false;
    QTest::newRow("white/black")     << 7  << ColorPalette::White   << false <<  0 << ColorPalette::Black    << false;

    // Bold FG, normal BG.
    QTest::newRow("Bblack/white")    << 8  << ColorPalette::Black   << true <<   7 << ColorPalette::White    << false;
    QTest::newRow("Bred/cyan")       << 9  << ColorPalette::Red     << true <<   6 << ColorPalette::Cyan     << false;
    QTest::newRow("Bgreen/magenta")  << 10 << ColorPalette::Green   << true <<   5 << ColorPalette::Magenta  << false;
    QTest::newRow("Byellow/blue")    << 11 << ColorPalette::Yellow  << true <<   4 << ColorPalette::Blue     << false;
    QTest::newRow("Bblue/yellow")    << 12 << ColorPalette::Blue    << true <<   3 << ColorPalette::Yellow   << false;
    QTest::newRow("Bmagenta/green")  << 13 << ColorPalette::Magenta << true <<   2 << ColorPalette::Green    << false;
    QTest::newRow("Bcyan/red")       << 14 << ColorPalette::Cyan    << true <<   1 << ColorPalette::Red      << false;
    QTest::newRow("Bwhite/black")    << 15 << ColorPalette::White   << true <<   0 << ColorPalette::Black    << false;

    // Normal FG, bold BG.
    QTest::newRow("black/Bwhite")    << 0  << ColorPalette::Black   << false <<  15 << ColorPalette::White   << true;
    QTest::newRow("red/Bcyan")       << 1  << ColorPalette::Red     << false <<  14 << ColorPalette::Cyan    << true;
    QTest::newRow("green/Bmagenta")  << 2  << ColorPalette::Green   << false <<  13 << ColorPalette::Magenta << true;
    QTest::newRow("yellow/Bblue")    << 3  << ColorPalette::Yellow  << false <<  12 << ColorPalette::Blue    << true;
    QTest::newRow("blue/Byellow")    << 4  << ColorPalette::Blue    << false <<  11 << ColorPalette::Yellow  << true;
    QTest::newRow("magenta/Bgreen")  << 5  << ColorPalette::Magenta << false <<  10 << ColorPalette::Green   << true;
    QTest::newRow("cyan/Bred")       << 6  << ColorPalette::Cyan    << false <<   9 << ColorPalette::Red     << true;
    QTest::newRow("white/Bblack")    << 7  << ColorPalette::White   << false <<   8 << ColorPalette::Black   << true;

    // Bold FG, bold BG.
    QTest::newRow("Bblack/Bwhite")   << 8  << ColorPalette::Black   << true <<  15 << ColorPalette::White   << true;
    QTest::newRow("Bred/Bcyan")      << 9  << ColorPalette::Red     << true <<  14 << ColorPalette::Cyan    << true;
    QTest::newRow("Bgreen/Bmagenta") << 10 << ColorPalette::Green   << true <<  13 << ColorPalette::Magenta << true;
    QTest::newRow("Byellow/Bblue")   << 11 << ColorPalette::Yellow  << true <<  12 << ColorPalette::Blue    << true;
    QTest::newRow("Bblue/Byellow")   << 12 << ColorPalette::Blue    << true <<  11 << ColorPalette::Yellow  << true;
    QTest::newRow("Bmagenta/Bgreen") << 13 << ColorPalette::Magenta << true <<  10 << ColorPalette::Green   << true;
    QTest::newRow("Bcyan/Bred")      << 14 << ColorPalette::Cyan    << true <<  9  << ColorPalette::Red     << true;
    QTest::newRow("Bwhite/Bblack")   << 15 << ColorPalette::White   << true <<  8  << ColorPalette::Black   << true;
}

// SGR 38/48, 5: indexed colors.
// The first 0-7 are the standard terminal colors. The last 8-15 are the bright
// (or bold) versions.
void tst_Parser::xtermIndexed()
{
    QFETCH(int, fgSGR);
    QFETCH(ColorPalette::Color, fgColor);
    QFETCH(bool, fgBold);

    QFETCH(int, bgSGR);
    QFETCH(ColorPalette::Color, bgColor);
    QFETCH(bool, bgBold);

    Screen s;
    Parser p(&s);

    QByteArray data;

    // Check default state
    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().foreground), s.colorPalette()->defaultForeground());
    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().background), s.colorPalette()->defaultBackground());

    data = QString::asprintf("\033[38;5;%d;48;5;%dm", fgSGR, bgSGR).toUtf8();
    p.addData(data);


    // Check expected state
    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().foreground), s.colorPalette()->color(fgColor, fgBold));
    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().background), s.colorPalette()->color(bgColor, bgBold));

    // Check reset state
    data = QString("\033[39;49m").toUtf8();
    p.addData(data);

    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().foreground), s.colorPalette()->defaultForeground());
    QCOMPARE(QColor(s.currentCursor()->currentTextStyle().background), s.colorPalette()->defaultBackground());
}

#include <tst_parser.moc>
QTEST_MAIN(tst_Parser);
