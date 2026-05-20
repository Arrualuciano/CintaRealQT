/**
 * @file    mainwindow.cpp
 * @brief   Implementacion de la ventana principal del HMI.
 * @author  Luciano
 * @date    Mayo 2026
 */

#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollArea>
#include <QFrame>
#include <QFont>
#include <QTime>
#include <QScrollBar>

/* =========================================================================
 * Constructor
 * ====================================================================== */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , serial(new SerialManager(this))
    , refreshTimer(new QTimer(this))
{
    setWindowTitle("Cinta Clasificadora - HMI");
    setMinimumSize(960, 680);

    buildUi();
    applyStyle();

    connect(serial, &SerialManager::connected,       this, &MainWindow::onSerialConnected);
    connect(serial, &SerialManager::disconnected,     this, &MainWindow::onSerialDisconnected);
    connect(serial, &SerialManager::connectionError,  this, &MainWindow::onSerialError);
    connect(serial, &SerialManager::frameReceived,    this, &MainWindow::onFrameReceived);
    connect(refreshTimer, &QTimer::timeout,           this, &MainWindow::onRefreshTick);

    refreshTimer->start(100); // 10 Hz refresco de UI
    onRefreshPorts();
}

/* =========================================================================
 * Construccion de la interfaz
 * ====================================================================== */

void MainWindow::buildUi() {
    auto *central = new QWidget;
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(buildToolbar());

    auto *body = new QHBoxLayout;
    body->setContentsMargins(0,0,0,0);
    body->setSpacing(0);
    body->addWidget(buildSidebar());

    pages = new QStackedWidget;
    pages->addWidget(buildCalibrationPage());
    pages->addWidget(buildClassificationPage());
    body->addWidget(pages, 1);

    mainLayout->addLayout(body, 1);

    /* Panel de comunicacion (siempre visible) */
    auto *logGroup = new QGroupBox("Comunicacion");
    logGroup->setObjectName("logGroup");
    logGroup->setFixedHeight(150);
    auto *logLay = new QVBoxLayout(logGroup);
    logLay->setContentsMargins(8,12,8,8);
    txtLog = new QTextEdit;
    txtLog->setReadOnly(true);
    txtLog->setObjectName("logText");
    txtLog->setPlaceholderText("Esperando datos de comunicacion...");
    logLay->addWidget(txtLog);
    mainLayout->addWidget(logGroup);

    setCentralWidget(central);
    setPage(0);
}

QWidget *MainWindow::buildToolbar() {
    auto *bar = new QWidget;
    bar->setObjectName("toolbar");
    bar->setFixedHeight(52);
    auto *lay = new QHBoxLayout(bar);
    lay->setContentsMargins(16,0,16,0);

    auto *title = new QLabel("CINTA CLASIFICADORA");
    title->setObjectName("toolbarTitle");
    lay->addWidget(title);
    lay->addStretch();

    lay->addWidget(new QLabel("Puerto:"));
    cmbPort = new QComboBox;
    cmbPort->setMinimumWidth(100);
    lay->addWidget(cmbPort);

    auto *btnRefresh = new QPushButton("Actualizar");
    btnRefresh->setObjectName("smallBtn");
    connect(btnRefresh, &QPushButton::clicked, this, &MainWindow::onRefreshPorts);
    lay->addWidget(btnRefresh);

    btnConnect = new QPushButton("Conectar");
    btnConnect->setObjectName("connectBtn");
    connect(btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    lay->addWidget(btnConnect);

    lblStatus = new QLabel("  Desconectado");
    lblStatus->setObjectName("statusDisconnected");
    lay->addWidget(lblStatus);

    return bar;
}

QWidget *MainWindow::buildSidebar() {
    auto *sidebar = new QWidget;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(180);
    auto *lay = new QVBoxLayout(sidebar);
    lay->setContentsMargins(8,16,8,16);
    lay->setSpacing(6);

    btnPageCalib = new QPushButton("Calibracion");
    btnPageCalib->setObjectName("sideBtn");
    btnPageCalib->setCheckable(true);
    connect(btnPageCalib, &QPushButton::clicked, this, [this]{ setPage(0); });
    lay->addWidget(btnPageCalib);

    btnPageClassif = new QPushButton("Clasificacion");
    btnPageClassif->setObjectName("sideBtn");
    btnPageClassif->setCheckable(true);
    connect(btnPageClassif, &QPushButton::clicked, this, [this]{ setPage(1); });
    lay->addWidget(btnPageClassif);

    lay->addStretch();
    return sidebar;
}

/* ---- Pagina de Calibracion ---- */

QWidget *MainWindow::buildCalibrationPage() {
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *page = new QWidget;
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(24,20,24,20);
    lay->setSpacing(16);

    auto *header = new QLabel("Modo Calibracion");
    header->setObjectName("pageTitle");
    lay->addWidget(header);

    /* --- Calibracion de tiempos --- */
    auto *grpCalib = new QGroupBox("Calibracion de Tiempos de Arribo");
    auto *calibLay = new QVBoxLayout(grpCalib);

    lblCalibState = new QLabel("Estado: esperando conexion...");
    lblCalibState->setObjectName("calibState");
    calibLay->addWidget(lblCalibState);

    auto *calibBtns = new QHBoxLayout;
    btnStartCalib = new QPushButton("Iniciar Calibracion");
    btnStartCalib->setObjectName("primaryBtn");
    connect(btnStartCalib, &QPushButton::clicked, this, &MainWindow::onStartCalibration);
    calibBtns->addWidget(btnStartCalib);

    btnResetDefaults = new QPushButton("Resetear a Defaults (2s / 4s / 6s)");
    connect(btnResetDefaults, &QPushButton::clicked, this, &MainWindow::onResetDefaultTimes);
    calibBtns->addWidget(btnResetDefaults);
    calibBtns->addStretch();
    calibLay->addLayout(calibBtns);

    auto *timesGrid = new QGridLayout;
    QString sensorNames[3] = {"Salida 1 (S1)", "Salida 2 (S2)", "Salida 3 (S3)"};
    for (int i = 0; i < 3; i++) {
        timesGrid->addWidget(new QLabel(sensorNames[i]), i, 0);
        lblCalibTime[i] = new QLabel("2.00 s");
        lblCalibTime[i]->setObjectName("calibTime");
        timesGrid->addWidget(lblCalibTime[i], i, 1);
    }
    /* Valores iniciales por defecto */
    lblCalibTime[0]->setText("2.00 s");
    lblCalibTime[1]->setText("4.00 s");
    lblCalibTime[2]->setText("6.00 s");
    calibLay->addLayout(timesGrid);
    lay->addWidget(grpCalib);

    /* --- Test de servos --- */
    auto *grpServo = new QGroupBox("Test de Servos");
    auto *servoLay = new QGridLayout(grpServo);
    servoLay->setColumnStretch(3, 1);

    for (int i = 0; i < 3; i++) {
        servoLay->addWidget(new QLabel(QString("Servo %1:").arg(i+1)), i, 0);
        btnTestServo[i] = new QPushButton("Patear");
        btnTestServo[i]->setFixedWidth(90);
        int idx = i;
        connect(btnTestServo[i], &QPushButton::clicked, this, [this, idx]{ onTestServo(idx); });
        servoLay->addWidget(btnTestServo[i], i, 1);

        servoLay->addWidget(new QLabel("Angulo:"), i, 2);
        spnServoAngle[i] = new QSpinBox;
        spnServoAngle[i]->setRange(0, 180);
        spnServoAngle[i]->setValue(180);
        spnServoAngle[i]->setSuffix(" grados");
        servoLay->addWidget(spnServoAngle[i], i, 3);
    }

    btnApplyAngle = new QPushButton("Aplicar Angulos");
    connect(btnApplyAngle, &QPushButton::clicked, this, &MainWindow::onApplyServoAngle);
    servoLay->addWidget(btnApplyAngle, 3, 0, 1, 4);
    lay->addWidget(grpServo);

    /* --- Umbrales de distancia --- */
    auto *grpThresh = new QGroupBox("Umbrales de Distancia (sensor a 20 cm)");
    auto *threshLay = new QGridLayout(grpThresh);

    threshLay->addWidget(new QLabel("Grande  (dist <)"), 0, 0);
    spnThreshGrande = new QSpinBox;
    spnThreshGrande->setRange(1, 30);
    spnThreshGrande->setValue(12);
    spnThreshGrande->setSuffix(" cm");
    threshLay->addWidget(spnThreshGrande, 0, 1);

    threshLay->addWidget(new QLabel("Mediana (dist <)"), 1, 0);
    spnThreshMediana = new QSpinBox;
    spnThreshMediana->setRange(1, 30);
    spnThreshMediana->setValue(14);
    spnThreshMediana->setSuffix(" cm");
    threshLay->addWidget(spnThreshMediana, 1, 1);

    threshLay->addWidget(new QLabel("Pequena (dist <)"), 2, 0);
    spnThreshPequena = new QSpinBox;
    spnThreshPequena->setRange(1, 30);
    spnThreshPequena->setValue(17);
    spnThreshPequena->setSuffix(" cm");
    threshLay->addWidget(spnThreshPequena, 2, 1);

    btnApplyThresh = new QPushButton("Aplicar Umbrales");
    connect(btnApplyThresh, &QPushButton::clicked, this, &MainWindow::onApplyThresholds);
    threshLay->addWidget(btnApplyThresh, 3, 0, 1, 2);
    lay->addWidget(grpThresh);

    lay->addStretch();
    scroll->setWidget(page);
    return scroll;
}

/* ---- Pagina de Clasificacion ---- */

QWidget *MainWindow::buildClassificationPage() {
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *page = new QWidget;
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(24,20,24,20);
    lay->setSpacing(16);

    auto *header = new QLabel("Modo Clasificacion");
    header->setObjectName("pageTitle");
    lay->addWidget(header);

    /* --- Controles de cinta --- */
    auto *ctrlLay = new QHBoxLayout;
    btnStart = new QPushButton("Iniciar");
    btnStart->setObjectName("primaryBtn");
    connect(btnStart, &QPushButton::clicked, this, &MainWindow::onStart);
    ctrlLay->addWidget(btnStart);

    btnStop = new QPushButton("Detener");
    btnStop->setObjectName("warningBtn");
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::onStop);
    ctrlLay->addWidget(btnStop);

    btnReset = new QPushButton("Reset");
    btnReset->setObjectName("dangerBtn");
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::onReset);
    ctrlLay->addWidget(btnReset);
    ctrlLay->addStretch();
    lay->addLayout(ctrlLay);

    /* --- Configuracion de salidas --- */
    auto *grpConfig = new QGroupBox("Configuracion de Salidas");
    auto *cfgGrid = new QGridLayout(grpConfig);
    cfgGrid->addWidget(new QLabel("Salida"), 0, 0);
    cfgGrid->addWidget(new QLabel("Tipo de caja"), 0, 1);
    cfgGrid->addWidget(new QLabel("Tiempo de arribo"), 0, 2);

    QString boxOptions[3] = {"6 cm (Pequena)", "8 cm (Mediana)", "10 cm (Grande)"};
    int defaultIdx[3] = {0, 1, 2};
    double defaultTimes[3] = {2.0, 4.0, 6.0};

    for (int i = 0; i < 3; i++) {
        cfgGrid->addWidget(new QLabel(QString("Salida %1").arg(i+1)), i+1, 0);

        cmbOutputType[i] = new QComboBox;
        for (auto &opt : boxOptions) cmbOutputType[i]->addItem(opt);
        cmbOutputType[i]->setCurrentIndex(defaultIdx[i]);
        cfgGrid->addWidget(cmbOutputType[i], i+1, 1);

        spnOutputTime[i] = new QDoubleSpinBox;
        spnOutputTime[i]->setRange(0.1, 30.0);
        spnOutputTime[i]->setValue(defaultTimes[i]);
        spnOutputTime[i]->setSuffix(" s");
        spnOutputTime[i]->setDecimals(2);
        spnOutputTime[i]->setSingleStep(0.1);
        cfgGrid->addWidget(spnOutputTime[i], i+1, 2);
    }

    btnApplyConfig = new QPushButton("Aplicar Configuracion");
    btnApplyConfig->setObjectName("primaryBtn");
    connect(btnApplyConfig, &QPushButton::clicked, this, &MainWindow::onApplyOutputConfig);
    cfgGrid->addWidget(btnApplyConfig, 4, 0, 1, 3);
    lay->addWidget(grpConfig);

    /* --- Estado en tiempo real --- */
    auto *realTimeLayout = new QHBoxLayout;

    auto *grpSensors = new QGroupBox("Sensores");
    auto *sensLay = new QGridLayout(grpSensors);
    QString sNames[4] = {"S0 (Medicion)", "S1 (Salida 1)", "S2 (Salida 2)", "S3 (Salida 3)"};
    for (int i = 0; i < 4; i++) {
        lblSensor[i] = new QLabel("OFF");
        lblSensor[i]->setObjectName("ledOff");
        lblSensor[i]->setAlignment(Qt::AlignCenter);
        lblSensor[i]->setFixedSize(60, 28);
        sensLay->addWidget(new QLabel(sNames[i]), i, 0);
        sensLay->addWidget(lblSensor[i], i, 1);
    }
    lblDistance = new QLabel("Distancia: -- cm");
    lblDistance->setObjectName("distLabel");
    sensLay->addWidget(lblDistance, 4, 0, 1, 2);
    realTimeLayout->addWidget(grpSensors);

    auto *grpServos = new QGroupBox("Servos");
    auto *srvLay = new QVBoxLayout(grpServos);
    for (int i = 0; i < 3; i++) {
        lblServoState[i] = new QLabel(QString("Servo %1: REPOSO").arg(i+1));
        lblServoState[i]->setObjectName("servoIdle");
        srvLay->addWidget(lblServoState[i]);
    }
    srvLay->addStretch();
    realTimeLayout->addWidget(grpServos);

    auto *grpCount = new QGroupBox("Conteo de Cajas");
    auto *cntLay = new QVBoxLayout(grpCount);
    QString typeNames[3] = {"Pequena (6 cm):", "Mediana (8 cm):", "Grande (10 cm):"};
    for (int i = 0; i < 3; i++) {
        auto *row = new QHBoxLayout;
        row->addWidget(new QLabel(typeNames[i]));
        lblBoxCount[i] = new QLabel("0");
        lblBoxCount[i]->setObjectName("boxCount");
        row->addWidget(lblBoxCount[i]);
        row->addStretch();
        cntLay->addLayout(row);
    }
    lblTotalBoxes = new QLabel("Total: 0");
    lblTotalBoxes->setObjectName("totalCount");
    cntLay->addWidget(lblTotalBoxes);
    cntLay->addStretch();
    realTimeLayout->addWidget(grpCount);

    lay->addLayout(realTimeLayout);

    /* --- Cola de cajas --- */
    auto *grpQueue = new QGroupBox("Cola de Cajas en Transito");
    auto *qLay = new QVBoxLayout(grpQueue);
    tblQueue = new QTableWidget(0, 4);
    tblQueue->setHorizontalHeaderLabels({"#", "Tipo", "Destino", "Tiempo restante"});
    tblQueue->horizontalHeader()->setStretchLastSection(true);
    tblQueue->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tblQueue->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblQueue->setSelectionMode(QAbstractItemView::NoSelection);
    tblQueue->setMaximumHeight(180);
    qLay->addWidget(tblQueue);
    lay->addWidget(grpQueue);

    lay->addStretch();
    scroll->setWidget(page);
    return scroll;
}

/* =========================================================================
 * Estilos
 * ====================================================================== */

void MainWindow::applyStyle() {
    setStyleSheet(R"(
        * { font-size: 10pt; color: #37474f; }

        QMainWindow { background: #f0f2f5; }

        #toolbar {
            background: #1a237e;
            border-bottom: 2px solid #0d1642;
        }
        #toolbar QLabel { color: #ffffff; font-size: 10pt; }
        #toolbarTitle { font-size: 12pt; font-weight: bold; letter-spacing: 2px; }

        #sidebar {
            background: #263238;
            border-right: 1px solid #1a2327;
        }
        #sideBtn {
            background: transparent;
            color: #b0bec5;
            border: none;
            border-radius: 6px;
            padding: 12px 16px;
            font-size: 11pt;
            text-align: left;
        }
        #sideBtn:checked {
            background: #37474f;
            color: #ffffff;
            font-weight: bold;
        }
        #sideBtn:hover { background: #37474f; color: #eceff1; }

        #pageTitle {
            font-size: 15pt;
            font-weight: bold;
            color: #1a237e;
            padding-bottom: 4px;
        }

        QGroupBox {
            font-weight: bold;
            font-size: 10pt;
            color: #37474f;
            border: 1px solid #cfd8dc;
            border-radius: 8px;
            margin-top: 1.2em;
            padding-top: 1.2em;
            padding-left: 12px;
            padding-right: 12px;
            padding-bottom: 12px;
            background: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 16px;
            padding: 0 6px;
            background: #ffffff;
        }

        QPushButton {
            background: #eceff1;
            border: 1px solid #b0bec5;
            border-radius: 6px;
            padding: 8px 18px;
            font-size: 10pt;
            color: #37474f;
        }
        QPushButton:hover { background: #cfd8dc; }
        QPushButton:pressed { background: #b0bec5; }

        #primaryBtn {
            background: #1565c0;
            color: #ffffff;
            border: none;
            font-weight: bold;
        }
        #primaryBtn:hover { background: #1976d2; }

        #warningBtn {
            background: #e65100;
            color: #ffffff;
            border: none;
        }
        #warningBtn:hover { background: #ef6c00; }

        #dangerBtn {
            background: #b71c1c;
            color: #ffffff;
            border: none;
        }
        #dangerBtn:hover { background: #c62828; }

        #connectBtn {
            background: #2e7d32;
            color: #ffffff;
            border: none;
            font-weight: bold;
            padding: 6px 20px;
        }
        #connectBtn:hover { background: #388e3c; }

        #smallBtn {
            padding: 6px 12px;
            font-size: 8pt;
        }

        #statusDisconnected { color: #ef5350; font-weight: bold; }
        #statusConnected    { color: #66bb6a; font-weight: bold; }

        #calibState {
            font-size: 11pt;
            color: #1565c0;
            padding: 8px;
            background: #e3f2fd;
            border-radius: 6px;
        }
        #calibTime {
            font-size: 12pt;
            font-weight: bold;
            color: #1b5e20;
            padding: 4px 12px;
        }

        QSpinBox, QDoubleSpinBox, QComboBox {
            padding: 6px 10px;
            border: 1px solid #b0bec5;
            border-radius: 4px;
            background: #ffffff;
            font-size: 10pt;
        }

        #ledOff {
            background: #cfd8dc;
            color: #546e7a;
            border-radius: 4px;
            font-weight: bold;
            font-size: 8pt;
        }
        #ledOn {
            background: #66bb6a;
            color: #ffffff;
            border-radius: 4px;
            font-weight: bold;
            font-size: 8pt;
        }

        #servoIdle   { color: #78909c; font-size: 10pt; padding: 4px; }
        #servoActive { color: #e65100; font-weight: bold; font-size: 10pt; padding: 4px; }

        #boxCount   { font-size: 14pt; font-weight: bold; color: #1565c0; }
        #totalCount { font-size: 11pt; font-weight: bold; color: #1a237e; padding-top: 4px; }
        #distLabel  { font-size: 10pt; color: #455a64; padding-top: 6px; }

        #logGroup {
            background: #eceff1;
            margin: 0px 4px 4px 4px;
        }
        #logText {
            color: #37474f;
            font-family: "Consolas", "Courier New", monospace;
            font-size: 8pt;
            background: #fafafa;
            border: 1px solid #cfd8dc;
            border-radius: 4px;
            padding: 4px;
        }

        QTableWidget {
            border: 1px solid #cfd8dc;
            border-radius: 4px;
            gridline-color: #eceff1;
            font-size: 9pt;
        }
        QHeaderView::section {
            background: #37474f;
            color: #ffffff;
            padding: 6px;
            border: none;
            font-weight: bold;
            font-size: 9pt;
        }
    )");
}

/* =========================================================================
 * Navegacion
 * ====================================================================== */

void MainWindow::setPage(int index) {
    pages->setCurrentIndex(index);
    btnPageCalib->setChecked(index == 0);
    btnPageClassif->setChecked(index == 1);
}

/* =========================================================================
 * Conexion serie
 * ====================================================================== */

void MainWindow::onRefreshPorts() {
    cmbPort->clear();
    cmbPort->addItems(serial->availablePorts());
}

void MainWindow::onConnectClicked() {
    if (serial->isConnected()) {
        serial->disconnectPort();
    } else {
        if (cmbPort->currentText().isEmpty()) {
            QMessageBox::warning(this, "Error", "No hay puertos disponibles.");
            return;
        }
        serial->connectPort(cmbPort->currentText());
    }
}

void MainWindow::onSerialConnected() {
    btnConnect->setText("Desconectar");
    btnConnect->setStyleSheet("background:#b71c1c; color:#fff; border:none; font-weight:bold; padding:6px 20px; border-radius:6px;");
    lblStatus->setText("  Conectado");
    lblStatus->setObjectName("statusConnected");
    lblStatus->style()->unpolish(lblStatus);
    lblStatus->style()->polish(lblStatus);
    log("Conectado al puerto " + cmbPort->currentText());
}

void MainWindow::onSerialDisconnected() {
    btnConnect->setText("Conectar");
    btnConnect->setStyleSheet("");
    btnConnect->setObjectName("connectBtn");
    lblStatus->setText("  Desconectado");
    lblStatus->setObjectName("statusDisconnected");
    lblStatus->style()->unpolish(lblStatus);
    lblStatus->style()->polish(lblStatus);
    log("Desconectado.");
}

void MainWindow::onSerialError(const QString &msg) {
    QMessageBox::critical(this, "Error de conexion", msg);
}

/* =========================================================================
 * Recepcion de tramas del MCU
 * ====================================================================== */

void MainWindow::onFrameReceived(uint8_t cmdId, QByteArray payload) {

    /* Registrar trama recibida (excepto heartbeat para no saturar) */
    if (cmdId != EVT_ALIVE) {
        QString hex;
        for (int i = 0; i < payload.size(); i++)
            hex += QString("%1 ").arg(static_cast<uint8_t>(payload[i]), 2, 16, QChar('0')).toUpper();
        log(QString("RX [CMD=0x%1] payload: %2")
            .arg(cmdId, 2, 16, QChar('0')).toUpper()
            .arg(hex.isEmpty() ? "(vacio)" : hex.trimmed()));
    }

    switch (cmdId) {

    case EVT_ALIVE:
        /* Heartbeat recibido, MCU vivo */
        break;

    case EVT_NEW_BOX:
        if (payload.size() >= 4) {
            uint8_t type  = static_cast<uint8_t>(payload[0]);
            uint8_t servo = static_cast<uint8_t>(payload[1]);
            uint16_t ticks = (static_cast<uint8_t>(payload[2]) << 8)
                           |  static_cast<uint8_t>(payload[3]);
            addBoxToQueue(type, servo, ticks);
            log(QString("Caja detectada: %1 cm -> Servo %2").arg(type).arg(servo+1));
        }
        break;

    case EVT_IR_SENSOR:
        if (payload.size() >= 2) {
            int sensor = static_cast<uint8_t>(payload[0]);
            bool state = static_cast<uint8_t>(payload[1]) != 0;
            if (sensor < 4) setSensorLed(sensor, state);
        }
        break;

    case EVT_SERVO_FIRED:
        if (payload.size() >= 1) {
            uint8_t servo = static_cast<uint8_t>(payload[0]);
            if (servo < 3) {
                lblServoState[servo]->setText(QString("Servo %1: ACTIVO").arg(servo+1));
                lblServoState[servo]->setObjectName("servoActive");
                lblServoState[servo]->style()->unpolish(lblServoState[servo]);
                lblServoState[servo]->style()->polish(lblServoState[servo]);

                /* Incrementar conteo segun el tipo asignado a esta salida */
                int typeIdx = cmbOutputType[servo]->currentIndex();
                boxCounts[typeIdx]++;
                lblBoxCount[typeIdx]->setText(QString::number(boxCounts[typeIdx]));
                int total = boxCounts[0] + boxCounts[1] + boxCounts[2];
                lblTotalBoxes->setText(QString("Total: %1").arg(total));

                removeBoxByServo(servo);
                log(QString("Servo %1 disparo - caja clasificada.").arg(servo+1));
            }
        }
        break;

    case EVT_DISTANCE:
        if (payload.size() >= 2) {
            uint16_t dist = (static_cast<uint8_t>(payload[0]) << 8)
                          |  static_cast<uint8_t>(payload[1]);
            lblDistance->setText(QString("Distancia: %1 cm").arg(dist));
        }
        break;

    case EVT_CALIB_TIME:
        if (payload.size() >= 3) {
            uint8_t sensor = static_cast<uint8_t>(payload[0]);
            uint16_t ticks = (static_cast<uint8_t>(payload[1]) << 8)
                           |  static_cast<uint8_t>(payload[2]);
            double secs = ticks * 0.002;
            if (sensor < 3) {
                lblCalibTime[sensor]->setText(QString("%1 s").arg(secs, 0, 'f', 2));
                /* Actualizar tambien el spinbox de la pagina clasificacion */
                spnOutputTime[sensor]->setValue(secs);
                log(QString("Calibracion S%1: %2 s (%3 ticks)")
                    .arg(sensor+1).arg(secs, 0, 'f', 2).arg(ticks));
            }
        }
        break;
    }
}

/* =========================================================================
 * Slots de Calibracion
 * ====================================================================== */

void MainWindow::onStartCalibration() {
    calibMode = !calibMode;
    QByteArray data;
    data.append(static_cast<char>(calibMode ? 1 : 0));
    serial->sendCommand(CMD_CALIB_MODE, data);

    if (calibMode) {
        btnStartCalib->setText("Detener Calibracion");
        btnStartCalib->setObjectName("warningBtn");
        lblCalibState->setText("Calibrando: coloque una caja en el inicio de la cinta...");
    } else {
        btnStartCalib->setText("Iniciar Calibracion");
        btnStartCalib->setObjectName("primaryBtn");
        lblCalibState->setText("Calibracion detenida.");
    }
    btnStartCalib->style()->unpolish(btnStartCalib);
    btnStartCalib->style()->polish(btnStartCalib);
}

void MainWindow::onResetDefaultTimes() {
    double defaults[3] = {2.0, 4.0, 6.0};
    int tickDefaults[3] = {DEFAULT_TIME_SERVO0, DEFAULT_TIME_SERVO1, DEFAULT_TIME_SERVO2};

    for (int i = 0; i < 3; i++) {
        lblCalibTime[i]->setText(QString("%1 s").arg(defaults[i], 0, 'f', 2));
        spnOutputTime[i]->setValue(defaults[i]);

        /* Enviar al MCU */
        QByteArray data;
        data.append(static_cast<char>(i));
        data.append(static_cast<char>((tickDefaults[i] >> 8) & 0xFF));
        data.append(static_cast<char>(tickDefaults[i] & 0xFF));
        serial->sendCommand(CMD_SET_TIMES, data);
    }
    log("Tiempos reseteados a valores por defecto.");
}

void MainWindow::onApplyThresholds() {
    QByteArray data;
    data.append(static_cast<char>(spnThreshGrande->value()));
    data.append(static_cast<char>(spnThreshMediana->value()));
    data.append(static_cast<char>(spnThreshPequena->value()));
    serial->sendCommand(CMD_SET_THRESHOLD, data);
    log(QString("Umbrales aplicados: Grande<%1, Mediana<%2, Pequena<%3")
        .arg(spnThreshGrande->value())
        .arg(spnThreshMediana->value())
        .arg(spnThreshPequena->value()));
}

void MainWindow::onTestServo(int servo) {
    QByteArray data;
    data.append(static_cast<char>(servo));
    serial->sendCommand(CMD_TEST_SERVO, data);
    log(QString("Test: pateando servo %1").arg(servo+1));
}

void MainWindow::onApplyServoAngle() {
    /* Convertir grados a ticks de 0.5ms: 0°=2, 90°=3, 180°=4 */
    for (int i = 0; i < 3; i++) {
        int deg = spnServoAngle[i]->value();
        uint8_t pulseTicks = 2 + (deg * 2 / 180); // 0°->2, 90°->3, 180°->4
        QByteArray data;
        data.append(static_cast<char>(i));
        data.append(static_cast<char>(pulseTicks));
        serial->sendCommand(CMD_SET_SERVO_ANGLE, data);
    }
    log("Angulos de servo aplicados.");
}

/* =========================================================================
 * Slots de Clasificacion
 * ====================================================================== */

void MainWindow::onStart() {
    onApplyOutputConfig(); /* Enviar config primero */
    serial->sendCommand(CMD_START);
    beltRunning = true;
    log("Cinta iniciada.");
}

void MainWindow::onStop() {
    serial->sendCommand(CMD_STOP);
    beltRunning = false;
    log("Cinta detenida.");
}

void MainWindow::onReset() {
    serial->sendCommand(CMD_RESET);
    beltRunning = false;
    boxQueue.clear();
    for (int i = 0; i < 3; i++) {
        boxCounts[i] = 0;
        lblBoxCount[i]->setText("0");
    }
    lblTotalBoxes->setText("Total: 0");
    nextBoxId = 1;
    updateQueueTable();
    log("Sistema reseteado.");
}

void MainWindow::onApplyOutputConfig() {
    /* Enviar boxType */
    uint8_t types[3] = {6, 8, 10};
    QByteArray cfgData;
    for (int i = 0; i < 3; i++) {
        cfgData.append(static_cast<char>(types[cmbOutputType[i]->currentIndex()]));
    }
    serial->sendCommand(CMD_SET_CONFIG, cfgData);

    /* Enviar tiempos */
    for (int i = 0; i < 3; i++) {
        uint16_t ticks = static_cast<uint16_t>(spnOutputTime[i]->value() / 0.002);
        QByteArray tData;
        tData.append(static_cast<char>(i));
        tData.append(static_cast<char>((ticks >> 8) & 0xFF));
        tData.append(static_cast<char>(ticks & 0xFF));
        serial->sendCommand(CMD_SET_TIMES, tData);
    }
    log("Configuracion de salidas aplicada.");
}

/* =========================================================================
 * Cola de cajas
 * ====================================================================== */

void MainWindow::addBoxToQueue(uint8_t type, uint8_t servo, uint16_t timeTicks) {
    BoxInTransit box;
    box.id = nextBoxId++;
    box.type = type;
    box.servo = servo;
    box.totalTime = timeTicks * 0.002;
    box.remaining = box.totalTime;
    boxQueue.append(box);
    updateQueueTable();
}

void MainWindow::removeBoxByServo(uint8_t servo) {
    for (int i = 0; i < boxQueue.size(); i++) {
        if (boxQueue[i].servo == servo) {
            boxQueue.removeAt(i);
            break;
        }
    }
    updateQueueTable();
}

void MainWindow::updateQueueTable() {
    tblQueue->setRowCount(boxQueue.size());
    for (int i = 0; i < boxQueue.size(); i++) {
        const auto &box = boxQueue[i];
        tblQueue->setItem(i, 0, new QTableWidgetItem(QString::number(box.id)));
        tblQueue->setItem(i, 1, new QTableWidgetItem(QString("%1 cm").arg(box.type)));
        tblQueue->setItem(i, 2, new QTableWidgetItem(QString("Servo %1").arg(box.servo+1)));
        tblQueue->setItem(i, 3, new QTableWidgetItem(QString("%1 s").arg(box.remaining, 0, 'f', 2)));
    }
}

/* =========================================================================
 * Refresco periodico (10 Hz)
 * ====================================================================== */

void MainWindow::onRefreshTick() {
    /* Decrementar timers de la cola localmente */
    if (beltRunning) {
        for (auto &box : boxQueue) {
            box.remaining -= 0.1;
            if (box.remaining < 0) box.remaining = 0;
        }
        updateQueueTable();
    }

    /* Resetear LEDs de servo despues de un tiempo */
    for (int i = 0; i < 3; i++) {
        if (lblServoState[i]->objectName() == "servoActive") {
            /* Se resetea al siguiente tick (simplificado) */
        }
    }
}

/* =========================================================================
 * Utilidades
 * ====================================================================== */

void MainWindow::setSensorLed(int idx, bool active) {
    if (idx < 0 || idx >= 4) return;
    lblSensor[idx]->setText(active ? "ON" : "OFF");
    lblSensor[idx]->setObjectName(active ? "ledOn" : "ledOff");
    lblSensor[idx]->style()->unpolish(lblSensor[idx]);
    lblSensor[idx]->style()->polish(lblSensor[idx]);
}

void MainWindow::log(const QString &msg) {
    QString time = QTime::currentTime().toString("HH:mm:ss.zzz");
    txtLog->append(QString("[%1] %2").arg(time, msg));

    /* Limitar a 300 lineas para evitar consumo excesivo de memoria */
    while (txtLog->document()->blockCount() > 300) {
        QTextCursor cursor = txtLog->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.deleteChar();
    }

    /* Auto-scroll al final */
    txtLog->verticalScrollBar()->setValue(txtLog->verticalScrollBar()->maximum());
}
