

#include <QApplication>

#include <qmplayer/qmplay.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QmPlay w;


    w.show();
    return a.exec();
}
