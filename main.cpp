

#include <QApplication>
#include <QSurfaceFormat>

#include <qmplayer/qmplay.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    QSurfaceFormat fmt;
//    fmt.setDepthBufferSize(24);


//    fmt.setVersion(3, 3);
//    fmt.setProfile(QSurfaceFormat::CoreProfile);

//    QSurfaceFormat::setDefaultFormat(fmt);

    QmPlay w;


    w.show();
    return a.exec();
}
