#include "systray.h"
#include <fstream>
#include <sstream>

#ifdef Q_OS_FREEBSD
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifndef QT_NO_SYSTEMTRAYICON

System_Tray::System_Tray(QObject *parent) : QObject(parent)
{
    _freeMem = 0;
    _activeMem = 0;
    _inactiveMem = 0;
    _laundryMem = 0;
    _wiredMem = 0;

    // Значение не изменяются у следующих полей

    size_t len = sizeof(_pageSize);
    sysctlbyname("vm.stats.vm.v_page_size", &_pageSize, &len, NULL, 0);

    len = sizeof(_pageCount);
    sysctlbyname("vm.stats.vm.v_page_count", &_pageCount, &len, NULL, 0);

    _totalMem = _pageSize * _pageCount;

    //

    createActions();
    createTrayIcons();

    slotTimerOut();

    trayIconTemperature->show();
    trayIconMemory->show();

    _timer = new QTimer(this);
    _timer->setInterval(2000);

    connect(_timer, &QTimer::timeout, this, &System_Tray::slotTimerOut);
    _timer->start();
}

System_Tray::~System_Tray()
{
    delete trayIconTemperatureMenu;
    delete trayIconMemoryMenu;
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
    trayIconTemperature->setToolTip("Температура процессора (C°)");

    painter.fillRect(rect, backgroundActiveMem());
    painter.drawText(rect, Qt::AlignCenter, QString::number(_activeMem / (_totalMem / 100)));
    trayIconMemory->setIcon(pix);

    std::stringstream stream;
    stream << "Оперативная память (Использовано %)\n\n"
           << "Детали:\n"
           << "total: " << (_totalMem / 1024 / 1024) << " МБ (" << toPer(_totalMem) << "%)\n"
           << "active: " << (_activeMem / 1024 / 1024) << " МБ (" << toPer(_activeMem) << "%)\n"
           << "inactive: " << (_inactiveMem / 1024 / 1024) << " МБ (" << toPer(_inactiveMem) << "%)\n"
           << "laundry: " << (_laundryMem / 1024 / 1024) << " МБ (" << toPer(_laundryMem) << "%)\n"
           << "wired: " << (_wiredMem / 1024 / 1024) << " МБ (" << toPer(_wiredMem) << "%)\n"
           << "free: " << (_freeMem / 1024 / 1024) << " МБ (" << toPer(_freeMem) << "%)";

    trayIconMemory->setToolTip(stream.str().c_str());
}

void System_Tray::showTemperatureMessage() const
{
    trayIconTemperature->showMessage("Внимание", "Высокий уровень температуры процессора", QSystemTrayIcon::Warning, 2000);
}

void System_Tray::showFreeMemMessage() const
{
    trayIconMemory->showMessage("Внимание", "Мало свободной ОЗУ", QSystemTrayIcon::Warning, 2000);
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

    _freeMem = 0;
    _activeMem = 0;
    _inactiveMem = 0;
    _laundryMem = 0;
    _wiredMem = 0;

    int page_count;
    len = sizeof(page_count);
    sysctlbyname("vm.stats.vm.v_page_count", &page_count, &len, NULL, 0);

    int free_count;
    len = sizeof(free_count);
    sysctlbyname("vm.stats.vm.v_free_count", &free_count, &len, NULL, 0);

    _freeMem = free_count * _pageSize;

    len = sizeof(_activeMem);
    sysctlbyname("vm.stats.vm.v_active_count", &_activeMem, &len, NULL, 0);
    _activeMem = _activeMem * _pageSize;

    sysctlbyname("vm.stats.vm.v_inactive_count", &_inactiveMem, &len, NULL, 0);
    _inactiveMem = _inactiveMem * _pageSize;

    sysctlbyname("vm.stats.vm.v_laundry_count", &_laundryMem, &len, NULL, 0);
    _laundryMem = _laundryMem * _pageSize;

    sysctlbyname("vm.stats.vm.v_wire_count", &_wiredMem, &len, NULL, 0);
    _wiredMem = _wiredMem * _pageSize;

#endif

    if (_temperature >= 90 && showMessageTemperature->isChecked())
        showTemperatureMessage();

    if ((_activeMem / (_totalMem / 100)) >= 90 && showMessageMemory->isChecked())
        showFreeMemMessage();

    setIcon();
}

void System_Tray::createActions()
{
    quitAction = new QAction("Выход", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    propertyTemperature = new QAction("Настройки", this);
    propertyMemory = new QAction("Настройки", this);

    showMessageTemperature = new QAction("Отображать сообщения", this);
    showMessageTemperature->setCheckable(true);
    showMessageTemperature->setChecked(true);

    showMessageMemory = new QAction("Отображать сообщения", this);;
    showMessageMemory->setCheckable(true);
    showMessageMemory->setChecked(true);
}

void System_Tray::createTrayIcons()
{
    trayIconTemperatureMenu = new QMenu;
    trayIconTemperatureMenu->addAction(showMessageTemperature);
    trayIconTemperatureMenu->addAction(propertyTemperature);
    trayIconTemperatureMenu->addSeparator();
    trayIconTemperatureMenu->addAction(quitAction);

    trayIconTemperature = new QSystemTrayIcon(this);
    trayIconTemperature->setContextMenu(trayIconTemperatureMenu);

    trayIconMemoryMenu = new QMenu;
    trayIconMemoryMenu->addAction(showMessageMemory);
    trayIconMemoryMenu->addAction(propertyMemory);
    trayIconMemoryMenu->addSeparator();
    trayIconMemoryMenu->addAction(quitAction);

    trayIconMemory = new QSystemTrayIcon(this);
    trayIconMemory->setContextMenu(trayIconMemoryMenu);
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

QColor System_Tray::backgroundActiveMem() const
{
    int actMemPer = _activeMem / (_totalMem / 100);
    if (actMemPer <= 30)
        return Qt::green;
    if (actMemPer <= 50)
        return Qt::yellow;
    if (actMemPer <= 90)
        return qRgb(255, 165, 0);

    return Qt::red;
}

QString System_Tray::temperature() const
{
    if (_temperature <= -100 || _temperature > 100)
        return "err";

    return QString::number(_temperature);
}

float System_Tray::toPer(size_t val) const
{
    return val / (_totalMem / 100.0);
}

#endif
