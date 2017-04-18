#include "../../../backend/block.h"
#include <QtTest/QtTest>

#include "../../../backend/screen.h"
#include "../../../backend/screen_data.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    // Deliberately not creating a QApp instance, because that makes fuzzing a
    // slow and painful process.
    Screen s;
    Parser p(&s);

    if (argc == 1) {
        char buf[1024 * 4];
        int byteCount = read(STDIN_FILENO, buf, sizeof(buf));
        p.addData(QByteArray::fromRawData(buf, byteCount));
    } else {
        QFile f(argv[1]);
        f.open(QIODevice::ReadOnly);
        p.addData(f.readAll());

    }
}

