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

#ifndef SCROLLBACK_H
#define SCROLLBACK_H

#include "selection.h"

#include <list>

#include <QtCore/qglobal.h>
#include <QtCore/QPoint>

class Block;

class Scrollback
{
public:
    Scrollback(size_t max_size);

    void addBlock(Block *block);
    Block *reclaimBlock();
    void ensureVisibleLines(int screenHeight, int top_line);
    void fixupVisibility(int screenHeight);

    size_t height() const;

    void setWidth(int screenHeight, int width);

    size_t blockCount() { return m_block_count; }

    QString selection(const QPoint &start, const QPoint &end) const;
    const SelectionRange getDoubleClickSelectionRange(size_t character, size_t line);
private:
    std::list<Block *>::iterator findIteratorForLine(size_t line);

    std::list<Block *> m_blocks;
    size_t m_height;
    size_t m_width;
    size_t m_block_count;
    size_t m_max_size;
    int m_firstVisibleLine;
};

#endif //SCROLLBACK_H
