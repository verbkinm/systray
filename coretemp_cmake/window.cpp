#include "window.h"
#include <fstream>

#ifndef QT_NO_SYSTEMTRAYICON

CoreTemp::CoreTemp(QObject *parent) : QObject(parent)
{
    createActions();
    createTrayIcon();

    slotTimerOut();

    trayIcon->show();

    _timer = new QTimer(this);
    _timer->setInterval(2000);

    connect(_timer, &QTimer::timeout, this, &CoreTemp::slotTimerOut);
    _timer->start();
}

CoreTemp::~CoreTemp()
{
    delete trayIconMenu;
}

void CoreTemp::setIcon()
{
    QRect rect (0, 0, 22, 22);
    QPixmap pix(rect.size());
    QPainter painter(&pix);
    QFont font = painter.font();
    font.setPixelSize(14);
    painter.setFont(font);

    painter.fillRect(rect, background());
    painter.drawText(rect, Qt::AlignCenter, temperature());

    trayIcon->setIcon(pix);
    trayIcon->setToolTip("Температура процессора");
}

void CoreTemp::showMessage()
{
    //    trayIcon->showMessage(titleEdit->text(), bodyEdit->toPlainText(), msgIcon, durationSpinBox->value() * 1000);
}

void CoreTemp::slotTimerOut()
{
    std::ifstream file("//sys/devices/platform/coretemp.0/hwmon/hwmon4/temp1_input");
    if (!file.is_open())
    {
        return;
    }

    file >> _temperature;
    _temperature /= 1000;
    setIcon();
}

//void Window::createMessageGroupBox()
//{
//    messageGroupBox = new QGroupBox(tr("Balloon Message"));

//    typeLabel = new QLabel(tr("Type:"));

//    typeComboBox = new QComboBox;
//    typeComboBox->addItem(tr("None"), QSystemTrayIcon::NoIcon);
//    typeComboBox->addItem(style()->standardIcon(
//                              QStyle::SP_MessageBoxInformation), tr("Information"),
//                          QSystemTrayIcon::Information);
//    typeComboBox->addItem(style()->standardIcon(
//                              QStyle::SP_MessageBoxWarning), tr("Warning"),
//                          QSystemTrayIcon::Warning);
//    typeComboBox->addItem(style()->standardIcon(
//                              QStyle::SP_MessageBoxCritical), tr("Critical"),
//                          QSystemTrayIcon::Critical);
//    typeComboBox->addItem(QIcon(), tr("Custom icon"),
//                          -1);
//    typeComboBox->setCurrentIndex(1);

//    durationLabel = new QLabel(tr("Duration:"));

//    durationSpinBox = new QSpinBox;
//    durationSpinBox->setRange(5, 60);
//    durationSpinBox->setSuffix(" s");
//    durationSpinBox->setValue(15);

//    durationWarningLabel = new QLabel(tr("(some systems might ignore this "
//                                         "hint)"));
//    durationWarningLabel->setIndent(10);

//    titleLabel = new QLabel(tr("Title:"));

//    titleEdit = new QLineEdit(tr("Cannot connect to network"));

//    bodyLabel = new QLabel(tr("Body:"));

//    bodyEdit = new QTextEdit;
//    bodyEdit->setPlainText(tr("Don't believe me. Honestly, I don't have a "
//                              "clue.\nClick this balloon for details."));

//    showMessageButton = new QPushButton(tr("Show Message"));
//    showMessageButton->setDefault(true);

//    QGridLayout *messageLayout = new QGridLayout;
//    messageLayout->addWidget(typeLabel, 0, 0);
//    messageLayout->addWidget(typeComboBox, 0, 1, 1, 2);
//    messageLayout->addWidget(durationLabel, 1, 0);
//    messageLayout->addWidget(durationSpinBox, 1, 1);
//    messageLayout->addWidget(durationWarningLabel, 1, 2, 1, 3);
//    messageLayout->addWidget(titleLabel, 2, 0);
//    messageLayout->addWidget(titleEdit, 2, 1, 1, 4);
//    messageLayout->addWidget(bodyLabel, 3, 0);
//    messageLayout->addWidget(bodyEdit, 3, 1, 2, 4);
//    messageLayout->addWidget(showMessageButton, 5, 4);
//    messageLayout->setColumnStretch(3, 1);
//    messageLayout->setRowStretch(4, 1);
//    messageGroupBox->setLayout(messageLayout);
//}

void CoreTemp::createActions()
{
    quitAction = new QAction("Выход", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void CoreTemp::createTrayIcon()
{
    trayIconMenu = new QMenu;
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
}

QColor CoreTemp::background()
{
    if (_temperature >= 0 &&_temperature <= 40)
        return Qt::green;
    else if (_temperature > 40 &&_temperature <= 60)
        return Qt::yellow;
    else if (_temperature > 60 &&_temperature <= 80)
        return qRgb(255,165,0);

    return Qt::red;
}

QString CoreTemp::temperature()
{
    if (_temperature < -100 || _temperature > 100)
        return "err";

    return QString::number(_temperature);
}

#endif
