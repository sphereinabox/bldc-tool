
#include <QThread>
#include <QMessageBox>
#include <QSerialPortInfo>
#include "vescconnectorwidget.h"

VescConnectorWidget::VescConnectorWidget(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);

    mSerialPort = new QSerialPort(this);

    mTimer = new QTimer(this);
    mTimer->setInterval(20); // milliseconds
    mTimer->start();

    // Compatible firmwares
    mFwVersionReceived = false;
    mFwRetries = 0;
    mCompatibleFws.append(qMakePair(2, 7));
    mCompatibleFws.append(qMakePair(2, 8));

    mPacketInterface = new PacketInterface(this);

    loadSerialPortsList();

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
//    connect(mPacketInterface, SIGNAL(samplesReceived(QByteArray)),
//            this, SLOT(samplesReceived(QByteArray)));
//    connect(mPacketInterface, SIGNAL(rotorPosReceived(double)),
//            this, SLOT(rotorPosReceived(double)));
//    connect(mPacketInterface, SIGNAL(experimentSamplesReceived(QVector<double>)),
//            this, SLOT(experimentSamplesReceived(QVector<double>)));
//    connect(mPacketInterface, SIGNAL(mcconfReceived(mc_configuration)),
//            this, SLOT(mcconfReceived(mc_configuration)));
//    connect(mPacketInterface, SIGNAL(motorParamReceived(double,double,QVector<int>,int)),
//            this, SLOT(motorParamReceived(double,double,QVector<int>,int)));
//    connect(mPacketInterface, SIGNAL(motorRLReceived(double,double)),
//            this, SLOT(motorRLReceived(double,double)));
//    connect(mPacketInterface, SIGNAL(motorLinkageReceived(double)),
//            this, SLOT(motorLinkageReceived(double)));
//    connect(mPacketInterface, SIGNAL(encoderParamReceived(double,double,bool)),
//            this, SLOT(encoderParamReceived(double,double,bool)));
//    connect(mPacketInterface, SIGNAL(appconfReceived(app_configuration)),
//            this, SLOT(appconfReceived(app_configuration)));
//    connect(mPacketInterface, SIGNAL(decodedPpmReceived(double,double)),
//            this, SLOT(decodedPpmReceived(double,double)));
//    connect(mPacketInterface, SIGNAL(decodedAdcReceived(double,double)),
//            this, SLOT(decodedAdcReceived(double,double)));
//    connect(mPacketInterface, SIGNAL(decodedChukReceived(double)),
//            this, SLOT(decodedChukReceived(double)));
}

bool VescConnectorWidget::setDutyCycle(double dutyCycle)
{
    mDriveModeLabel->setText(QString("Duty Cycle: %1").arg(dutyCycle));
    return mPacketInterface->setDutyCycle(dutyCycle);
}

bool VescConnectorWidget::setCurrent(double current)
{
    mDriveModeLabel->setText(QString("Current: %1").arg(current));
    return mPacketInterface->setCurrent(current);
}

bool VescConnectorWidget::setCurrentBrake(double current)
{
    mDriveModeLabel->setText(QString("Current Brake: %1").arg(current));
    return mPacketInterface->setCurrentBrake(current);
}

bool VescConnectorWidget::setRpm(int rpm)
{
    mDriveModeLabel->setText(QString("RPM: %1").arg(rpm));
    return mPacketInterface->setRpm(rpm);
}

bool VescConnectorWidget::getValues()
{
    return mPacketInterface->getValues();
}

bool VescConnectorWidget::isConnected()
{
    // TODO: revise this with other checks
    return mSerialPort->isOpen();
}

void VescConnectorWidget::on_serialConnectButton_clicked()
{
    if(mSerialPort->isOpen()) {
        return;
    }

    mSerialPort->setPortName(this->serialDeviceComboBox->currentText().trimmed());
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
}

void VescConnectorWidget::on_serialDisconnectButton_clicked()
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

void VescConnectorWidget::loadSerialPortsList()
{
    serialDeviceComboBox->clear();
    /*
     * TODO: Show only VESC or similar devices
     *
     *  systemLocation()     "/dev/cu.usbmodem301"
     *  description()        "ChibiOS/RT Virtual COM Port"
     *  manufacturer()       "STMicroelectronics"
     *  portName()           "cu.usbmodem301"
     *  productIdentifier()  22336
     *  serialNumber()       "301"
     *  vendorIdentifier()   1155
     */
    QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();
    QList<QSerialPortInfo>::iterator serial;
    for (serial = availablePorts.begin(); serial != availablePorts.end(); ++serial)
    {
//        qDebug()
//            << serial->systemLocation() << endl
//            << serial->description() << endl
//            << serial->manufacturer() << endl
//            << serial->portName() << endl
//            << serial->productIdentifier() << endl
//            << serial->serialNumber() << endl
//            << serial->vendorIdentifier() << endl
//            << endl;
        serialDeviceComboBox->addItem(serial->systemLocation());
    }
}

void VescConnectorWidget::showStatusInfo(QString info, bool isGood)
{
    if (isGood) {
        mStatusLabel->setStyleSheet("QLabel { background-color : lightgreen; color : black; }");
    } else {
        mStatusLabel->setStyleSheet("QLabel { background-color : red; color : black; }");
    }

    mStatusInfoTime = 80;
    mStatusLabel->setText(info);
}


void VescConnectorWidget::serialDataAvailable()
{
    while (mSerialPort->bytesAvailable() > 0) {
        QByteArray data = mSerialPort->readAll();
        mPacketInterface->processData(data);
    }
}

void VescConnectorWidget::serialPortError(QSerialPort::SerialPortError error)
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

void VescConnectorWidget::timerSlot()
{
    // Update CAN fwd function
//    mPacketInterface->setSendCan(ui->canFwdBox->isChecked(), ui->canIdBox->value());

    // Read FW version if needed
//    static bool sendCanBefore = false;
//    static int canIdBefore = 0;
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
                on_serialDisconnectButton_clicked();
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
            mStatusLabel->setStyleSheet(qApp->styleSheet());
        }
    } else {
        if (mSerialPort->isOpen() || mPacketInterface->isUdpConnected()) {
            if (mPacketInterface->isLimitedMode()) {
                mStatusLabel->setText("Connected, limited");
            } else {
                mStatusLabel->setText("Connected");
                //mDriveModeLabel->setText("Released");
            }
        } else {
            mStatusLabel->setText("Not connected");
// HACK: For debugging while not connected:
//            mDriveModeLabel->setText("");
        }
    }

    // Update MC readings
//    if (ui->realtimeActivateBox->isChecked()) {
//        mPacketInterface->getValues();
//    }

//    // Handle key events
//    static double keyPower = 0.0;
//    static double lastKeyPower = 0.0;
//    const double lowPower = 0.18;
//    const double lowPowerRev = 0.1;
//    const double highPower = 0.9;
//    const double highPowerRev = 0.3;
//    const double lowStep = 0.02;
//    const double highStep = 0.01;

//    if (keyRight && keyLeft) {
//        if (keyPower >= lowPower) {
//            stepTowards(keyPower, highPower, highStep);
//        } else if (keyPower <= -lowPower) {
//            stepTowards(keyPower, -highPowerRev, highStep);
//        } else if (keyPower >= 0) {
//            stepTowards(keyPower, highPower, lowStep);
//        } else {
//            stepTowards(keyPower, -highPowerRev, lowStep);
//        }
//    } else if (keyRight) {
//        if (fabs(keyPower) > lowPower) {
//            stepTowards(keyPower, lowPower, highStep);
//        } else {
//            stepTowards(keyPower, lowPower, lowStep);
//        }
//    } else if (keyLeft) {
//        if (fabs(keyPower) > lowPower) {
//            stepTowards(keyPower, -lowPowerRev, highStep);
//        } else {
//            stepTowards(keyPower, -lowPowerRev, lowStep);
//        }
//    } else {
//        stepTowards(keyPower, 0.0, lowStep * 3);
//    }

//    if (keyPower != lastKeyPower) {
//        lastKeyPower = keyPower;
//        mPacketInterface->setDutyCycle(keyPower);
//    }

}

void VescConnectorWidget::packetDataToSend(QByteArray &data)
{
    if (mSerialPort->isOpen()) {
        mSerialPort->write(data);
    }
}

void VescConnectorWidget::fwVersionReceived(int major, int minor)
{
    QPair<int, int> highest_supported = *std::max_element(mCompatibleFws.begin(), mCompatibleFws.end());
    QPair<int, int> fw_connected = qMakePair(major, minor);

    bool wasReceived = mFwVersionReceived;

    if (major < 0) {
        mFwVersionReceived = false;
        mFwRetries = 0;
        on_serialDisconnectButton_clicked();
        QMessageBox messageBox;
        messageBox.critical(this, "Error", "The firmware on the connected VESC is too old. Please"
                            " update it using a programmer.");
//        this->firmwareVersionLabel->setText("Old Firmware");
    } else if (fw_connected > highest_supported) {
        mFwVersionReceived = true;
        mPacketInterface->setLimitedMode(true);
        if (!wasReceived) {
            QMessageBox messageBox;
            messageBox.warning(this, "Warning", "The connected VESC has newer firmware than this version of"
                                                " BLDC Tool supports. It is recommended that you update BLDC "
                                                " Tool to the latest version. Alternatively, the firmware on"
                                                " the connected VESC can be downgraded in the firmware tab."
                                                " Until then, limited communication mode will be used where"
                                                " only the firmware can be changed.");
        }
    } else if (!mCompatibleFws.contains(fw_connected)) {
        if (fw_connected >= qMakePair(1, 1)) {
            mFwVersionReceived = true;
            mPacketInterface->setLimitedMode(true);
            if (!wasReceived) {
                QMessageBox messageBox;
                messageBox.warning(this, "Warning", "The connected VESC has too old firmware. Since the"
                                                    " connected VESC has firmware with bootloader support, it can be"
                                                    " updated from the Firmware tab."
                                                    " Until then, limited communication mode will be used where only the"
                                                    " firmware can be changed.");
            }
        } else {
            mFwVersionReceived = false;
            mFwRetries = 0;
            on_serialDisconnectButton_clicked();
            if (!wasReceived) {
                QMessageBox messageBox;
                messageBox.critical(this, "Error", "The firmware on the connected VESC is too old. Please"
                                                   " update it using a programmer.");
            }
        }
    } else {
        mFwVersionReceived = true;
        if (fw_connected < highest_supported) {
            if (!wasReceived) {
                QMessageBox messageBox;
                messageBox.warning(this, "Warning", "The connected VESC has compatible, but old"
                                                    " firmware. It is recommended that you update it.");
            }
        }

        QString fwStr;
        mPacketInterface->setLimitedMode(false);
        fwStr.sprintf("VESC Firmware Version %d.%d", major, minor);
        showStatusInfo(fwStr, true);
    }

    if (major >= 0) {
        QString fwText;
        fwText.sprintf("%d.%d", major, minor);
//        this->firmwareVersionLabel->setText(fwText);
    }
}

void VescConnectorWidget::ackReceived(QString ackType)
{
    showStatusInfo(ackType, true);
}

void VescConnectorWidget::mcValuesReceived(MC_VALUES values)
{
    // Forward:
    vescValuesReceived(values);
}


void VescConnectorWidget::printReceived(QString str)
{
    mStatusLabel->setText(str);
}
