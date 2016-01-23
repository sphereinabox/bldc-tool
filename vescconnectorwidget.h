#ifndef VESCCONNECTORWIDGET_H
#define VESCCONNECTORWIDGET_H

#include <QWidget>
#include <QSerialPort>
#include "ui_vescconnectorwidget.h"

#include "packetinterface.h"

class VescConnectorWidget : public QWidget, protected Ui::VescConnectorWidget
{
    Q_OBJECT
public:
    explicit VescConnectorWidget(QWidget *parent = 0);

    bool setDutyCycle(double dutyCycle);
    bool setCurrent(double current);
    bool setCurrentBrake(double current);
    bool setRpm(int rpm);
    bool getValues();
    bool isConnected();

signals:
    void vescValuesReceived(MC_VALUES values);
public slots:

private slots:
    void serialDataAvailable();
    void serialPortError(QSerialPort::SerialPortError error);

    void loadSerialPortsList();

    void timerSlot();
    void packetDataToSend(QByteArray &data);
    void fwVersionReceived(int major, int minor);
    void ackReceived(QString ackType);
    void mcValuesReceived(MC_VALUES values);
    void printReceived(QString str);
//    void samplesReceived(QByteArray data);
//    void rotorPosReceived(double pos);
//    void experimentSamplesReceived(QVector<double> samples);
//    void mcconfReceived(mc_configuration mcconf);
//    void motorParamReceived(double cycle_int_limit, double bemf_coupling_k, QVector<int> hall_table, int hall_res);
//    void motorRLReceived(double r, double l);
//    void motorLinkageReceived(double flux_linkage);
//    void encoderParamReceived(double offset, double ratio, bool inverted);
//    void appconfReceived(app_configuration appconf);
//    void decodedPpmReceived(double ppm_value, double ppm_last_len);
//    void decodedAdcReceived(double adc_value, double adc_voltage);
//    void decodedChukReceived(double chuk_value);

    void on_serialConnectButton_clicked();
    void on_serialDisconnectButton_clicked();

private:
    QSerialPort *mSerialPort;
    QTimer *mTimer;
    int mStatusInfoTime;
    bool mFwVersionReceived;
    int mFwRetries;
    QList<QPair<int, int> > mCompatibleFws;

    PacketInterface *mPacketInterface;

//    mc_configuration getMcconfGui();
//    void setMcconfGui(const mc_configuration &mcconf);
    void showStatusInfo(QString info, bool isGood);
//    void appendDoubleAndTrunc(QVector<double> *vec, double num, int maxSize);
//    void clearBuffers();
//    void saveExperimentSamplesToFile(QString path);
};

#endif // VESCCONNECTORWIDGET_H
