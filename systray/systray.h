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
    QColor backgroundGPUTemperature() const;
    QColor backgroundActiveMem() const;

    QString temperature(int t) const;
    float toPer(size_t val) const;

    QAction *_quitAction,
            *_propertyTemperature, *_propertyMemory, *_propertyGPUTemperature,
            *_showMessageTemperature, *_showMessageGPUTemperature, *_showMessageMemory;

    QSystemTrayIcon *_trayIconTemperature, *_trayIconMemory, *_trayIconGPUTemperature;
    QMenu *_trayIconTemperatureMenu, *_trayIconMemoryMenu, *_trayIconGPUTemperatureMenu;

    QTimer *_timer;

    int _temperature, _pageSize, _pageCount, _ncpu, _temperatureGPU;
    std::vector<int> _tempeCores;
    size_t _totalMem, _activeMem, _inactiveMem, _laundryMem, _wiredMem, _freeMem;

public slots:
    void setIcon();
    void showTemperatureMessage() const;
    void showFreeMemMessage() const;

    void slotTimerOut();
};

#endif // QT_NO_SYSTEMTRAYICON

#endif // SYSTEMTRAY
