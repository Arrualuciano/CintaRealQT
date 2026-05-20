/**
 * @file    mainwindow.h
 * @brief   Ventana principal del HMI para la cinta clasificadora.
 *
 * @details Interfaz grafica de usuario (GUI) construida con Qt que permite:
 *          - Conexion/desconexion al puerto serie del ATmega328P.
 *          - Modo Calibracion: medir tiempos de arribo S0->S1/S2/S3,
 *            test manual de servos, ajuste de angulos y umbrales de distancia.
 *          - Modo Clasificacion: iniciar/detener la cinta, configurar
 *            tipos de caja por salida, visualizar estado de sensores IR,
 *            distancia medida, servos activos y conteo de cajas clasificadas.
 *          - Log de comunicacion en tiempo real con formato timestamp.
 *
 *          La interfaz se divide en:
 *          - Toolbar superior: conexion serie y estado.
 *          - Sidebar izquierdo: navegacion entre paginas.
 *          - Area central: paginas de Calibracion y Clasificacion.
 *          - Panel inferior: log de comunicacion (siempre visible).
 *
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

/**
 * @brief Representa una caja en transito sobre la cinta transportadora.
 *
 * @details Se crea al recibir EVT_NEW_BOX del MCU y se elimina al recibir
 *          EVT_SERVO_FIRED para el servo correspondiente.
 */
struct BoxInTransit {
    int     id;         /**< Identificador secuencial unico de la caja.         */
    uint8_t type;       /**< Tipo de caja (altura en cm: 6, 8 o 10).            */
    uint8_t servo;      /**< Servo destino asignado (0, 1 o 2).                 */
    double  totalTime;  /**< Tiempo total de arribo en segundos.                */
    double  remaining;  /**< Tiempo restante estimado en segundos.              */
};

/**
 * @brief Ventana principal del HMI de la cinta clasificadora.
 *
 * @details Hereda de QMainWindow. Gestiona toda la interfaz grafica,
 *          la comunicacion con el MCU via SerialManager, y el estado
 *          local de la aplicacion (conteo de cajas, cola de transito, etc.).
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Constructor. Construye la UI, aplica estilos y conecta senales.
     * @param parent Widget padre (nullptr para ventana independiente).
     */
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    /* --- Conexion serie --- */

    /** @brief Slot del boton Conectar/Desconectar. */
    void onConnectClicked();

    /** @brief Refresca la lista de puertos serie disponibles. */
    void onRefreshPorts();

    /** @brief Slot invocado al conectar exitosamente al puerto. */
    void onSerialConnected();

    /** @brief Slot invocado al desconectar del puerto. */
    void onSerialDisconnected();

    /**
     * @brief Slot invocado al ocurrir un error de conexion.
     * @param msg Descripcion del error.
     */
    void onSerialError(const QString &msg);

    /**
     * @brief Despacha las tramas recibidas del MCU y actualiza la UI.
     * @param cmdId   Identificador del evento recibido.
     * @param payload Datos del evento.
     */
    void onFrameReceived(uint8_t cmdId, QByteArray payload);

    /* --- Calibracion --- */

    /** @brief Inicia o detiene el modo calibracion en el MCU. */
    void onStartCalibration();

    /** @brief Resetea los tiempos de arribo a los valores por defecto (2s/4s/6s). */
    void onResetDefaultTimes();

    /** @brief Envia los umbrales de distancia configurados al MCU. */
    void onApplyThresholds();

    /**
     * @brief Dispara un servo manualmente para test.
     * @param servo Numero de servo (0, 1 o 2).
     */
    void onTestServo(int servo);

    /** @brief Envia los angulos de servo configurados al MCU. */
    void onApplyServoAngle();

    /* --- Clasificacion --- */

    /** @brief Inicia la clasificacion: envia configuracion y CMD_START al MCU. */
    void onStart();

    /** @brief Detiene la clasificacion: envia CMD_STOP al MCU. */
    void onStop();

    /** @brief Resetea el sistema: envia CMD_RESET y limpia contadores locales. */
    void onReset();

    /** @brief Envia la configuracion de tipos de caja y tiempos al MCU. */
    void onApplyOutputConfig();

    /** @brief Timer de refresco de UI a 10 Hz: actualiza cola de cajas. */
    void onRefreshTick();

private:
    /* --- Construccion de UI --- */

    /** @brief Construye todos los widgets de la interfaz. */
    void buildUi();

    /** @brief Construye la barra de herramientas superior. */
    QWidget *buildToolbar();

    /** @brief Construye el panel lateral de navegacion. */
    QWidget *buildSidebar();

    /** @brief Construye la pagina de Calibracion. */
    QWidget *buildCalibrationPage();

    /** @brief Construye la pagina de Clasificacion. */
    QWidget *buildClassificationPage();

    /** @brief Aplica el stylesheet con soporte High DPI (unidades en pt). */
    void applyStyle();

    /**
     * @brief Cambia la pagina visible en el area central.
     * @param index 0 = Calibracion, 1 = Clasificacion.
     */
    void setPage(int index);

    /* --- Gestion de cola de cajas --- */

    /**
     * @brief Agrega una caja a la cola de transito.
     * @param type      Tipo de caja (6, 8 o 10 cm).
     * @param servo     Servo destino (0, 1 o 2).
     * @param timeTicks Tiempo de arribo en ticks de 2 ms.
     */
    void addBoxToQueue(uint8_t type, uint8_t servo, uint16_t timeTicks);

    /**
     * @brief Elimina la primera caja asignada al servo indicado.
     * @param servo Numero de servo (0, 1 o 2).
     */
    void removeBoxByServo(uint8_t servo);

    /** @brief Refresca la tabla visual de la cola de cajas. */
    void updateQueueTable();

    /* --- Utilidades --- */

    /**
     * @brief Actualiza el indicador LED de un sensor IR en la UI.
     * @param idx    Indice del sensor (0 = S0, 1 = S1, 2 = S2, 3 = S3).
     * @param active true = sensor detectando objeto, false = libre.
     */
    void setSensorLed(int idx, bool active);

    /**
     * @brief Agrega un mensaje al log de comunicacion con timestamp.
     * @param msg Mensaje a registrar.
     */
    void log(const QString &msg);

    /* --- Miembros --- */

    SerialManager *serial;       /**< Gestor de comunicacion serie.              */
    QTimer        *refreshTimer; /**< Timer de refresco de UI (10 Hz).           */

    /* Toolbar */
    QComboBox   *cmbPort;        /**< Selector de puerto serie.                  */
    QPushButton *btnConnect;     /**< Boton Conectar/Desconectar.                */
    QLabel      *lblStatus;      /**< Indicador de estado de conexion.           */

    /* Sidebar */
    QPushButton    *btnPageCalib;   /**< Boton de navegacion a Calibracion.      */
    QPushButton    *btnPageClassif; /**< Boton de navegacion a Clasificacion.    */
    QStackedWidget *pages;         /**< Widget apilado para las paginas.         */

    /* --- Pagina Calibracion --- */
    QLabel      *lblCalibState;       /**< Label de estado de calibracion.       */
    QLabel      *lblCalibTime[3];     /**< Labels de tiempos calibrados S1/S2/S3.*/
    QPushButton *btnStartCalib;       /**< Boton Iniciar/Detener Calibracion.    */
    QPushButton *btnResetDefaults;    /**< Boton de reset a valores por defecto. */
    QPushButton *btnTestServo[3];     /**< Botones de test de servo.             */
    QSpinBox    *spnServoAngle[3];    /**< Spinboxes de angulo de servo.         */
    QPushButton *btnApplyAngle;       /**< Boton para aplicar angulos.           */
    QSpinBox    *spnThreshGrande;     /**< Spinbox umbral caja grande.           */
    QSpinBox    *spnThreshMediana;    /**< Spinbox umbral caja mediana.          */
    QSpinBox    *spnThreshPequena;    /**< Spinbox umbral caja pequena.          */
    QPushButton *btnApplyThresh;      /**< Boton para aplicar umbrales.          */

    /* --- Pagina Clasificacion --- */
    QPushButton    *btnStart;            /**< Boton Iniciar clasificacion.        */
    QPushButton    *btnStop;             /**< Boton Detener clasificacion.        */
    QPushButton    *btnReset;            /**< Boton Reset del sistema.            */
    QComboBox      *cmbOutputType[3];    /**< Combo tipo de caja por salida.      */
    QDoubleSpinBox *spnOutputTime[3];    /**< Spinbox tiempo de arribo por salida.*/
    QPushButton    *btnApplyConfig;      /**< Boton aplicar configuracion.        */
    QLabel         *lblSensor[4];        /**< LEDs indicadores de sensores IR.    */
    QLabel         *lblServoState[3];    /**< Labels de estado de servos.         */
    QLabel         *lblDistance;         /**< Label de distancia medida.          */
    QLabel         *lblBoxCount[3];      /**< Labels de conteo por tipo de caja.  */
    QLabel         *lblTotalBoxes;       /**< Label de conteo total de cajas.     */
    QTableWidget   *tblQueue;            /**< Tabla de cajas en transito.         */
    QTextEdit      *txtLog;              /**< Area de log de comunicacion.        */

    /* --- Estado de la aplicacion --- */
    bool calibMode   = false;       /**< true si esta en modo calibracion.       */
    bool beltRunning = false;       /**< true si la cinta esta en marcha.        */
    int  boxCounts[3] = {0, 0, 0};  /**< Conteo de cajas por tipo [peq,med,gra]. */
    int  nextBoxId = 1;             /**< Siguiente ID secuencial para cajas.     */
    QList<BoxInTransit> boxQueue;   /**< Cola local de cajas en transito.        */
    QElapsedTimer calibElapsed;     /**< Timer para medir duracion de calibracion.*/
};

#endif // MAINWINDOW_H
