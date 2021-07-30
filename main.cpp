

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

    quint16 aa = 789;
    quint8 p[2];
    memcpy(&p,&aa,sizeof(16));
    w.show();
    return a.exec();
}
