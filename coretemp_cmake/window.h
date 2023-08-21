#ifndef WINDOW_H
#define WINDOW_H

#include <QSystemTrayIcon>
#include <QAction>
#include <QCoreApplication>
#include <QMenu>
#include <QPainter>
#include <QTimer>
//#include <QProcess>

#ifndef QT_NO_SYSTEMTRAYICON

class CoreTemp : public QObject
{
    Q_OBJECT

public:
    CoreTemp(QObject *parent = nullptr);
    ~CoreTemp();

private:
    void createActions();
    void createTrayIcon();
    QColor background();
    QString temperature();

    QAction *quitAction;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;

    QTimer *_timer;

    int _temperature;

public slots:
    void setIcon();
    void showMessage();

    void slotTimerOut();
};

#endif // QT_NO_SYSTEMTRAYICON

#endif
