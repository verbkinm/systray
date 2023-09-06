#include "systray.h"
#include <fstream>

#ifdef Q_OS_FREEBSD
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifndef QT_NO_SYSTEMTRAYICON

System_Tray::System_Tray(QObject *parent) : QObject(parent)
{
    createActions();
    createTrayIcons();

    slotTimerOut();

    trayIconTemperature->show();
    trayIconFreeMemory->show();

    _timer = new QTimer(this);
    _timer->setInterval(2000);

    connect(_timer, &QTimer::timeout, this, &System_Tray::slotTimerOut);
    _timer->start();
}

System_Tray::~System_Tray()
{
    delete trayIconMenu;
}

void System_Tray::setIcon()
{
    QRect rect (0, 0, 22, 22);
    QPixmap pix(rect.size());
    QPainter painter(&pix);
    QFont font = painter.font();
    font.setPixelSize(14);
    painter.setFont(font);

    painter.fillRect(rect, backgroundTemperature());
    painter.drawText(rect, Qt::AlignCenter, temperature());

    trayIconTemperature->setIcon(pix);
    trayIconTemperature->setToolTip("Температура процессора");

    painter.fillRect(rect, backgroundFreeMem());
    painter.drawText(rect, Qt::AlignCenter, freeMemory());
    trayIconFreeMemory->setIcon(pix);
    trayIconFreeMemory->setToolTip(QString("Всего ОЗУ: %1 МБ.\n"
                                           "Свободно: %2 %").arg(totalMemory() / 1024 / 1024).arg(_freeMemPer));
}

void System_Tray::showTemperatureMessage() const
{
    trayIconTemperature->showMessage("Внимание", "Высокий уровень температуры процессора", QSystemTrayIcon::Warning, 2000);
}

void System_Tray::showFreeMemMessage() const
{
    trayIconFreeMemory->showMessage("Внимание", "Мало свободной ОЗУ", QSystemTrayIcon::Warning, 2000);
}

void System_Tray::slotTimerOut()
{
#ifdef Q_OS_LINUX
    std::ifstream file("//sys/devices/platform/coretemp.0/hwmon/hwmon4/temp1_input");
    if (!file.is_open())
    {
        _temperature = -240;
        return;
    }

    file >> _temperature;
    _temperature /= 1000;
#endif
#ifdef Q_OS_FREEBSD
    size_t len = sizeof(int);
    int ncpu = 0;

    const char *ncpu_str = "hw.ncpu";
    sysctlbyname(ncpu_str, &ncpu, &len, NULL, 0);

    int t = 0;
    _temperature = 0;
    for (int i = 0; i < ncpu; ++i)
    {
        std::string temp = "dev.cpu." + std::to_string(i) + ".temperature";
        sysctlbyname(temp.c_str(), &t, &len, NULL, 0);

        t = (t - 2731) / 10;
        _temperature += t;
    }
    _temperature /= ncpu;

    _freeMemPer = 0;

    int page_size;
    len = sizeof(page_size);
    sysctlbyname("vm.stats.vm.v_page_size", &page_size, &len, NULL, 0);

    int page_count;
    len = sizeof(page_count);
    sysctlbyname("vm.stats.vm.v_page_count", &page_count, &len, NULL, 0);

    size_t total_memory = page_count * page_size;

    int free_count;
    len = sizeof(free_count);
    sysctlbyname("vm.stats.vm.v_free_count", &free_count, &len, NULL, 0);

    size_t free_memory = free_count * page_size;

    _freeMemPer = free_memory / (total_memory / 100);
#endif

    if (_temperature >= 90)
        showTemperatureMessage();

//    if (_freeMemPer <= 10)
//        showFreeMemMessage();

    setIcon();
}

void System_Tray::createActions()
{
    quitAction = new QAction("Выход", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void System_Tray::createTrayIcons()
{
    trayIconMenu = new QMenu;
    trayIconMenu->addAction(quitAction);

    trayIconTemperature = new QSystemTrayIcon(this);
    trayIconTemperature->setContextMenu(trayIconMenu);

    trayIconFreeMemory = new QSystemTrayIcon(this);
    trayIconFreeMemory->setContextMenu(trayIconMenu);
}

QColor System_Tray::backgroundTemperature() const
{
    if (_temperature >= 0 && _temperature <= 40)
        return Qt::green;
    if (_temperature <= 60)
        return Qt::yellow;
    if (_temperature <= 80)
        return qRgb(255, 165, 0);

    return Qt::red;
}

QColor System_Tray::backgroundFreeMem() const
{
    if (_freeMemPer >= 60)
        return Qt::green;
    if (_freeMemPer >= 40)
        return Qt::yellow;
    if (_freeMemPer >= 20)
        return qRgb(255, 165, 0);

    return Qt::red;
}

QString System_Tray::temperature() const
{
    if (_temperature <= -100 || _temperature > 100)
        return "err";

    return QString::number(_temperature);
}

QString System_Tray::freeMemory() const
{
    return QString::number(_freeMemPer);
}

uint64_t System_Tray::totalMemory() const
{
#ifdef Q_OS_FREEBSD
    int page_size;
    size_t len = sizeof(page_size);
    sysctlbyname("vm.stats.vm.v_page_size", &page_size, &len, NULL, 0);

    int page_count;
    len = sizeof(page_count);
    sysctlbyname("vm.stats.vm.v_page_count", &page_count, &len, NULL, 0);

    return page_size * page_count;
#endif

    return 0;
}

#endif
