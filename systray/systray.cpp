#include "systray.h"
#include <fstream>
#include <sstream>
#include <iostream>

#ifdef Q_OS_FREEBSD
#include <QProcess>
#include <sys/types.h>
#include <sys/sysctl.h>
#define NVIDIA_SMI "/usr/local/bin/nvidia-smi"
#endif

#ifndef QT_NO_SYSTEMTRAYICON

System_Tray::System_Tray(QObject *parent) : QObject(parent), _trayIconGPUTemperatureMenu(nullptr)
{
    _freeMem = 0;
    _activeMem = 0;
    _inactiveMem = 0;
    _laundryMem = 0;
    _wiredMem = 0;
    _temperatureGPU = 0;

    // Значение не изменяются у следующих полей

    size_t len = sizeof(_pageSize);
    sysctlbyname("vm.stats.vm.v_page_size", &_pageSize, &len, NULL, 0);

    len = sizeof(_pageCount);
    sysctlbyname("vm.stats.vm.v_page_count", &_pageCount, &len, NULL, 0);

    _totalMem = _pageSize * _pageCount;

    len = sizeof(_ncpu);
    sysctlbyname("hw.ncpu", &_ncpu, &len, NULL, 0);

    _tempeCores.resize(_ncpu);
    _tempeCores.shrink_to_fit();

    //

    createActions();
    createTrayIcons();

    slotTimerOut();

    _trayIconTemperature->show();
    _trayIconMemory->show();

    if (_trayIconGPUTemperature != nullptr)
        _trayIconGPUTemperature->show();

    _timer = new QTimer(this);
    _timer->setInterval(2000);

    connect(_timer, &QTimer::timeout, this, &System_Tray::slotTimerOut);
    _timer->start();
}

System_Tray::~System_Tray()
{
    delete _trayIconTemperatureMenu;
    delete _trayIconMemoryMenu;

    if (_trayIconGPUTemperatureMenu != nullptr)
        delete _trayIconGPUTemperatureMenu;
}

void System_Tray::setIcon()
{
    QRect rect (0, 0, 22, 22);
    QPixmap pix(rect.size());
    QPainter painter(&pix);
    QFont font = painter.font();
    font.setPixelSize(14);
    painter.setFont(font);

    // Температура

    painter.fillRect(rect, backgroundTemperature());
    painter.drawText(rect, Qt::AlignCenter, temperature(_temperature));

    _trayIconTemperature->setIcon(pix);

    std::stringstream stream;
    stream << "Температура процессора (C°)\n\n";

    for (int i = 0; i < _ncpu - 1; ++i)
        stream << "Ядро " << i << ": " << _tempeCores.at(i) << "\n";
    stream << "Ядро " << _ncpu - 1 << ": " << _tempeCores.back();

    _trayIconTemperature->setToolTip(stream.str().c_str());

    // ОЗУ

    painter.fillRect(rect, backgroundActiveMem());
    painter.drawText(rect, Qt::AlignCenter, QString::number(int(_activeMem / (_totalMem / 100.0) + 0.5)));
    _trayIconMemory->setIcon(pix);

    stream.str({});
    stream << "Оперативная память (Использовано %)\n\n"
           << "Детали:\n"
           << "total: " << (_totalMem / 1024 / 1024) << " МБ (" << toPer(_totalMem) << "%)\n"
           << "active: " << (_activeMem / 1024 / 1024) << " МБ (" << toPer(_activeMem) << "%)\n"
           << "inactive: " << (_inactiveMem / 1024 / 1024) << " МБ (" << toPer(_inactiveMem) << "%)\n"
           << "laundry: " << (_laundryMem / 1024 / 1024) << " МБ (" << toPer(_laundryMem) << "%)\n"
           << "wired: " << (_wiredMem / 1024 / 1024) << " МБ (" << toPer(_wiredMem) << "%)\n"
           << "free: " << (_freeMem / 1024 / 1024) << " МБ (" << toPer(_freeMem) << "%)";

    _trayIconMemory->setToolTip(stream.str().c_str());

    // GPU температура
    painter.fillRect(rect, backgroundGPUTemperature());
    painter.drawText(rect, Qt::AlignCenter, temperature(_temperatureGPU));

    _trayIconGPUTemperature->setIcon(pix);

    stream.str({});
    stream << "Температура GPU (C°)\n\n";

    stream << _temperatureGPU;

    _trayIconGPUTemperature->setToolTip(stream.str().c_str());
}

void System_Tray::showTemperatureMessage() const
{
    _trayIconTemperature->showMessage("Внимание", "Высокий уровень температуры процессора", QSystemTrayIcon::Warning, 2000);
}

void System_Tray::showFreeMemMessage() const
{
    _trayIconTemperature->showMessage("Внимание", "Мало свободной ОЗУ", QSystemTrayIcon::Warning, 2000);
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
    int t = 0;
    size_t len = sizeof(t);
    _temperature = 0;
    for (int i = 0; i < _ncpu; ++i)
    {
        std::string temp = "dev.cpu." + std::to_string(i) + ".temperature";
        sysctlbyname(temp.c_str(), &t, &len, NULL, 0);

        t = (t - 2731) / 10;
        _tempeCores.at(i) = t;
        _temperature += t;
    }
    _temperature /= _ncpu;

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

    if (FILE *file = fopen(NVIDIA_SMI, "r"))
    {
        fclose(file);
        QProcess nv_smi;
        nv_smi.start(NVIDIA_SMI, QStringList() << "-q" << "-d" << "TEMPERATURE");
        nv_smi.waitForFinished();
        QStringList list = QString(nv_smi.readAll()).split("\n");

        for(const auto &el : list)
        {
            if (el.contains("GPU Current Temp"))
            {
                auto index = el.indexOf(":");
                bool ok;
                int t = el.mid(index + 1).remove(" ").remove("C").toInt(&ok);
                if (ok)
                    _temperatureGPU = t;
            }
        }
    }

#endif

    if (_temperature >= 90 && _showMessageTemperature->isChecked())
        showTemperatureMessage();

    if ((_activeMem / (_totalMem / 100)) >= 90 && _showMessageTemperature->isChecked())
        showFreeMemMessage();

    setIcon();
}

void System_Tray::createActions()
{
    _quitAction = new QAction("Выход", this);
    connect(_quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    _propertyTemperature = new QAction("Настройки", this);
    _propertyMemory = new QAction("Настройки", this);
    _propertyGPUTemperature = new QAction("Настройки", this);

    _showMessageTemperature = new QAction("Отображать сообщения", this);
    _showMessageTemperature->setCheckable(true);
    _showMessageTemperature->setChecked(true);

    _showMessageMemory = new QAction("Отображать сообщения", this);
    _showMessageMemory->setCheckable(true);
    _showMessageMemory->setChecked(true);

    if (FILE *file = fopen(NVIDIA_SMI, "r"))
    {
        _showMessageGPUTemperature = new QAction("Отображать сообщения", this);
        _showMessageGPUTemperature->setCheckable(true);
        _showMessageGPUTemperature->setChecked(true);
    }
}

void System_Tray::createTrayIcons()
{
    _trayIconTemperatureMenu = new QMenu;
    _trayIconTemperatureMenu->addAction(_showMessageTemperature);
    _trayIconTemperatureMenu->addAction(_propertyTemperature);
    _trayIconTemperatureMenu->addSeparator();
    _trayIconTemperatureMenu->addAction(_quitAction);

    _trayIconTemperature = new QSystemTrayIcon(this);
    _trayIconTemperature->setContextMenu(_trayIconTemperatureMenu);

    _trayIconMemoryMenu = new QMenu;
    _trayIconMemoryMenu->addAction(_showMessageMemory);
    _trayIconMemoryMenu->addAction(_propertyMemory);
    _trayIconMemoryMenu->addSeparator();
    _trayIconMemoryMenu->addAction(_quitAction);

    _trayIconMemory = new QSystemTrayIcon(this);
    _trayIconMemory->setContextMenu(_trayIconMemoryMenu);

    if (FILE *file = fopen(NVIDIA_SMI, "r"))
    {
        _trayIconGPUTemperatureMenu = new QMenu;
        _trayIconGPUTemperatureMenu->addAction(_showMessageGPUTemperature);
        _trayIconGPUTemperatureMenu->addAction(_propertyGPUTemperature);
        _trayIconGPUTemperatureMenu->addSeparator();
        _trayIconGPUTemperatureMenu->addAction(_quitAction);

        _trayIconGPUTemperature = new QSystemTrayIcon(this);
        _trayIconGPUTemperature->setContextMenu(_trayIconGPUTemperatureMenu);;
    }
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

QColor System_Tray::backgroundGPUTemperature() const
{
    if (_temperatureGPU >= 0 && _temperatureGPU <= 40)
        return Qt::green;
    if (_temperatureGPU <= 60)
        return Qt::yellow;
    if (_temperatureGPU <= 80)
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

QString System_Tray::temperature(int t) const
{
    if (t <= -100 || t > 100)
        return "err";

    return QString::number(t);
}

float System_Tray::toPer(size_t val) const
{
    return val / (_totalMem / 100.0);
}

#endif
