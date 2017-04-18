/*******************************************************************************
* Copyright (c) 2013 Jørgen Lind
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
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

import QtQuick 2.5

import Yat 1.0 as Yat

Yat.TerminalScreen {
    id: screenItem

    property string title: screen.title
    property real fontWidth: fontMetricText.paintedWidth
    property real fontHeight: fontMetricText.paintedHeight

    anchors.fill: parent
    focus: true

    Component {
        id: textComponent
        Yat.Text {
        }
    }
    Component {
        id: cursorComponent
        Yat.Cursor {
        }
    }
    Shortcut {
        sequence: StandardKey.Copy
        onActivated: screen.selection.sendToClipboard()
    }
    Shortcut {
        sequence: StandardKey.Paste
        onActivated: screen.selection.pasteFromClipboard()
    }

    onActiveFocusChanged: {
        if (activeFocus) {
            Qt.inputMethod.show();
        }
    }

    Keys.onPressed: {
        if (event.text === "?") {
            screen.printScreen()
        }
        screen.sendKey(event.text, event.key, event.modifiers);
    }

    Text {
        id: fontMetricText
        text: "B"
        font: parent.screen.font
        visible: false
        textFormat: Text.PlainText
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: screen.defaultBackgroundColor
    }
    Flickable {
        id: flickable
        anchors.top: parent.top
        anchors.left: parent.left
        contentWidth: width
        contentHeight: textContainer.height
        interactive: true
        flickableDirection: Flickable.VerticalFlick
        contentY: ((screen.contentHeight - screen.height) * screenItem.fontHeight)

        Item {
            id: textContainer
            width: parent.width
            height: screen.contentHeight * screenItem.fontHeight

            Selection {
                characterHeight: fontHeight
                characterWidth: fontWidth
                screenWidth: screenItem.width

                startX: screen.selection.startX
                startY: screen.selection.startY

                endX: screen.selection.endX
                endY: screen.selection.endY

                visible: screen.selection.enable
            }
        }

        Item {
            id: cursorContainer
            width: textContainer.width
            height: textContainer.height
        }


        onContentYChanged: {
            if (!atYEnd) {
                var top_line = Math.floor(Math.max(contentY,0) / screenItem.fontHeight);
                screen.ensureVisibleLines(top_line);
            }
        }
    }

    Connections {
        id: connections

        target: screen

        onFlash: {
            flashAnimation.start()
        }

        onReset: {
            resetScreenItems();
        }

        onTextCreated: {
            var textSegment = textComponent.createObject(screenItem,
                {
                    "parent" : textContainer,
                    "objectHandle" : text,
                    "font" : screenItem.screen.font,
                    "fontWidth" : screenItem.fontWidth,
                    "fontHeight" : screenItem.fontHeight,
                });
        }

        onCursorCreated: {
            if (cursorComponent.status != Component.Ready) {
                console.log(cursorComponent.errorString());
                return;
            }
            var cursorVariable = cursorComponent.createObject(screenItem,
                {
                    "parent" : cursorContainer,
                    "objectHandle" : cursor,
                    "fontWidth" : screenItem.fontWidth,
                    "fontHeight" : screenItem.fontHeight,
                })
        }

        onRequestHeightChange: {
            setTerminalHeight(newHeight)
        }

        onRequestWidthChange: {
            setTerminalWidth(newWidth)
        }
    }

    //onFontChanged: {
    //    setTerminalHeight();
    //    setTerminalWidth();
    //}

    onWidthChanged: {
        setTerminalWidth();
    }
    onHeightChanged: {
        setTerminalHeight();
    }

    function setTerminalWidth(explicitWidth) {
        if (fontWidth > 0 || explicitWidth) {
            var pty_width = explicitWidth ? explicitWidth : Math.floor(width / fontWidth);
            flickable.width = pty_width * fontWidth;
            if (!explicitWidth)
                screen.width = pty_width;
        }
    }

    function setTerminalHeight(explicitHeight) {
        if (fontHeight > 0 || explicitHeight) {
            var pty_height = explicitHeight ? explicitHeight : Math.floor(height / fontHeight);
            flickable.height = pty_height * fontHeight;
            if (!explicitHeight)
                screen.height = pty_height;
        }
    }

    Rectangle {
        id: flash
        z: 1.2
        anchors.fill: parent
        color: "grey"
        opacity: 0
        SequentialAnimation {
            id: flashAnimation
            NumberAnimation {
                target: flash
                property: "opacity"
                to: 1
                duration: 75
            }
            NumberAnimation {
                target: flash
                property: "opacity"
                to: 0
                duration: 75
            }
        }
    }

    MouseArea {
        id:mousArea

        property int drag_start_x
        property int drag_start_y

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        onPressed: {
            if (mouse.button == Qt.LeftButton) {
                hoverEnabled = true;
                var transformed_mouse = mapToItem(textContainer, mouse.x, mouse.y);
                var character = Math.floor((transformed_mouse.x / fontWidth));
                var line = Math.floor(transformed_mouse.y / fontHeight);
                var start = Qt.point(character,line);
                drag_start_x = character;
                drag_start_y = line;
                screen.selection.startX = character;
                screen.selection.startY = line;
                screen.selection.endX = character;
                screen.selection.endY = line;
            }
        }

        onPositionChanged: {
            var transformed_mouse = mapToItem(textContainer, mouse.x, mouse.y);
            var character = Math.floor(transformed_mouse.x / fontWidth);
            var line = Math.floor(transformed_mouse.y / fontHeight);
            var current_pos = Qt.point(character,line);
            if (line < drag_start_y || (line === drag_start_y && character < drag_start_x)) {
                screen.selection.startX = character;
                screen.selection.startY = line;
                screen.selection.endX = drag_start_x;
                screen.selection.endY = drag_start_y;
            } else {
                screen.selection.startX = drag_start_x;
                screen.selection.startY = drag_start_y;
                screen.selection.endX = character;
                screen.selection.endY = line;
            }
        }

        onReleased: {
            if (mouse.button == Qt.LeftButton) {
                hoverEnabled = false;
                screen.selection.sendToSelection();
            }
        }

        onClicked: {
            if (mouse.button == Qt.MiddleButton) {
                screen.pasteFromSelection();
            }
        }

        onDoubleClicked: {
            if (mouse.button == Qt.LeftButton) {
                var transformed_mouse = mapToItem(textContainer, mouse.x, mouse.y);
                var character = Math.floor(transformed_mouse.x / fontWidth);
                var line = Math.floor(transformed_mouse.y / fontHeight);
                screen.doubleClicked(character,line);
            }
        }
    }
}
