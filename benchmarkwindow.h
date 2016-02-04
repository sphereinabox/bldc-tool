#ifndef BENCHMARKWINDOW_H
#define BENCHMARKWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTime>

#include "packetinterface.h"

struct BenchmarkDatapoint {
    int elapsedMilliseconds;
    int isDriveMotor;
    MC_VALUES values;
};

namespace Ui {
class BenchmarkWindow;
}

class BenchmarkWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BenchmarkWindow(QWidget *parent = 0);
    ~BenchmarkWindow();

private slots:
    void timerSlot();
    void mcValuesReceived1(MC_VALUES values);
    void mcValuesReceived2(MC_VALUES values);

    void on_startBenchmarkPushButton_clicked();
    void on_abortPushButton_clicked();

    void on_saveResultsPushButton_clicked();

private:
    Ui::BenchmarkWindow *ui;

    QTimer *mTimer;

    QVector<BenchmarkDatapoint> *mBenchmarkDatapoints;

    enum BenchmarkState { BenchmarkNotStarted = 0,
                          BenchmarkReadyToBegin,
                          BenchmarkMotorStartup,
                          BenchmarkRunning,
                          BenchmarkDone } BenchmarkState;
    // 500 or 1000 likely
    int mTimeIncrementMillis;
    // timer started when benchmark starts
    QTime mBenchmarkStartTime;
    // # millis at which to increment to next step of benchmark
    // (referencing mBenchmarkStartTime->elapsed()
    int mBenchmarkNextIncrementMillis;

    double mBenchmarkDriveCurrent;
    double mBenchmarkBrakeCurrent;
    double mBenchmarkBrakeCurrentIncrement;
    double mBenchmarkMaxBrakeCurrent;
};

#endif // BENCHMARKWINDOW_H
