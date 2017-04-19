# about

literm is a terminal emulator for Linux first and foremost, but it is also
usable elsewhere (on Mac). The design goal is to be simplistic while still
providing a reasonable amount of features.

If you'd like to talk to people using literm, feel free to pop onto #literm on
freenode.

# status

I wouldn't use this without a dose of caution on a regular basis yet. There are
still crashes triggerable through escape codes (see
[this bug](https://github.com/rburchell/literm/issues/11) for efforts ongoing to
improve that situation), and a myriad of other issues, not to mention a dire
lack of features.

This having been said, feel free to give it a shot and file bugs - ideally with
pull requests, but at the least with as much information about how to reproduce
them as possible.

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

