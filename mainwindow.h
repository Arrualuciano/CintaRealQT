/**
 * @file    mainwindow.h
 * @brief   Ventana principal del HMI para la cinta clasificadora.
 * @author  Luciano
 * @date    Mayo 2026
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QTableWidget>
#include <QTextEdit>
#include <QTimer>
#include <QElapsedTimer>
#include "serialmanager.h"
#include "protocol.h"

struct BoxInTransit {
    int     id;
    uint8_t type;
    uint8_t servo;
    double  totalTime;
    double  remaining;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    /* Conexion */
    void onConnectClicked();
    void onRefreshPorts();
    void onSerialConnected();
    void onSerialDisconnected();
    void onSerialError(const QString &msg);
    void onFrameReceived(uint8_t cmdId, QByteArray payload);

    /* Calibracion */
    void onStartCalibration();
    void onResetDefaultTimes();
    void onApplyThresholds();
    void onTestServo(int servo);
    void onApplyServoAngle();

    /* Clasificacion */
    void onStart();
    void onStop();
    void onReset();
    void onApplyOutputConfig();

    /* Timer de refresco */
    void onRefreshTick();

private:
    void buildUi();
    QWidget *buildToolbar();
    QWidget *buildSidebar();
    QWidget *buildCalibrationPage();
    QWidget *buildClassificationPage();
    void applyStyle();
    void setPage(int index);
    void addBoxToQueue(uint8_t type, uint8_t servo, uint16_t timeTicks);
    void removeBoxByServo(uint8_t servo);
    void updateQueueTable();
    void setSensorLed(int idx, bool active);
    void log(const QString &msg);

    SerialManager *serial;
    QTimer        *refreshTimer;

    /* Toolbar */
    QComboBox   *cmbPort;
    QPushButton *btnConnect;
    QLabel      *lblStatus;

    /* Sidebar */
    QPushButton *btnPageCalib;
    QPushButton *btnPageClassif;
    QStackedWidget *pages;

    /* --- Pagina Calibracion --- */
    QLabel      *lblCalibState;
    QLabel      *lblCalibTime[3];
    QPushButton *btnStartCalib;
    QPushButton *btnResetDefaults;
    QPushButton *btnTestServo[3];
    QSpinBox    *spnServoAngle[3];
    QPushButton *btnApplyAngle;
    QSpinBox    *spnThreshGrande;
    QSpinBox    *spnThreshMediana;
    QSpinBox    *spnThreshPequena;
    QPushButton *btnApplyThresh;

    /* --- Pagina Clasificacion --- */
    QPushButton *btnStart;
    QPushButton *btnStop;
    QPushButton *btnReset;
    QComboBox   *cmbOutputType[3];
    QDoubleSpinBox *spnOutputTime[3];
    QPushButton *btnApplyConfig;
    QLabel      *lblSensor[4];
    QLabel      *lblServoState[3];
    QLabel      *lblDistance;
    QLabel      *lblBoxCount[3];
    QLabel      *lblTotalBoxes;
    QTableWidget *tblQueue;
    QTextEdit   *txtLog;

    /* Estado */
    bool calibMode  = false;
    bool beltRunning = false;
    int  boxCounts[3] = {0, 0, 0};
    int  nextBoxId = 1;
    QList<BoxInTransit> boxQueue;
    QElapsedTimer calibElapsed;
};

#endif // MAINWINDOW_H
