/*******************************************************************************
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
import QtQuick.Window 2.0

import Yat 1.0 as Yat

Window {
    id: terminalWindow

    TabView {
        id: tabView
        anchors.fill: parent

        Component.onCompleted: {
            add_terminal();
            terminalWindow.show();
        }

        Component {
            id: terminalScreenComponent
            Yat.Screen {
            }
        }

        function add_terminal()
        {
            var tab = tabView.addTab("", terminalScreenComponent);
            tab.item.aboutToBeDestroyed.connect(destroy_tab);
            tabView.currentIndex = tabView.count - 1;
        }

        function destroy_tab(screenItem) {
            if (tabView.count == 1) {
                Qt.quit();
                return;
            }
            for (var i = 0; i < tabView.count; i++) {
                if (tabView.getTab(i).item === screenItem) {
                    tabView.getTab(i).item.parent = null;
                    tabView.removeTab(i);
                    break;
                }
            }
        }

        function set_current_terminal_active(index) {
            terminalWindow.color = Qt.binding(function() { return tabView.getTab(index).item.screen.defaultBackgroundColor;})
            tabView.getTab(index).item.forceActiveFocus();
        }

        onCurrentIndexChanged: {
            set_current_terminal_active(tabView.currentIndex);
        }

        // hurgh, this is a bit ugly
        Shortcut {
            sequence: "Ctrl+1"
            onActivated: {
                if (tabView.count >= 2)
                    tabView.currentIndex = 0 // yes, this is right. 0 indexed.
            }
        }
        Shortcut {
            sequence: "Ctrl+2"
            onActivated: {
                if (tabView.count >= 2)
                    tabView.currentIndex = 1 // yes, this is right. 0 indexed.
            }
        }
        Shortcut {
            sequence: "Ctrl+3"
            onActivated: {
                if (tabView.count >= 3)
                    tabView.currentIndex = 2 // yes, this is right. 0 indexed.
            }
        }
        Shortcut {
            sequence: "Ctrl+4"
            onActivated: {
                if (tabView.count >= 4)
                    tabView.currentIndex = 3 // yes, this is right. 0 indexed.
            }
        }
        Shortcut {
            sequence: "Ctrl+5"
            onActivated: {
                if (tabView.count >= 5)
                    tabView.currentIndex = 4 // yes, this is right. 0 indexed.
            }
        }
        Shortcut {
            sequence: "Ctrl+6"
            onActivated: {
                if (tabView.count >= 6)
                    tabView.currentIndex = 5 // yes, this is right. 0 indexed.
            }
        }
        Shortcut {
            sequence: "Ctrl+7"
            onActivated: {
                if (tabView.count >= 7)
                    tabView.currentIndex = 6 // yes, this is right. 0 indexed.
            }
        }
        Shortcut {
            sequence: "Ctrl+8"
            onActivated: {
                if (tabView.count >= 8)
                    tabView.currentIndex = 7 // yes, this is right. 0 indexed.
            }
        }
        Shortcut {
            sequence: "Ctrl+9"
            onActivated: {
                if (tabView.count >= 9)
                    tabView.currentIndex = 8 // yes, this is right. 0 indexed.
            }
        }
        Shortcut {
            sequence: "Ctrl+0"
            onActivated: {
                if (tabView.count >= 10)
                    tabView.currentIndex = 9 // yes, this is right. 0 indexed.
            }
        }
        Shortcut {
            sequence: "Ctrl+T"
            onActivated: {
                tabView.add_terminal();
            }
        }
        Shortcut {
            sequence: "Ctrl+W"
            onActivated: {
                tabView.destroy_tab(tabView.getTab(tabView.currentIndex).item)
            }
        }
        Shortcut {
            sequence: "Ctrl+Shift+]"
            onActivated: {
                tabView.currentIndex = (tabView.currentIndex + 1) % tabView.count;
            }
        }
        Shortcut {
            sequence: "Ctrl+Shift+["
            onActivated: {
                if (tabView.currentIndex > 0) {
                    tabView.currentIndex--;
                } else {
                    tabView.currentIndex = tabView.count -1;
                }
            }
        }
    }

    width: 800
    height: 600
}
