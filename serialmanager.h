/**
 * @file    serialmanager.h
 * @brief   Modulo de comunicacion serie con protocolo UNER para el HMI Qt.
 *
 * @details Encapsula la comunicacion con el ATmega328P a traves de un puerto
 *          serie (115200 baud, 8N1). Implementa:
 *          - Envio de tramas UNER con checksum XOR.
 *          - Recepcion y decodificacion mediante FSM identica a la del MCU.
 *          - Heartbeat periodico (CMD_ALIVE) cada 500 ms.
 *
 *          Formato de trama UNER:
 *          @code
 *          [ 'U' | 'N' | 'E' | 'R' | LEN | ':' | CMDID | DATA... | CKS ]
 *          @endcode
 *          donde LEN = 1 (CMDID) + N (datos) + 1 (CKS), y
 *          CKS = XOR de todos los bytes desde 'U' hasta el ultimo dato.
 *
 * @author  Luciano
 * @date    Mayo 2026
 */

#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>
#include <QTimer>
#include "protocol.h"

/**
 * @brief Clase que gestiona la comunicacion serie con el MCU.
 *
 * @details Emite senales Qt al conectar, desconectar, recibir tramas
 *          validas o detectar errores de conexion.
 */
class SerialManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructor. Crea el QSerialPort y el timer de heartbeat.
     * @param parent Widget padre (ownership de Qt).
     */
    explicit SerialManager(QObject *parent = nullptr);

    /** @brief Destructor. Cierra el puerto si esta abierto. */
    ~SerialManager();

    /**
     * @brief Retorna la lista de puertos serie disponibles en el sistema.
     * @return QStringList con los nombres de los puertos (ej: "COM3", "COM5").
     */
    QStringList availablePorts() const;

    /**
     * @brief Abre el puerto serie especificado a 115200 baud, 8N1.
     * @param portName Nombre del puerto (ej: "COM3").
     * @return true si la conexion fue exitosa, false en caso de error.
     */
    bool connectPort(const QString &portName);

    /**
     * @brief Cierra el puerto serie y detiene el heartbeat.
     */
    void disconnectPort();

    /**
     * @brief Consulta si el puerto serie esta abierto.
     * @return true si esta conectado, false en caso contrario.
     */
    bool isConnected() const;

    /**
     * @brief Envia un comando al MCU encapsulado en una trama UNER.
     * @param cmdId Identificador del comando (ver Protocol.h).
     * @param data  Datos adicionales del comando (puede estar vacio).
     */
    void sendCommand(uint8_t cmdId, const QByteArray &data = {});

signals:
    /** @brief Emitida al conectar exitosamente al puerto serie. */
    void connected();

    /** @brief Emitida al desconectar del puerto serie. */
    void disconnected();

    /**
     * @brief Emitida al recibir una trama UNER valida con checksum correcto.
     * @param cmdId   Identificador del comando/evento recibido.
     * @param payload Datos del payload (sin CMDID ni CKS).
     */
    void frameReceived(uint8_t cmdId, QByteArray payload);

    /**
     * @brief Emitida cuando ocurre un error al abrir el puerto.
     * @param msg Descripcion del error.
     */
    void connectionError(const QString &msg);

private slots:
    /** @brief Slot invocado por QSerialPort al recibir datos. */
    void onReadyRead();

    /** @brief Slot del timer de heartbeat: envia CMD_ALIVE cada 500 ms. */
    void onAliveTimer();

private:
    /**
     * @brief Procesa un byte recibido a traves de la FSM de decodificacion UNER.
     * @param byte Byte recibido del puerto serie.
     */
    void processRxByte(uint8_t byte);

    QSerialPort *port;       /**< Instancia del puerto serie Qt.               */
    QTimer      *aliveTimer; /**< Timer para envio periodico de heartbeat.     */

    /** @brief Estados de la FSM de decodificacion de tramas UNER. */
    enum FsmState {
        WAIT_U,        /**< Esperando caracter 'U' del encabezado. */
        WAIT_N,        /**< Esperando caracter 'N'.                */
        WAIT_E,        /**< Esperando caracter 'E'.                */
        WAIT_R,        /**< Esperando caracter 'R'.                */
        WAIT_LEN,      /**< Esperando byte de longitud (LEN).      */
        WAIT_COLON,    /**< Esperando separador ':'.                */
        RECV_PAYLOAD   /**< Recibiendo payload y checksum final.    */
    };

    FsmState   fsmState = WAIT_U; /**< Estado actual de la FSM RX.               */
    uint8_t    fsmLen   = 0;      /**< Bytes restantes del payload en curso.      */
    uint8_t    fsmCks   = 0;      /**< Checksum acumulado durante recepcion.      */
    QByteArray fsmPayload;        /**< Buffer de payload en construccion.         */
};

#endif // SERIALMANAGER_H
