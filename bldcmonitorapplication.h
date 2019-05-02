#ifndef MYCOREAPPLICATION_H
#define MYCOREAPPLICATION_H

#include <QCoreApplication>
#include <QSerialPort>
#include <QUdpSocket>
#include <QTimer>
#include "packetinterface.h"
#include "datatypes.h"

class BldcMonitorApplication : public QCoreApplication
{
    Q_OBJECT
public:
    explicit BldcMonitorApplication(int &argc, char **argv);
    ~BldcMonitorApplication();

private slots:
    void serialDataAvailable();
    void serialPortError(QSerialPort::SerialPortError error);

    void timerSlot();
    void packetDataToSend(QByteArray &data);
    void fwVersionReceived(int major, int minor);
    void ackReceived(QString ackType);
    void mcValuesReceived(MC_VALUES values);
    void printReceived(QString str);
    void mcconfReceived(mc_configuration mcconf);

    void on_serialConnectButton_clicked();
    void on_disconnectButton_clicked();

private:
    QSerialPort *mSerialPort;
    QTimer *mTimer;
    int mStatusInfoTime;
    bool mFwVersionReceived;
    int mFwRetries;
    QList<QPair<int, int> > mCompatibleFws;

    QUdpSocket *mUdpSocket;

    PacketInterface *mPacketInterface;

    void showStatusInfo(QString info, bool isGood);
};

#endif // MYCOREAPPLICATION_H
