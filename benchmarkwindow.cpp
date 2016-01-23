#include "benchmarkwindow.h"
#include "ui_benchmarkwindow.h"

BenchmarkWindow::BenchmarkWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BenchmarkWindow)
{
    ui->setupUi(this);

    BenchmarkState = BenchmarkNotStarted;
    mTimeIncrementMillis = 500;
    mBenchmarkStartTime = QTime();
    mBenchmarkNextIncrementMillis = 0;


    mTimer = new QTimer(this);
    mTimer->setInterval(20); // milliseconds
    mTimer->start();

    // TODO: connect slots to vescthing1/2 errors

    connect(mTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    connect(ui->mVescConnector1,  SIGNAL(vescValuesReceived(MC_VALUES)),
            this, SLOT(mcValuesReceived1(MC_VALUES)));
    connect(ui->mVescConnector2,  SIGNAL(vescValuesReceived(MC_VALUES)),
            this, SLOT(mcValuesReceived2(MC_VALUES)));
}

BenchmarkWindow::~BenchmarkWindow()
{
    delete ui;
}

/*
 * setDutyCycle(duty cycle);
 * setRpm(erpm);
 * setCurrent(amps)
 * freespin: setCurrent(0.0)
 * full brake: setDutyCycle(0.0)
 *
 * TODO: timer
 * BIG stop button (lets motors freewheel, not brake)
 * input for drive current
 * input for delay between steps
 * input for brake current step (amps)
 * button to start benchmark
 * chart?
 * stopping naturally should release the brake slightly BEFORE turning off throttle? (if you're using a bench supply to avoid sending current back into it)
 *
 * what if spinning up the motor doesn't work at full current? I guess user must just hit abort.
 */

void BenchmarkWindow::on_startBenchmarkPushButton_clicked()
{
    // make it happen!
//    ui->mTextLog->append("Not Implemented!");
    BenchmarkState = BenchmarkReadyToBegin;
}

void BenchmarkWindow::on_abortPushButton_clicked()
{
    // "end" benchmark
    BenchmarkState = BenchmarkDone;
    // freewheel both motors
    ui->mVescConnector1->setCurrent(0.0);
    ui->mVescConnector2->setCurrent(0.0);

    ui->mTextLog->append("Aborted");
}

void BenchmarkWindow::timerSlot()
{
    switch (BenchmarkState) {
    case BenchmarkNotStarted:
        break;
    case BenchmarkReadyToBegin:
        // TODO: if we're connected to both, init benchmark vars
        // TODO: the benchmark state machine
        if (
// HACK: for testing before getting VESCs running:
true ||
                (ui->mVescConnector1->isConnected() && ui->mVescConnector2->isConnected()))
        {
            // init benchmark state
            BenchmarkState = BenchmarkRunning;
            mTimeIncrementMillis = 2000;
            mBenchmarkStartTime.start();
            mBenchmarkNextIncrementMillis = mTimeIncrementMillis;

            // TODO: load from UI:
            mBenchmarkDriveCurrent = 10.0;
// TODO: "slow start" mode
            mBenchmarkBrakeCurrent = 0.0;
            mBenchmarkBrakeCurrentIncrement = 0.5;
            mBenchmarkMaxBrakeCurrent = 5.0;
            ui->mTextLog->clear();
            ui->mTextLog->append(
                        QString("%1: %2 %3 - %4")
                        .arg(0, 6) // millis
                        .arg(mBenchmarkDriveCurrent, 6) // drive current (A)
                        .arg(mBenchmarkBrakeCurrent, 6) // brake current (A)
                        // message
                        .arg("Spin up motor")
                        );
            ui->mVescConnector1->setCurrent(mBenchmarkDriveCurrent);
            ui->mVescConnector2->setCurrentBrake(mBenchmarkBrakeCurrent);
        }
        else
        {
            ui->mTextLog->append("Both VESC must be connected to start benchmark.");
            BenchmarkState = BenchmarkDone;
        }
        break;
    case BenchmarkRunning:
        {
            // ask VESC for details:
            ui->mVescConnector1->getValues();
            ui->mVescConnector2->getValues();

            int elapsedMillis = mBenchmarkStartTime.elapsed();
            if (elapsedMillis > mBenchmarkNextIncrementMillis) {
                mBenchmarkNextIncrementMillis += mTimeIncrementMillis;
    // TODO: also abort if ERPM gets below some threshold
                mBenchmarkBrakeCurrent += mBenchmarkBrakeCurrentIncrement;
                if (mBenchmarkBrakeCurrent < mBenchmarkMaxBrakeCurrent + 0.001)
                {
                    // log
                    ui->mTextLog->append(
                                QString("%1: %2 %3 - %4")
                                .arg(elapsedMillis, 6) // millis
                                .arg(mBenchmarkDriveCurrent, 6) // drive current (A)
                                .arg(mBenchmarkBrakeCurrent, 6) // brake current (A)
                                // message
                                .arg("More Brake")
                                );
                    ui->mVescConnector1->setCurrent(mBenchmarkDriveCurrent);
                    ui->mVescConnector2->setCurrentBrake(mBenchmarkBrakeCurrent);
                }
                else
                {
                    BenchmarkState = BenchmarkDone;
                    mBenchmarkDriveCurrent = 0.0;
                    mBenchmarkBrakeCurrent = 0.0;
                    // Benchmark over. end brake and let motors spin down
                    // log
                    ui->mTextLog->append(
                                QString("%1: %2 %3 - %4")
                                .arg(elapsedMillis, 6) // millis
                                .arg(mBenchmarkDriveCurrent, 6) // drive current (A)
                                .arg(mBenchmarkBrakeCurrent, 6) // brake current (A)
                                // message
                                .arg("Done")
                                );
                    ui->mVescConnector2->setCurrent(0.0);
                    ui->mVescConnector1->setCurrent(0.0);
                }
            }
            else
            {
                // keep motors going:
                ui->mVescConnector1->setCurrent(mBenchmarkDriveCurrent);
                ui->mVescConnector2->setCurrentBrake(mBenchmarkBrakeCurrent);
            }
            // get motor spinning
            // delay for a sec
    //        for (double brake = 0.0; brake <= maxbrake; brake += brakeIncrement)
    //        {
    //            // set brake current
    //            // delay for a sec
    //        }
            break;
        }
    case BenchmarkDone:
        // Nothing to do
        break;
    }

}

void BenchmarkWindow::mcValuesReceived1(MC_VALUES values)
{
    // TODO: update chart?
    // TODO: abort benchmark if ERPM below some threshold
    // TODO: abort benchmark if motors show different ERPM
}

void BenchmarkWindow::mcValuesReceived2(MC_VALUES values)
{
    // TODO: update chart?
    // TODO: abort benchmark if ERPM below some threshold
    // TODO: abort benchmark if motors show different ERPM
}
