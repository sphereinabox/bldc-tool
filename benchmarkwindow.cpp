#include "benchmarkwindow.h"
#include "ui_benchmarkwindow.h"
#include <QFile>

BenchmarkWindow::BenchmarkWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BenchmarkWindow)
{
    ui->setupUi(this);

    BenchmarkState = BenchmarkNotStarted;
    mTimeIncrementMillis = 500;
    mBenchmarkStartTime = QTime();
    mBenchmarkNextIncrementMillis = 0;
    mBenchmarkDatapoints = new QVector<BenchmarkDatapoint>();


    mTimer = new QTimer(this);
    mTimer->setInterval(50); // milliseconds
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

/* TODO: On close:
    ui->mVescConnector1->disconnect();
    ui->mVescConnector2->disconnect();
*/
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
            BenchmarkState = BenchmarkMotorStartup;

            mBenchmarkDriveCurrent = 3.0; // startup current
            mBenchmarkBrakeCurrent = 0.0; // freewheel
            mBenchmarkStartTime.start();

            if (mBenchmarkDatapoints != NULL)
            {
                delete mBenchmarkDatapoints;
            }
            mBenchmarkDatapoints = new QVector<BenchmarkDatapoint>();

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
    case BenchmarkMotorStartup:
        {
            if (mBenchmarkStartTime.elapsed() < 2000)
            {
                ui->mVescConnector1->setCurrent(mBenchmarkDriveCurrent);
                ui->mVescConnector2->setCurrentBrake(mBenchmarkBrakeCurrent);
            }
            else
            {
// TODO: Abort if motor didn't spin up.
                // init benchmark state
                BenchmarkState = BenchmarkRunning;
                mTimeIncrementMillis = 2000;
                mBenchmarkStartTime.start();
                mBenchmarkNextIncrementMillis = mTimeIncrementMillis;

                // TODO: load from UI:
                mBenchmarkDriveCurrent = 80.0; // full benchmark current
                mBenchmarkBrakeCurrent = 0.0;
                mBenchmarkBrakeCurrentIncrement = 2;
                mBenchmarkMaxBrakeCurrent = 70.0;

                // Switch to running benchmark
                mTimeIncrementMillis = 2000;
                mBenchmarkStartTime.start();
                mBenchmarkNextIncrementMillis = mTimeIncrementMillis;

                ui->mTextLog->append(
                            QString("%1: %2 %3 - %4")
                            .arg(0, 6) // millis
                            .arg(mBenchmarkDriveCurrent, 6) // drive current (A)
                            .arg(mBenchmarkBrakeCurrent, 6) // brake current (A)
                            // message
                            .arg("Start Benchmark")
                            );
                ui->mVescConnector1->setCurrent(mBenchmarkDriveCurrent);
                ui->mVescConnector2->setCurrentBrake(mBenchmarkBrakeCurrent);
            }
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
    int elapsed = mBenchmarkStartTime.elapsed();
    BenchmarkDatapoint bdp = {
        elapsed,
        // isDriveMotor
        1,
        values
    };
    mBenchmarkDatapoints->append(bdp);
    // TODO: update chart?
    // TODO: abort benchmark if ERPM below some threshold
    // TODO: abort benchmark if motors show different ERPM

    ui->mRpmLabel->setText(QString().sprintf("%.3f", values.rpm));
}

void BenchmarkWindow::mcValuesReceived2(MC_VALUES values)
{
    int elapsed = mBenchmarkStartTime.elapsed();
    BenchmarkDatapoint bdp = {
        elapsed,
        // isDriveMotor
        0,
        values
    };
    mBenchmarkDatapoints->append(bdp);
    // TODO: update chart?
    // TODO: abort benchmark if ERPM below some threshold
    // TODO: abort benchmark if motors show different ERPM
}

void BenchmarkWindow::on_saveResultsPushButton_clicked()
{
    QString path = QString("/Users/nick/Desktop/log.csv");
    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Could not open" << path;
        return;
    }

    QTextStream out(&file);

    QString header;
    header.append("elapsedMilliseconds,");
    header.append("isDriveMotor,");
    header.append("v_in,");
    header.append("temp_mos1,");
    header.append("temp_mos2,");
    header.append("temp_mos3,");
    header.append("temp_mos4,");
    header.append("temp_mos5,");
    header.append("temp_mos6,");
    header.append("temp_pcb,");
    header.append("current_motor,");
    header.append("current_in,");
    header.append("rpm,");
    header.append("duty_now,");
    header.append("amp_hours,");
    header.append("amp_hours_charged,");
    header.append("watt_hours,");
    header.append("watt_hours_charged,");
    header.append("tachometer,");
    header.append("tachometer_abs\n");
    out << header;

    QVectorIterator<BenchmarkDatapoint> i(*mBenchmarkDatapoints);

    while (i.hasNext()) {
        const BenchmarkDatapoint &element = i.next();
        QString row;

        row.append(QString().sprintf("%d,", element.elapsedMilliseconds));
        row.append(QString().sprintf("%d,", element.isDriveMotor));
        row.append(QString().sprintf("%.3f,", element.values.v_in));
        row.append(QString().sprintf("%.3f,", element.values.temp_mos1));
        row.append(QString().sprintf("%.3f,", element.values.temp_mos2));
        row.append(QString().sprintf("%.3f,", element.values.temp_mos3));
        row.append(QString().sprintf("%.3f,", element.values.temp_mos4));
        row.append(QString().sprintf("%.3f,", element.values.temp_mos5));
        row.append(QString().sprintf("%.3f,", element.values.temp_mos6));
        row.append(QString().sprintf("%.3f,", element.values.temp_pcb));
        row.append(QString().sprintf("%.3f,", element.values.current_motor));
        row.append(QString().sprintf("%.3f,", element.values.current_in));
        row.append(QString().sprintf("%.3f,", element.values.rpm));
        row.append(QString().sprintf("%.3f,", element.values.duty_now));
        row.append(QString().sprintf("%.3f,", element.values.amp_hours));
        row.append(QString().sprintf("%.3f,", element.values.amp_hours_charged));
        row.append(QString().sprintf("%.3f,", element.values.watt_hours));
        row.append(QString().sprintf("%.3f,", element.values.watt_hours_charged));
        row.append(QString().sprintf("%d,", element.values.tachometer));
        row.append(QString().sprintf("%d", element.values.tachometer_abs));

        out << row << "\n";
    }

    file.close();
}
