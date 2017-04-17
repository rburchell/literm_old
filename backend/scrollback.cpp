/******************************************************************************
 * Copyright (C) 2017 Robin Burchell <robin+git@viroteck.net>
 * Copyright (c) 2013 Jørgen Lind
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

#include "scrollback.h"

#include "screen_data.h"
#include "screen.h"
#include "block.h"

#include <QtCore/QLoggingCategory>

Q_LOGGING_CATEGORY(lcScrollback, "yat.scrollback", QtDebugMsg)

Scrollback::Scrollback(size_t max_size, ScreenData *screen_data)
    : m_screen_data(screen_data)
    , m_height(0)
    , m_width(0)
    , m_block_count(0)
    , m_max_size(max_size)
    , m_firstVisibleLine(0)
{
}

void Scrollback::addBlock(Block *block)
{
    if (!m_max_size) {
        delete block;
        return;
    }

    qCDebug(lcScrollback) << "Adding block " << block;
    m_blocks.push_back(block);
    block->releaseTextObjects();
    m_block_count++;
    m_height += m_blocks.back()->lineCount();

    while (m_blocks.front() != block && m_height - m_blocks.front()->lineCount() >= m_max_size) {
        qCDebug(lcScrollback) << "Popping excess block " << block;
        m_block_count--;
        m_height -= std::min(m_blocks.front()->lineCount(), (int)m_height);
        delete m_blocks.front();
        m_blocks.pop_front();
    }
}

Block *Scrollback::reclaimBlock()
{
    if (m_blocks.empty())
        return nullptr;

    Block *last = m_blocks.back();
    qCDebug(lcScrollback) << "Reclaiming block " << last;
    last->setWidth(m_width);
    m_block_count--;
    m_height -= last->lineCount();
    m_blocks.pop_back();

    return last;
}

// Make sure everything from top_line down one screen is visible.
void Scrollback::ensureVisibleLines(int top_line)
{
    if (top_line < 0)
        return;
    if (size_t(top_line) >= m_height)
        return;

    // Hide old lines.
    // TODO: optimize to *only* hide the range that top_line -> top_line + height &
    // m_firstVisibleLine -> m_firstVisibleLine + height do not intersect.
    std::list<Block*>::iterator it = findIteratorForLine(m_firstVisibleLine);
    int lastVisibleLine = m_firstVisibleLine + m_screen_data->screen()->height();
    int line_no = m_firstVisibleLine;
    while (it != m_blocks.end() && line_no <= lastVisibleLine) {
        Block *b = *it;
        qCDebug(lcScrollback) << "Releasing scrollback block starting " << line_no;
        b->releaseTextObjects();
        line_no += b->lineCount();
    }

    m_firstVisibleLine = top_line;
    fixupVisibility();
}

// Fix line numbers for blocks in the viewport.
void Scrollback::fixupVisibility()
{
    std::list<Block*>::iterator it = findIteratorForLine(m_firstVisibleLine);
    int lastVisibleLine = m_firstVisibleLine + m_screen_data->screen()->height();
    int line_no = m_firstVisibleLine;
    while (it != m_blocks.end() && line_no <= lastVisibleLine) {
        qCDebug(lcScrollback) << "Showing scrollback block starting " << line_no;
        Block *b = *it;
        b->setLine(line_no);
        b->dispatchEvents();
        line_no += b->lineCount();
        it++;
    }
}

size_t Scrollback::height() const
{
    return m_height;
}

void Scrollback::setWidth(int width)
{
    m_width = width;
    m_height = 0;

    // Update length of all blocks so line numbers are correct.
    for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it) {
        Block *b = *it;
        b->setWidth(m_width);
        m_height += b->lineCount();
    }

    // And make sure the blocks visible are correct.
    fixupVisibility();
}

QString Scrollback::selection(const QPoint &start, const QPoint &end) const
{
    Q_ASSERT(start.y() >= 0);
    Q_ASSERT(end.y() >= 0);
    Q_ASSERT(size_t(end.y()) < m_height);
    QString return_string;

    size_t current_line = m_height;
    auto it = m_blocks.end();

    bool should_continue = true;
    while (it != m_blocks.begin() && should_continue) {
        --it;
        const int block_height = (*it)->lineCount();
        current_line -= block_height;
        if (current_line > size_t(end.y())) {
            continue;
        }

        int end_pos = (*it)->textSize();
        if (current_line <= size_t(end.y()) && current_line + block_height >= size_t(end.y())) {
            size_t end_line_count = end.y() - current_line;
            end_pos = end_line_count * m_width + end.x();
        }
        int start_pos = 0;
        if (current_line <= size_t(start.y()) && current_line + block_height >= size_t(start.y())) {
            size_t start_line_count = start.y() - current_line;
            start_pos = start_line_count * m_width + start.x();
            should_continue = false;
        } else if (current_line + block_height < size_t(start.y())) {
            should_continue = false;
        }
        return_string.prepend((*it)->textLine().mid(start_pos, end_pos));
        if (should_continue)
            return_string.prepend(QChar('\n'));
    }

    return return_string;
}

std::list<Block *>::iterator Scrollback::findIteratorForLine(size_t line)
{
   size_t current_line = m_height;
    auto it = m_blocks.end();
    while (it != m_blocks.begin()) {
        --it;
        current_line -= (*it)->lineCount();
        if (current_line <= line)
            return it;
    }
    return m_blocks.end();
}

const SelectionRange Scrollback::getDoubleClickSelectionRange(size_t character, size_t line)
{
    auto it = findIteratorForLine(line);
    if (it != m_blocks.end())
        return Selection::getDoubleClickRange(it, character,line, m_width);
    return { QPoint(), QPoint() };
}
