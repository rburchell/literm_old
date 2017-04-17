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
#include <QtGui/QGuiApplication>
#include <QtCore/QLoggingCategory>

#include <QtCore/QDebug>

#include <float.h>
#include <cmath>

Q_LOGGING_CATEGORY(lcKeyboard, "yat.keyboard", QtDebugMsg)

static bool hasControll(Qt::KeyboardModifiers modifiers)
{
#ifdef Q_OS_MAC
    return modifiers & Qt::MetaModifier;
#else
    return modifiers & Qt::ControlModifier;
#endif
}

static bool hasMeta(Qt::KeyboardModifiers modifiers)
{
#ifdef Q_OS_MAC
    return modifiers & Qt::ControlModifier;
#else
    return modifiers & Qt::MetaModifier;
#endif
}

void Screen::sendKey(const QString &text, Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    qCDebug(lcKeyboard) << text << key << modifiers;
//    if (key == Qt::Key_Control)
//        printScreen();
    /// UGH, this function should be re-written
    char escape = '\0';
    char  control = '\0';
    char  code = '\0';
    QVector<ushort> parameters;
    bool found = true;

    switch(key) {
    case Qt::Key_Up:
        escape = C0::ESC;
        if (m_application_cursor_key_mode)
            control = C1_7bit::SS3;
        else
            control = C1_7bit::CSI;

        code = 'A';
        break;
    case Qt::Key_Right:
        escape = C0::ESC;
        if (m_application_cursor_key_mode)
            control = C1_7bit::SS3;
        else
            control = C1_7bit::CSI;

        code = 'C';
        break;
    case Qt::Key_Down:
        escape = C0::ESC;
        if (m_application_cursor_key_mode)
            control = C1_7bit::SS3;
        else
            control = C1_7bit::CSI;

            code = 'B';
        break;
    case Qt::Key_Left:
        escape = C0::ESC;
        if (m_application_cursor_key_mode)
            control = C1_7bit::SS3;
        else
            control = C1_7bit::CSI;

        code = 'D';
        break;
    case Qt::Key_Insert:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(2);
        code = '~';
        break;
    case Qt::Key_Delete:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(3);
        code = '~';
        break;
    case Qt::Key_Home:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(1);
        code = '~';
        break;
    case Qt::Key_End:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(4);
        code = '~';
        break;
    case Qt::Key_PageUp:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(5);
        code = '~';
        break;
    case Qt::Key_PageDown:
        escape = C0::ESC;
        control = C1_7bit::CSI;
        parameters.append(6);
        code = '~';
        break;
    case Qt::Key_F1:
    case Qt::Key_F2:
    case Qt::Key_F3:
    case Qt::Key_F4:
        if (m_application_cursor_key_mode) {
            parameters.append((key & 0xff) - 37);
            escape = C0::ESC;
            control = C1_7bit::CSI;
            code = '~';
        }
        break;
    case Qt::Key_F5:
    case Qt::Key_F6:
    case Qt::Key_F7:
    case Qt::Key_F8:
    case Qt::Key_F9:
    case Qt::Key_F10:
    case Qt::Key_F11:
    case Qt::Key_F12:
        if (m_application_cursor_key_mode) {
            parameters.append((key & 0xff) - 36);
            escape = C0::ESC;
            control = C1_7bit::CSI;
            code = '~';
        }
        break;
    case Qt::Key_Control:
    case Qt::Key_Shift:
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
        return;
        break;
    default:
        found = false;
    }

    if (found) {
        int term_mods = 0;
        if (modifiers & Qt::ShiftModifier)
            term_mods |= 1;
        if (modifiers & Qt::AltModifier)
            term_mods |= 2;
        if (modifiers & Qt::ControlModifier)
            term_mods |= 4;

        qCDebug(lcKeyboard) << "Found; modifiers: " << term_mods;
        QByteArray toPty;

        if (term_mods) {
            term_mods++;
            parameters.append(term_mods);
        }
        if (escape)
            toPty.append(escape);
        if (control)
            toPty.append(control);
        if (parameters.size()) {
            for (int i = 0; i < parameters.size(); i++) {
                if (i)
                    toPty.append(';');
                toPty.append(QByteArray::number(parameters.at(i)));
            }
        }
        if (code)
            toPty.append(code);
        m_pty.write(toPty);

    } else {
        QString verifiedText = text.simplified();
        qCDebug(lcKeyboard) << "Not found; simplified text: " << verifiedText;
        if (verifiedText.isEmpty()) {
            switch (key) {
            case Qt::Key_Return:
            case Qt::Key_Enter:
                verifiedText = "\r";
                break;
            case Qt::Key_Backspace:
                verifiedText = "\010";
                break;
            case Qt::Key_Tab:
                verifiedText = "\t";
                break;
            case Qt::Key_Control:
            case Qt::Key_Meta:
            case Qt::Key_Alt:
            case Qt::Key_Shift:
                return;
            case Qt::Key_Space:
                verifiedText = " ";
                break;
            default:
                verifiedText = QChar(key);
                break;
            }
        }
        QByteArray to_pty;
        QByteArray key_text;
        if (hasControll(modifiers)) {
            char key_char = verifiedText.toLocal8Bit().at(0);
            key_text.append(key_char & 0x1F);
            qCDebug(lcKeyboard) << "hasControl ON, key_text " << key_text;
        } else {
            key_text = verifiedText.toUtf8();
            qCDebug(lcKeyboard) << "hasControl OFF, key_text " << key_text;
        }

        if (modifiers &  Qt::AltModifier) {
            qCDebug(lcKeyboard) << "AltModifier ON";
            to_pty.append(C0::ESC);
        }

        if (hasMeta(modifiers)) {
            qCDebug(lcKeyboard) << "hasMeta ON";
            to_pty.append(C0::ESC);
            to_pty.append('@');
            to_pty.append(FinalBytesNoIntermediate::Reserved3);
        }

        to_pty.append(key_text);
        qCDebug(lcKeyboard) << "Writing " << to_pty;
        m_pty.write(to_pty);
    }
}


