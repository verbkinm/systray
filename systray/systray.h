#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QSystemTrayIcon>
#include <QAction>
#include <QCoreApplication>
#include <QMenu>
#include <QPainter>
#include <QTimer>

#ifndef QT_NO_SYSTEMTRAYICON

class System_Tray : public QObject
{
    Q_OBJECT

public:
    System_Tray(QObject *parent = nullptr);
    ~System_Tray();

private:
    void createActions();
    void createTrayIcons();
    QColor backgroundTemperature() const;
    QColor backgroundFreeMem() const;
    QString temperature() const;
    QString freeMemory() const;

    uint64_t totalMemory() const;

    QAction *quitAction;

    QSystemTrayIcon *trayIconTemperature, *trayIconFreeMemory;
    QMenu *trayIconMenu;

    QTimer *_timer;

    int _temperature, _freeMemPer;

public slots:
    void setIcon();
    void showTemperatureMessage() const;
    void showFreeMemMessage() const;

    void slotTimerOut();
};

#endif // QT_NO_SYSTEMTRAYICON

#endif // SYSTEMTRAY
