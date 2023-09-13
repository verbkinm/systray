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
    explicit System_Tray(QObject *parent = nullptr);
    ~System_Tray();

private:
    void createActions();
    void createTrayIcons();

    QColor backgroundTemperature() const;
    QColor backgroundActiveMem() const;

    QString temperature() const;
    float toPer(size_t val) const;

    QAction *quitAction,
            *propertyTemperature, *propertyMemory,
            *showMessageTemperature, *showMessageMemory;

    QSystemTrayIcon *trayIconTemperature, *trayIconMemory;
    QMenu *trayIconTemperatureMenu, *trayIconMemoryMenu;

    QTimer *_timer;

    int _temperature, _pageSize, _pageCount;
    size_t _totalMem, _activeMem, _inactiveMem, _laundryMem, _wiredMem, _freeMem;

public slots:
    void setIcon();
    void showTemperatureMessage() const;
    void showFreeMemMessage() const;

    void slotTimerOut();
};

#endif // QT_NO_SYSTEMTRAYICON

#endif // SYSTEMTRAY
