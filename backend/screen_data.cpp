/*******************************************************************************
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
*******************************************************************************/

#include "screen_data.h"

#include "block.h"
#include "screen.h"
#include "scrollback.h"
#include "cursor.h"

#include <stdio.h>

#include <QtGui/QGuiApplication>
#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>

Q_LOGGING_CATEGORY(lcScreenData, "yat.screen_data", QtWarningMsg)

ScreenData::ScreenData(size_t max_scrollback, Screen *screen)
    : QObject(screen)
    , m_screen(screen)
    , m_scrollback(new Scrollback(max_scrollback))
    , m_screen_height(0)
    , m_height(0)
    , m_width(0)
    , m_block_count(0)
    , m_old_total_lines(0)
{
}

ScreenData::~ScreenData()
{
    for (auto it = m_screen_blocks.begin(); it != m_screen_blocks.end(); ++it) {
        delete *it;
    }
    delete m_scrollback;
}


int ScreenData::contentHeight() const
{
    return m_height + m_scrollback->height();
}

void ScreenData::setSize(int width, int height, int currentCursorLine)
{
    const int oldWidth = m_width;
    const int oldScreenHeight = m_screen_height;
    const int oldContentHeight = m_height;
    int removed = 0;
    int reclaimed = 0;
    bool somethingHappened = false;

    if (width != m_width) {
        m_width = width;

        for (Block *block : m_screen_blocks) {
            int before_count = block->lineCount();
            block->setWidth(width);
            m_height += block->lineCount() - before_count;
        }

        m_scrollback->setWidth(screen()->height(), width);

        if (m_height > m_screen_height) {
            int to_remove = m_height - m_screen_height;
            removed = push_at_most_to_scrollback(to_remove);
        } else {
            reclaimed = ensure_at_least_height(m_screen_height);
        }
        qCDebug(lcScreenData) << "Changed width from " << oldWidth << " to " << width << " height was " << oldContentHeight << " now " << m_height << " -- pushed " << removed << " reclaimed " << reclaimed;
        somethingHappened = true;
    }

    if (height != m_screen_height) {
        m_screen_height = height;

        int removed_beginning = 0;
        int removed_end = 0;

        if (m_height > height) {
            const int to_remove = m_height - height;
            const int remove_from_end = std::min(m_height - (currentCursorLine + 1), to_remove);
            const int remove_from_start = to_remove - remove_from_end;

            if (remove_from_end) {
                removed_end = remove_lines_from_end(remove_from_end);
            }
            if (remove_from_start) {
                removed_beginning = push_at_most_to_scrollback(remove_from_start);
            }

            qCDebug(lcScreenData) << "Changed height from " << oldScreenHeight << " to " << m_screen_height << " -- culling " << remove_from_end << " removed " << removed_end << " and start " << remove_from_start << " removed " << removed_beginning;
        } else {
            reclaimed = ensure_at_least_height(height);
            qCDebug(lcScreenData) << "Reclaimed" << reclaimed << "lines from scrollback";
        }

        removed += removed_beginning;
        somethingHappened = true;
    }

    if (somethingHappened) {
        qCDebug(lcScreenData) << "dataSizeChanged" << m_width << removed << reclaimed;
        emit dataSizeChanged(m_width, m_screen_height, removed, reclaimed);
    }
}


void ScreenData::clearToEndOfLine(const QPoint &point)
{
    auto it = it_for_row_ensure_single_line_block(point.y());
    if (it != m_screen_blocks.end())
        (*it)->clearToEnd(point.x());
}

void ScreenData::clearToEndOfScreen(int y)
{
    auto it = it_for_row_ensure_single_line_block(y);
    while(it != m_screen_blocks.end()) {
        clearBlock(it);
        ++it;
    }
}

void ScreenData::clearToBeginningOfLine(const QPoint &point)
{
    auto it = it_for_row_ensure_single_line_block(point.y());
    (*it)->clearCharacters(0,point.x());
}

void ScreenData::clearToBeginningOfScreen(int y)
{
    auto it = it_for_row_ensure_single_line_block(y);
    if (it != m_screen_blocks.end())
        (*it)->clear();
    while(it != m_screen_blocks.begin()) {
        --it;
        clearBlock(it);
    }
}

void ScreenData::clearLine(const QPoint &point)
{
    (*it_for_row_ensure_single_line_block(point.y()))->clear();
}

void ScreenData::clear()
{
    for (auto it = m_screen_blocks.begin(); it != m_screen_blocks.end(); ++it) {
        clearBlock(it);
    }
}

void ScreenData::releaseTextObjects()
{
    for (auto it = m_screen_blocks.begin(); it != m_screen_blocks.end(); ++it) {
        (*it)->releaseTextObjects();
    }
}

void ScreenData::clearCharacters(const QPoint &point, int to)
{
    auto it = it_for_row_ensure_single_line_block(point.y());
    (*it)->clearCharacters(point.x(),to);
}

void ScreenData::deleteCharacters(const QPoint &point, int to)
{
    auto it = it_for_row(point.y());
    if (it  == m_screen_blocks.end())
        return;

    int line_in_block = point.y() - (*it)->screenIndex();
    int chars_to_line = line_in_block * m_width;

    (*it)->deleteCharacters(chars_to_line + point.x(), chars_to_line + to);
}

const CursorDiff ScreenData::replace(const QPoint &point, const QString &text, const TextStyle &style, bool only_latin)
{
    return modify(point,text,style,true, only_latin);
}

const CursorDiff ScreenData::insert(const QPoint &point, const QString &text, const TextStyle &style, bool only_latin)
{
    return modify(point,text,style,false, only_latin);
}


void ScreenData::moveLine(int from, int to)
{
    if (from == to)
        return;

    const size_t old_content_height = contentHeight();
    if (to > from)
        to++;
    auto from_it = it_for_row_ensure_single_line_block(from);
    auto to_it = it_for_row_ensure_single_line_block(to);

    (*from_it)->clear();
    m_screen_blocks.splice(to_it, m_screen_blocks, from_it);
    emit contentModified(m_scrollback->height() + to, 1, content_height_diff(old_content_height));
}

void ScreenData::insertLine(int row, int topMargin)
{
    auto row_it = it_for_row(row + 1);

    const size_t old_content_height = contentHeight();

    if (!topMargin && m_height >= m_screen_height) {
        push_at_most_to_scrollback(1);
    } else {
        auto row_top_margin = it_for_row_ensure_single_line_block(topMargin);
        if (row == topMargin) {
            (*row_top_margin)->clear();
            return;
        }
        delete (*row_top_margin);
        m_screen_blocks.erase(row_top_margin);
        m_height--;
        m_block_count--;
    }

    Block *block_to_insert = new Block(m_screen);
    m_screen_blocks.insert(row_it,block_to_insert);
    m_height++;
    m_block_count++;

    emit contentModified(m_scrollback->height() + row + 1, 1, content_height_diff(old_content_height));
}


void ScreenData::fill(const QChar &character)
{
    clear();
    auto it = --m_screen_blocks.end();
    for (int i = 0; i < m_block_count; --it, i++) {
        QString fill_str(m_screen->width(), character);
        (*it)->replaceAtPos(0, fill_str, m_screen->defaultTextStyle());
    }
}

void ScreenData::dispatchLineEvents()
{
    if (!m_block_count)
        return;

    // We may have an initial multiline block that can't be pushed to scrollback
    // (too big), but can't be fit on screen (also too big). For this case, we
    // need to push the rendering of everything up.
    const int underflow = m_height - m_screen_height;
    const int scrollback_height = m_scrollback->height() + underflow;
    int i = -underflow;

    for (auto it = m_screen_blocks.begin(); it != m_screen_blocks.end(); ++it) {
        int line = scrollback_height + i;
        (*it)->setLine(line);
        //(*it)->setScreenIndex(i);
        (*it)->dispatchEvents();
        i+= (*it)->lineCount();
    }

    if (contentHeight() != m_old_total_lines) {
        m_old_total_lines = contentHeight();
        emit contentHeightChanged();
    }
}

void ScreenData::printRuler(QDebug &debug) const
{
    QString ruler = QString("|----i----").repeated((m_width/10)+1).append("|");
    debug << "    " << (void *) this << ruler;
}

void ScreenData::printStyleInformation() const
{
    auto it = m_screen_blocks.end();
    std::advance(it, -m_block_count);
    for (int i = 0; it != m_screen_blocks.end(); ++it, i++) {
        if (i % 5 == 0) {
            QDebug debug = qDebug();
            debug << "Ruler:";
            printRuler(debug);
        }
        QDebug debug = qDebug();
        (*it)->printStyleList(debug);
    }
    qDebug() << "On screen height" << m_height;
}

Screen *ScreenData::screen() const
{
    return m_screen;
}

void ScreenData::ensureVisibleLines(int top_line)
{
    m_scrollback->ensureVisibleLines(screen()->height(), top_line);
}

void ScreenData::sendSelectionToClipboard(const QPoint &start, const QPoint &end, QClipboard::Mode mode)
{
    if (start.y() < 0)
        return;
    if (end.y() >= contentHeight())
        return;

    QString to_clip_board_buffer;

    bool started_in_scrollback = false;
    if (size_t(start.y()) < m_scrollback->height()) {
        started_in_scrollback = true;
        QPoint end_scrollback = end;
        if (size_t(end.y()) >= m_scrollback->height()) {
            end_scrollback = QPoint(m_width, m_scrollback->height() - 1);
        }
        to_clip_board_buffer = m_scrollback->selection(start, end_scrollback);
    }

    if (size_t(end.y()) >= m_scrollback->height()) {
        QPoint start_in_screen;
        if (started_in_scrollback) {
            start_in_screen = QPoint(0,0);
        } else {
            start_in_screen = start;
            start_in_screen.ry() -= m_scrollback->height();
        }
        QPoint end_in_screen = end;
        end_in_screen.ry() -= m_scrollback->height();

        auto it = it_for_row(start_in_screen.y());
        size_t screen_index = (*it)->screenIndex();
        int start_pos = (start_in_screen.y() - (*it)->screenIndex()) * m_width + start_in_screen.x();
        for (; it != m_screen_blocks.end(); ++it, start_pos = 0) {
            int end_pos = (*it)->textSize();
            bool should_break = false;
            if (size_t(screen_index + (*it)->lineCount()) > size_t(end_in_screen.y())) {
                end_pos = (end_in_screen.y() - screen_index) * m_width + end_in_screen.x();
                should_break = true;
            }
            if (to_clip_board_buffer.size())
                to_clip_board_buffer += '\n';
            to_clip_board_buffer += (*it)->textLine().mid(start_pos, end_pos - start_pos);
            if (should_break)
                break;
            screen_index += (*it)->lineCount();
        }
    }
    QGuiApplication::clipboard()->setText(to_clip_board_buffer, mode);
}

const SelectionRange ScreenData::getDoubleClickSelectionRange(size_t character, size_t line)
{
    if (line < m_scrollback->height())
        return m_scrollback->getDoubleClickSelectionRange(character, line);
    size_t screen_line = line - m_scrollback->height();
    auto it = it_for_row(screen_line);
    if (it != m_screen_blocks.end())
        return Selection::getDoubleClickRange(it, character, line, m_width);
    return { QPoint(), QPoint() };
}

const CursorDiff ScreenData::modify(const QPoint &point, const QString &text, const TextStyle &style, bool replace, bool only_latin)
{
    auto it = it_for_row(point.y());
    if (it == m_screen_blocks.end())
        return { 0, 0 };
    Block *block = *it;
    const int start_char = (point.y() - block->screenIndex()) * m_width + point.x();
    const size_t lines_before = block->lineCount();
    const int lines_changed =
        block->lineCountAfterModified(start_char, text.size(), replace)  - lines_before;
    const size_t old_content_height = contentHeight();
    m_height += lines_changed;
    if (lines_changed > 0) {
        int removed = 0;
        auto to_merge_inn = it;
        ++to_merge_inn;
        while(removed < lines_changed && to_merge_inn != m_screen_blocks.end()) {
            Block *to_be_reduced = *to_merge_inn;
            bool remove_block = removed + to_be_reduced->lineCount() <= lines_changed;
            int lines_to_remove = remove_block ? to_be_reduced->lineCount() : to_be_reduced->lineCount() - (lines_changed - removed);
            block->moveLinesFromBlock(to_be_reduced, 0, lines_to_remove);
            removed += lines_to_remove;
            if (remove_block) {
                delete to_be_reduced;
                to_merge_inn = m_screen_blocks.erase(to_merge_inn);
                m_block_count--;
            } else {
                ++to_merge_inn;
            }
        }

        m_height -= removed;
    }

    if (m_height > m_screen_height)
        push_at_most_to_scrollback(m_height - m_screen_height);

    if (replace) {
        block->replaceAtPos(start_char, text, style, only_latin);
    } else {
        block->insertAtPos(start_char, text, style, only_latin);
    }
    int end_char = (start_char + text.size()) % m_width;
    if (end_char == 0)
        end_char = m_width -1;

    int end_line = (start_char + text.size()) / m_width;
    int line_diff = end_line - (start_char / m_width);
    emit contentModified(m_scrollback->height() + point.y(), lines_changed, content_height_diff(old_content_height));
    return { line_diff, end_char - point.x()};
}

void ScreenData::clearBlock(std::list<Block *>::iterator line)
{
    int before_count = (*line)->lineCount();
    (*line)->clear();
    int diff_line = before_count - (*line)->lineCount();
    if (diff_line > 0) {
        ++line;
        for (int i = 0; i < diff_line; i++) {
            m_screen_blocks.insert(line, new Block(m_screen));
        }
        m_block_count+=diff_line;
    }
}

std::list<Block *>::iterator ScreenData::it_for_row_ensure_single_line_block(int row)
{
    auto it = it_for_row(row);
    if (it == m_screen_blocks.end())
        return it;
    const int index = (*it)->screenIndex();
    const int lines = (*it)->lineCount();

    if (index == row && lines == 1) {
        return it;
    }

    int line_diff = row - index;
    return split_out_row_from_block(it, line_diff);
}

std::list<Block *>::iterator ScreenData::split_out_row_from_block(std::list<Block *>::iterator it, int row_in_block)
{
    int lines = (*it)->lineCount();

    if (row_in_block == 0 && lines == 1)
        return it;

    if (row_in_block == 0) {
        auto insert_before = (*it)->takeLine(0);
        insert_before->setScreenIndex(row_in_block);
        m_block_count++;
        return m_screen_blocks.insert(it,insert_before);
    } else if (row_in_block == lines -1) {
        auto insert_after = (*it)->takeLine(lines -1);
        insert_after->setScreenIndex(row_in_block);
        ++it;
        m_block_count++;
        return m_screen_blocks.insert(it, insert_after);
    }

    auto half = (*it)->split(row_in_block);
    ++it;
    auto it_width_first = m_screen_blocks.insert(it, half);
    auto the_one = half->takeLine(0);
    m_block_count+=2;
    return m_screen_blocks.insert(it_width_first,the_one);
}

int ScreenData::push_at_most_to_scrollback(int lines)
{
    if (lines >= m_height)
        lines = m_height - 1;
    int pushed = 0;
    auto it = m_screen_blocks.begin();
    while (it != m_screen_blocks.end() && pushed + (*it)->lineCount() <= lines) {
        m_block_count--;
        const int block_height = (*it)->lineCount();
        m_height -= block_height;
        pushed += block_height;
        m_scrollback->addBlock(*it);
        it = m_screen_blocks.erase(it);
    }
    return pushed;
}

int ScreenData::reclaim_at_least(int lines)
{
    int lines_reclaimed = 0;
    while (m_scrollback->blockCount() && lines_reclaimed < lines) {
        Block *block = m_scrollback->reclaimBlock();
        m_height += block->lineCount();
        lines_reclaimed += block->lineCount();
        m_block_count++;
        m_screen_blocks.push_front(block);
    }
    return lines_reclaimed;
}

int ScreenData::remove_lines_from_end(int lines)
{
    int removed = 0;
    auto it = m_screen_blocks.end();
    while (it != m_screen_blocks.begin() && removed < lines) {
        --it;
        const int block_height = (*it)->lineCount();
        if (removed + block_height <= lines) {
            removed += block_height;
            m_height -= block_height;
            m_block_count--;
            delete (*it);
            it = m_screen_blocks.erase(it);
        } else {
            const int to_remove = lines - removed;
            removed += to_remove;
            m_height -= to_remove;
            Block *block = *it;
            for (int i = 0; i < to_remove; i++) {
                block->removeLine(block->lineCount()-1);
            }
        }
    }
    return removed;
}

int ScreenData::ensure_at_least_height(int height)
{
    if (m_height > height)
        return 0;

    int to_grow = height - m_height;
    int reclaimed = reclaim_at_least(to_grow);

    if (height > m_height) {
        // Need to grow more than scrollback allows.
        int to_insert = height - m_height;
        for (int i = 0; i < to_insert; i++) {
            m_screen_blocks.push_back(new Block(m_screen));
        }
        qCDebug(lcScreenData) << "Inserted " << to_insert << "new blocks";
        m_height += to_insert;
        m_block_count += to_insert;

        // We are only interested in how much *scrollback* was reclaimed. Buffer
        // growth is not of interest.
        return reclaimed;
    } else {
        // We grew by "to_grow" lines, although a block may be more than this in
        // height in the case of a multiline block.
        return to_grow;
    }
}

int ScreenData::content_height_diff(size_t old_content_height)
{
    const size_t content_height = contentHeight();
    return old_content_height < content_height ? content_height - old_content_height :
        - int(old_content_height - content_height);
}
