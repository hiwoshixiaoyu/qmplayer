#ifndef QMPLAY_H
#define QMPLAY_H

#include <QMainWindow>
#include <QTimer>
#include <tester.h>

#include <ffplay/readthread.h>



QT_BEGIN_NAMESPACE
namespace Ui { class QmPlay; }
QT_END_NAMESPACE


class QmPlay : public QMainWindow
{
    Q_OBJECT

public:
    QmPlay(QWidget *parent = nullptr);
    ~QmPlay();

private slots:
    //void on_btntester_clicked();



    void RefreshVideo();
    void on_btnplay_clicked();

private:
    Ui::QmPlay *ui;
    Tester t;

    ReadThread *m_read;
    QTimer m_timer;

};
#endif // QMPLAY_H
