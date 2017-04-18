# about

literm is a terminal emulator for Linux first and foremost, but it is also
usable elsewhere (on Mac). The design goal is to be simplistic while still
providing a reasonable amount of features.

If you'd like to talk to people using literm, feel free to pop onto #literm on
freenode.

# technology

literm is implemented using QML to provide a fast, and fluid user interface.
The terminal emulator side is in C++ (also using Qt). It is exposed as a plugin
to allow reuse in other applications or contexts.

# building

    ./configure && make

.. should get you most of the way there, assuming you have qmake & Qt easily
available. After that, use:

    qmlscene -I imports yat_app/main.qml

... to run the application.

# history

literm started off life as [Yat, a terminal emulator by Jørgen
Lind](https://github.com/jorgen/yat). I decided to take it a bit further to do
what I wanted it to do on the UI end, and fix some outstanding problems.

