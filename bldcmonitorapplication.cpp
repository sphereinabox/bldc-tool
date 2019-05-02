#include "bldcmonitorapplication.h"
#include <QThread>

#define SERIAL_PORT "/dev/tty.usbmodem301"
#define PD_ADDRESS QHostAddress::LocalHost
#define PD_PORT 3000


BldcMonitorApplication::BldcMonitorApplication(int &argc, char **argv)
    : QCoreApplication(argc, argv)
{
    // Compatible firmwares
    mFwVersionReceived = false;
    mFwRetries = 0;
    mCompatibleFws.append(qMakePair(2, 17));
    mCompatibleFws.append(qMakePair(2, 18));

    mSerialPort = new QSerialPort(this);

    mUdpSocket = new QUdpSocket(this);
    //mUdpSocket->bind(QHostAddress(PD_ADDRESS), PD_PORT);

    mTimer = new QTimer(this);
    mTimer->setInterval(20);
    mTimer->start();

    mPacketInterface = new PacketInterface(this);
    mStatusInfoTime = 0;

    connect(mSerialPort, SIGNAL(readyRead()),
            this, SLOT(serialDataAvailable()));
    connect(mSerialPort, SIGNAL(error(QSerialPort::SerialPortError)),
            this, SLOT(serialPortError(QSerialPort::SerialPortError)));
    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));

    connect(mPacketInterface, SIGNAL(dataToSend(QByteArray&)),
            this, SLOT(packetDataToSend(QByteArray&)));
    connect(mPacketInterface, SIGNAL(fwVersionReceived(int,int)),
            this, SLOT(fwVersionReceived(int,int)));
    connect(mPacketInterface, SIGNAL(ackReceived(QString)),
            this, SLOT(ackReceived(QString)));
    connect(mPacketInterface, SIGNAL(valuesReceived(MC_VALUES)),
            this, SLOT(mcValuesReceived(MC_VALUES)));
    connect(mPacketInterface, SIGNAL(printReceived(QString)),
            this, SLOT(printReceived(QString)));
    connect(mPacketInterface, SIGNAL(mcconfReceived(mc_configuration)),
            this, SLOT(mcconfReceived(mc_configuration)));
}

BldcMonitorApplication::~BldcMonitorApplication()
{
}

void BldcMonitorApplication::showStatusInfo(QString info, bool isGood)
{
    if (isGood) {
        qDebug("%s", qPrintable(info));
    } else {
        qWarning("%s", qPrintable(info));
    }
}

void BldcMonitorApplication::serialDataAvailable()
{
    while (mSerialPort->bytesAvailable() > 0) {
        QByteArray data = mSerialPort->readAll();
        mPacketInterface->processData(data);
    }
}

void BldcMonitorApplication::serialPortError(QSerialPort::SerialPortError error)
{
    QString message;
    switch (error) {
    case QSerialPort::NoError:
        break;
    case QSerialPort::DeviceNotFoundError:
        message = tr("Device not found");
        break;
    case QSerialPort::OpenError:
        message = tr("Can't open device");
        break;
    case QSerialPort::NotOpenError:
        message = tr("Not open error");
        break;
    case QSerialPort::ResourceError:
        message = tr("Port disconnected");
        break;
    case QSerialPort::UnknownError:
        message = tr("Unknown error");
        break;
    default:
        message = "Serial port error: " + QString::number(error);
        break;
    }

    if(!message.isEmpty()) {
        showStatusInfo(message, false);

        if(mSerialPort->isOpen()) {
            mSerialPort->close();
        }
    }
}
void BldcMonitorApplication::timerSlot()
{
    // Update CAN fwd function
//    mPacketInterface->setSendCan(ui->canFwdBox->isChecked(), ui->canIdBox->value());

    // Read FW version if needed
    static bool sendCanBefore = false;
    static int canIdBefore = 0;
    if (mSerialPort->isOpen() || mPacketInterface->isUdpConnected()) {
//        if (sendCanBefore != ui->canFwdBox->isChecked() ||
//                canIdBefore != ui->canIdBox->value()) {
//            mFwVersionReceived = false;
//            mFwRetries = 0;
//        }

        if (!mFwVersionReceived) {
            mPacketInterface->getFwVersion();
            mFwRetries++;

            // Timeout if the firmware cannot be read
            if (mFwRetries >= 100) {
                showStatusInfo("No firmware read response", false);
                on_disconnectButton_clicked();
            }
        }

    } else {
        mFwVersionReceived = false;
        mFwRetries = 0;
    }
//    sendCanBefore = ui->canFwdBox->isChecked();
//    canIdBefore = ui->canIdBox->value();

    // Update status label
    if (mStatusInfoTime) {
        mStatusInfoTime--;
        if (!mStatusInfoTime) {
//            mStatusLabel->setStyleSheet(qApp->styleSheet());
        }
    } else {
        if (mSerialPort->isOpen() || mPacketInterface->isUdpConnected()) {
            if (mPacketInterface->isLimitedMode()) {
                showStatusInfo("Connected, limited", true);
            } else {
                // would emit continuously
                //showStatusInfo("Connected", true);
            }
        } else {
            this->showStatusInfo("Not connected", false);
        }
    }

    // Update MC readings
    if (mSerialPort->isOpen() || mPacketInterface->isUdpConnected()) {
        mPacketInterface->getValues();
    } else {
        on_serialConnectButton_clicked();
    }
}

void BldcMonitorApplication::packetDataToSend(QByteArray &data)
{
    if (mSerialPort->isOpen()) {
        mSerialPort->write(data);
    }
}

void BldcMonitorApplication::fwVersionReceived(int major, int minor)
{
    QPair<int, int> highest_supported = *std::max_element(mCompatibleFws.begin(), mCompatibleFws.end());
    QPair<int, int> fw_connected = qMakePair(major, minor);

    bool wasReceived = mFwVersionReceived;

    if (major < 0) {
        mFwVersionReceived = false;
        mFwRetries = 0;
        on_disconnectButton_clicked();
        showStatusInfo(
            "The firmware on the connected VESC is too old. Please"
                    " update it using a programmer.", false);
    } else if (fw_connected > highest_supported) {
        mFwVersionReceived = true;
        mPacketInterface->setLimitedMode(true);
        if (!wasReceived) {
            showStatusInfo(
                "The connected VESC has newer firmware than this version of"
                " BLDC Tool supports. It is recommended that you update BLDC "
                " Tool to the latest version. Alternatively, the firmware on"
                " the connected VESC can be downgraded in the firmware tab."
                " Until then, limited communication mode will be used where"
                " only the firmware can be changed.", false);
        }
    } else if (!mCompatibleFws.contains(fw_connected)) {
        if (fw_connected >= qMakePair(1, 1)) {
            mFwVersionReceived = true;
            mPacketInterface->setLimitedMode(true);
            if (!wasReceived) {
                showStatusInfo(
                    "The connected VESC has too old firmware. Since the"
                    " connected VESC has firmware with bootloader support, it can be"
                    " updated from the Firmware tab."
                    " Until then, limited communication mode will be used where only the"
                    " firmware can be changed.", false);
            }
        } else {
            mFwVersionReceived = false;
            mFwRetries = 0;
            on_disconnectButton_clicked();
            if (!wasReceived) {
                showStatusInfo(
                    "The firmware on the connected VESC is too old. Please"
                    " update it using a programmer.", false);
            }
        }
    } else {
        mFwVersionReceived = true;
        if (fw_connected < highest_supported) {
            if (!wasReceived) {
                showStatusInfo(
                    "The connected VESC has compatible, but old"
                    " firmware. It is recommended that you update it.", false);
            }
        }

        QString fwStr;
        mPacketInterface->setLimitedMode(false);
        fwStr.sprintf("VESC Firmware Version %d.%d", major, minor);
        showStatusInfo(fwStr, true);

        // ask for mcconf
        mPacketInterface->getMcconf();
    }

    if (major >= 0) {
        QString fwText;
        fwText.sprintf("%d.%d", major, minor);
        showStatusInfo(fwText, true);
    }
}

void BldcMonitorApplication::ackReceived(QString ackType)
{
    showStatusInfo(ackType, true);
}

void BldcMonitorApplication::mcValuesReceived(MC_VALUES values)
{
    QString valuesLineCsv;
    // TODO: timestamp?
    valuesLineCsv.sprintf(
        "vesc,%4.2f,%4.1f,%4.1f,%4.1f,%4.1f,%4.1f,%4.1f,%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,%d,%d,%d,%s",
                values.v_in,
                values.temp_mos1,
                values.temp_mos2,
                values.temp_mos3,
                values.temp_mos4,
                values.temp_mos5,
                values.temp_mos6,
                values.temp_pcb,
                values.current_motor,
                values.current_in,
                values.rpm,
                values.duty_now,
                values.amp_hours,
                values.amp_hours_charged,
                values.watt_hours,
                values.watt_hours_charged,
                values.tachometer,
                values.tachometer_abs,
                values.fault_code,
                qPrintable(values.fault_str));
    showStatusInfo(valuesLineCsv, true);
    // TODO: Log values
    // TODO: timestamp?
    
    // broadcast values to PD over UDP
    QString valuesLinePd;
    valuesLinePd.sprintf(
        "vesc %4.2f %4.1f %4.1f %4.1f %4.1f %4.1f %4.1f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %d %d %d %s;\n",
                values.v_in,
                values.temp_mos1,
                values.temp_mos2,
                values.temp_mos3,
                values.temp_mos4,
                values.temp_mos5,
                values.temp_mos6,
                values.temp_pcb,
                values.current_motor,
                values.current_in,
                values.rpm,
                values.duty_now,
                values.amp_hours,
                values.amp_hours_charged,
                values.watt_hours,
                values.watt_hours_charged,
                values.tachometer,
                values.tachometer_abs,
                values.fault_code,
                qPrintable(values.fault_str));
    if (mUdpSocket->writeDatagram(valuesLinePd.toUtf8().constData(), valuesLinePd.length(),
				  PD_ADDRESS,
				  PD_PORT) < 0) {
      QString err;err.sprintf("%d", mUdpSocket->error());
      showStatusInfo("Failed to send UDP", true);
      showStatusInfo(err, true);
    }
}

void BldcMonitorApplication::mcconfReceived(mc_configuration mcconf)
{
//    showStatusInfo("MCCONF Received", true);
    QString valuesLineCsv;
    // TODO: timestamp?
    valuesLineCsv.sprintf(
        "mcconf"
        ",%d,%d,%d"
        ",%.2f,%.2f,%.2f,%.2f,%.2f,%d"
        ",%d,%.2f,%.2f,%.2f,%.2f"
        ",%.2f,%.2f,%.2f,%.2f"
        ",%.2f,%.2f,%.2f,%.2f"
        ",%.4f,%.4f"
        ",%.2f,%.2f,%.2f,%.2f"
        ",%d,%.2f,%.2f,%.2f"
        ",%.3f,%.2f,%.5f"
        ",%.4f,%.4f,%.4f,%.5f"
        ,
                mcconf.pwm_mode,
                mcconf.motor_type,
                mcconf.sensor_mode,

                mcconf.l_current_max,
                mcconf.l_current_min,
                mcconf.l_in_current_max,
                mcconf.l_in_current_min,
                mcconf.l_abs_current_max,
                mcconf.l_slow_abs_current, // bool

                mcconf.l_rpm_lim_neg_torque, // bool
                mcconf.l_min_erpm,
                mcconf.l_max_erpm,
                mcconf.l_max_erpm_fbrake,
                mcconf.l_max_erpm_fbrake_cc,

                mcconf.l_temp_fet_start,
                mcconf.l_temp_fet_end,
                mcconf.l_temp_motor_start,
                mcconf.l_temp_motor_end,

                mcconf.l_min_vin,
                mcconf.l_max_vin,
                mcconf.l_battery_cut_start,
                mcconf.l_battery_cut_end,

                mcconf.l_min_duty,
                mcconf.l_max_duty,

                // BLDC sensorless settings
                mcconf.sl_min_erpm,
                mcconf.sl_cycle_int_rpm_br,
                mcconf.sl_phase_advance_at_br,
                mcconf.sl_max_fullbreak_current_dir_change,

                mcconf.comm_mode, // enum
                mcconf.sl_cycle_int_limit,
                mcconf.sl_min_erpm_cycle_int_limit,
                mcconf.sl_bemf_coupling_k,

                // skipping hall table
                // skipping FOC settings
                // skipping encoder buttons

                // advanced tab
                mcconf.cc_startup_boost_duty,
                mcconf.cc_min_current,
                mcconf.cc_gain,

                mcconf.m_duty_ramp_step,
                mcconf.cc_ramp_step_max, // nope
                mcconf.m_current_backoff_gain,
                mcconf.m_duty_ramp_step_rpm_lim);
//                qPrintable(values.fault_str));
    showStatusInfo(valuesLineCsv, true);
    // TODO: Log values
    // TODO: timestamp?

    // broadcast values to PD over UDP
    QString mcconfLinePd = valuesLineCsv.replace(","," ") + ";\n";
    if (mUdpSocket->writeDatagram(mcconfLinePd.toUtf8().constData(), mcconfLinePd.length(),
          PD_ADDRESS,
          PD_PORT) < 0) {
      QString err;err.sprintf("%d", mUdpSocket->error());
      showStatusInfo("Failed to send UDP", true);
      showStatusInfo(err, true);
    }
}

void BldcMonitorApplication::printReceived(QString str)
{
    showStatusInfo(str, true);
}

void BldcMonitorApplication::on_serialConnectButton_clicked()
{
    if(mSerialPort->isOpen()) {
        return;
    }

    mSerialPort->setPortName(SERIAL_PORT);
    mSerialPort->open(QIODevice::ReadWrite);

    if(!mSerialPort->isOpen()) {
        return;
    }

    mSerialPort->setBaudRate(QSerialPort::Baud115200);
    mSerialPort->setDataBits(QSerialPort::Data8);
    mSerialPort->setParity(QSerialPort::NoParity);
    mSerialPort->setStopBits(QSerialPort::OneStop);
    mSerialPort->setFlowControl(QSerialPort::NoFlowControl);

    // For nrf
    mSerialPort->setRequestToSend(true);
    mSerialPort->setDataTerminalReady(true);
    QThread::msleep(5);
    mSerialPort->setDataTerminalReady(false);
    QThread::msleep(100);

    mPacketInterface->stopUdpConnection();
}

void BldcMonitorApplication::on_disconnectButton_clicked()
{
    if (mSerialPort->isOpen()) {
        mSerialPort->close();
    }

    if (mPacketInterface->isUdpConnected()) {
        mPacketInterface->stopUdpConnection();
    }

    mFwVersionReceived = false;
    mFwRetries = 0;
}

