/******************************************************************************
 * Copyright (c) 2012 Jørgen Lind
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

#include "screen.h"

#include "screen_data.h"
#include "block.h"
#include "cursor.h"
#include "text.h"
#include "scrollback.h"
#include "selection.h"

#include "controll_chars.h"
#include "character_sets.h"

#include <QtCore/QTimer>
#include <QtCore/QSocketNotifier>
#include <QtCore/QLoggingCategory>
#include <QtGui/QGuiApplication>

#include <QtCore/QDebug>

#include <float.h>
#include <cmath>

Q_LOGGING_CATEGORY(lcScreen, "yat.screen", QtWarningMsg)

/*!
     Creates a new screen instance with the specified \a parent. If \a testMode
     is true, then the pty will not be connected, with the expectation being that
     either the parser is being driven externally, or that no parser is directly
     involved (i.e. the cursor is being manipulated directly by a test).
*/
Screen::Screen(QObject *parent, bool testMode)
    : QObject(parent)
    , m_palette(new ColorPalette(this))
    , m_parser(this)
    , m_timer_event_id(0)
    , m_width(1)
    , m_height(0)
    , m_primary_data(new ScreenData(500, this))
    , m_alternate_data(new ScreenData(0, this))
    , m_current_data(m_primary_data)
    , m_old_current_data(m_primary_data)
    , m_selection(new Selection(this))
    , m_flash(false)
    , m_cursor_changed(false)
    , m_application_cursor_key_mode(false)
    , m_fast_scroll(true)
    , m_default_background(m_palette->normalColor(ColorPalette::DefaultBackground))
{
    Cursor *cursor = new Cursor(this);
    m_cursor_stack << cursor;
    m_new_cursors << cursor;

    connect(m_primary_data, SIGNAL(contentHeightChanged()), this, SIGNAL(contentHeightChanged()));
    connect(m_primary_data, &ScreenData::contentModified, this, &Screen::contentModified);
    connect(m_primary_data, &ScreenData::dataHeightChanged, this, &Screen::dataHeightChanged);
    connect(m_primary_data, &ScreenData::dataWidthChanged, this, &Screen::dataWidthChanged);
    connect(m_palette, SIGNAL(changed()), this, SLOT(paletteChanged()));

    setHeight(25);
    setWidth(80);

    if (!testMode) {
        connect(&m_pty, &YatPty::readyRead, this, &Screen::readData);
        connect(&m_pty, &YatPty::hangupReceived,this, &Screen::hangup);
    }
}

Screen::~Screen()
{
    for(int i = 0; i < m_to_delete.size(); i++) {
        delete m_to_delete.at(i);
    }
    //m_to_delete.clear();

    delete m_primary_data;
    delete m_alternate_data;
}

QColor Screen::defaultForegroundColor() const
{
    return m_palette->normalColor(ColorPalette::DefaultForeground);
}

QColor Screen::defaultBackgroundColor() const
{
    return m_palette->normalColor(ColorPalette::DefaultBackground);
}

void Screen::emitRequestHeight(int newHeight)
{
    qCDebug(lcScreen) << "Requesting height " << newHeight;
    emit requestHeightChange(newHeight);
}

void Screen::setHeight(int height)
{
    height = std::max(1, height);
    if (height == m_height)
        return;

    qCDebug(lcScreen) << "Setting height " << height;
    m_height = height;

    m_primary_data->setHeight(height, currentCursor()->new_y());
    m_alternate_data->setHeight(height, currentCursor()->new_y());

    m_pty.setHeight(height, height * 10);

    emit heightChanged();
    scheduleEventDispatch();
}

int Screen::height() const
{
    return m_height;
}

int Screen::contentHeight() const
{
    return currentScreenData()->contentHeight();
}

void Screen::emitRequestWidth(int newWidth)
{
    qCDebug(lcScreen) << "Requesting width " << newWidth;
    emit requestWidthChange(newWidth);
}

void Screen::setWidth(int width)
{
    width = std::max(1,width);
    if (width == m_width)
        return;

    qCDebug(lcScreen) << "Width about to change to " << width;
    emit widthAboutToChange(width);

    qCDebug(lcScreen) << "Setting width " << width;
    m_width = width;

    m_primary_data->setWidth(width);
    m_alternate_data->setWidth(width);

    m_pty.setWidth(width, width * 10);

    emit widthChanged();
    scheduleEventDispatch();
}

int Screen::width() const
{
    return m_width;
}

void Screen::useAlternateScreenBuffer()
{
    if (m_current_data == m_primary_data) {
        qCDebug(lcScreen) << "Switching to alternate screen buffer";
        disconnect(m_primary_data, SIGNAL(contentHeightChanged()), this, SIGNAL(contentHeightChanged()));
        disconnect(m_primary_data, &ScreenData::contentModified, this, &Screen::contentModified);
        disconnect(m_primary_data, &ScreenData::dataHeightChanged, this, &Screen::dataHeightChanged);
        disconnect(m_primary_data, &ScreenData::dataWidthChanged, this, &Screen::dataWidthChanged);
        m_current_data = m_alternate_data;
        m_current_data->clear();
        connect(m_alternate_data, SIGNAL(contentHeightChanged()), this, SIGNAL(contentHeightChanged()));
        connect(m_alternate_data, &ScreenData::contentModified, this, &Screen::contentModified);
        connect(m_alternate_data, &ScreenData::dataHeightChanged, this, &Screen::dataHeightChanged);
        connect(m_alternate_data, &ScreenData::dataWidthChanged, this, &Screen::dataWidthChanged);
        emit contentHeightChanged();
    }
}

void Screen::useNormalScreenBuffer()
{
    if (m_current_data == m_alternate_data) {
        qCDebug(lcScreen) << "Switching to normal screen buffer";
        disconnect(m_alternate_data, SIGNAL(contentHeightChanged()), this, SIGNAL(contentHeightChanged()));
        disconnect(m_alternate_data, &ScreenData::contentModified, this, &Screen::contentModified);
        disconnect(m_alternate_data, &ScreenData::dataHeightChanged, this, &Screen::dataHeightChanged);
        disconnect(m_alternate_data, &ScreenData::dataWidthChanged, this, &Screen::dataWidthChanged);
        m_current_data = m_primary_data;
        connect(m_primary_data, SIGNAL(contentHeightChanged()), this, SIGNAL(contentHeightChanged()));
        connect(m_primary_data, &ScreenData::contentModified, this, &Screen::contentModified);
        connect(m_primary_data, &ScreenData::dataHeightChanged, this, &Screen::dataHeightChanged);
        connect(m_primary_data, &ScreenData::dataWidthChanged, this, &Screen::dataWidthChanged);
        emit contentHeightChanged();
    }
}

TextStyle Screen::defaultTextStyle() const
{
    TextStyle style;
    style.style = TextStyle::Normal;
    style.foreground = colorPalette()->defaultForeground().rgb();
    style.background = colorPalette()->defaultBackground().rgb();
    return style;
}

void Screen::saveCursor()
{
    Cursor *new_cursor = new Cursor(this);
    if (m_cursor_stack.size())
        m_cursor_stack.last()->setVisible(false);
    m_cursor_stack << new_cursor;
    m_new_cursors << new_cursor;
    qCDebug(lcScreen) << "Saved cursor, stack size: " << m_cursor_stack.size();
}

void Screen::restoreCursor()
{
    if (m_cursor_stack.size() <= 1)
        return;

    m_delete_cursors.append(m_cursor_stack.takeLast());
    m_cursor_stack.last()->setVisible(true);
    qCDebug(lcScreen) << "Restored cursor, stack size: " << m_cursor_stack.size();
}

void Screen::clearScreen()
{
    currentScreenData()->clear();
}


ColorPalette *Screen::colorPalette() const
{
    return m_palette;
}

void Screen::fill(const QChar character)
{
    currentScreenData()->fill(character);
}

void Screen::clear()
{
    fill(QChar(' '));
}

void Screen::setFastScroll(bool fast)
{
    m_fast_scroll = fast;
}

bool Screen::fastScroll() const
{
    return m_fast_scroll;
}

Selection *Screen::selection() const
{
    return m_selection;
}

void Screen::doubleClicked(double character, double line)
{
    QPoint start, end;
    int64_t charInt = std::llround(character);
    int64_t lineInt = std::llround(line);
    Q_ASSERT(charInt >= 0);
    //Q_ASSERT(charInt < std::numeric_limits<std::size_t>::max());
    Q_ASSERT(lineInt >= 0);
    //Q_ASSERT(lineInt < std::numeric_limits<std::size_t>::max());
    auto selectionRange = currentScreenData()->getDoubleClickSelectionRange(charInt, lineInt);
    m_selection->setStartX(selectionRange.start.x());
    m_selection->setStartY(selectionRange.start.y());
    m_selection->setEndX(selectionRange.end.x());
    m_selection->setEndY(selectionRange.end.y());
}

void Screen::setTitle(const QString &title)
{
    m_title = title;
    emit screenTitleChanged();
}

QString Screen::title() const
{
    return m_title;
}

QString Screen::platformName() const
{
    return qGuiApp->platformName();
}

void Screen::scheduleFlash()
{
    m_flash = true;
}

void Screen::printScreen() const
{
    currentScreenData()->printStyleInformation();
    qDebug() << "Total height: " << currentScreenData()->contentHeight();
}

void Screen::scheduleEventDispatch()
{
    if (!m_timer_event_id) {
        qCDebug(lcScreen) << "Scheduling dispatch";
        m_timer_event_id = startTimer(1);
        m_time_since_initiated.restart();
    }

    m_time_since_parsed.restart();
}

void Screen::dispatchChanges()
{
    qCDebug(lcScreen) << "Dispatching";
    if (m_old_current_data != m_current_data) {
        m_old_current_data->releaseTextObjects();
        m_old_current_data = m_current_data;
    }

    currentScreenData()->dispatchLineEvents();
    emit dispatchTextSegmentChanges();

    //be smarter than this
    static int max_to_delete_size = 0;
    if (max_to_delete_size < m_to_delete.size()) {
        max_to_delete_size = m_to_delete.size();
    }

    if (m_flash) {
        m_flash = false;
        emit flash();
    }

    for (int i = 0; i < m_delete_cursors.size(); i++) {
        int new_index = m_new_cursors.indexOf(m_delete_cursors.at(i));
        if (new_index >= 0)
            m_new_cursors.remove(new_index);
        delete m_delete_cursors.at(i);
    }
    m_delete_cursors.clear();

    for (int i = 0; i < m_new_cursors.size(); i++) {
        emit cursorCreated(m_new_cursors.at(i));
    }
    m_new_cursors.clear();

    for (int i = 0; i < m_cursor_stack.size(); i++) {
        m_cursor_stack[i]->dispatchEvents();
    }

    m_selection->dispatchChanges();
}

void Screen::sendPrimaryDA()
{
    m_pty.write(QByteArrayLiteral("\033[?6c"));

}

void Screen::sendSecondaryDA()
{
    m_pty.write(QByteArrayLiteral("\033[>1;95;0c"));
}

void Screen::setApplicationCursorKeysMode(bool enable)
{
    m_application_cursor_key_mode = enable;
}

bool Screen::applicationCursorKeyMode() const
{
    return m_application_cursor_key_mode;
}

void Screen::ensureVisibleLines(int top_line)
{
    currentScreenData()->ensureVisibleLines(top_line);
}

YatPty *Screen::pty()
{
    return &m_pty;
}

Text *Screen::createTextSegment(const TextStyleLine &style_line)
{
    Q_UNUSED(style_line);
    Text *to_return;
    if (m_to_delete.size()) {
        to_return = m_to_delete.takeLast();
        to_return->setVisible(true);
    } else {
        to_return = new Text(this);
        emit textCreated(to_return);
    }

    return to_return;
}

void Screen::releaseTextSegment(Text *text)
{
    m_to_delete.append(text);
}

void Screen::readData(const QByteArray &data)
{
    m_parser.addData(data);

    scheduleEventDispatch();
}

void Screen::paletteChanged()
{
    QColor new_default = m_palette->normalColor(ColorPalette::DefaultBackground);
    if (new_default != m_default_background) {
        m_default_background = new_default;
        emit defaultBackgroundColorChanged();
    }
}

void Screen::timerEvent(QTimerEvent *)
{
    if (m_timer_event_id && (m_time_since_parsed.elapsed() > 3 || m_time_since_initiated.elapsed() > 25)) {
        qCDebug(lcScreen) << "Preparing to dispatch time_since_parsed " << m_time_since_parsed.elapsed() << " time_since_initiated " << m_time_since_initiated.elapsed();
        killTimer(m_timer_event_id);
        m_timer_event_id = 0;
        dispatchChanges();
    }
}
