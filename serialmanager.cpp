/**
 * @file    serialmanager.cpp
 * @brief   Implementacion del modulo de comunicacion serie con protocolo UNER.
 *
 * @details Gestiona la conexion fisica al puerto serie (115200, 8N1) y la
 *          codificacion/decodificacion de tramas UNER. Envio de heartbeat
 *          periodico cada 500 ms para mantener la conexion activa.
 *
 *          La FSM de recepcion es identica a la implementada en Comm.c del
 *          firmware, garantizando compatibilidad bidireccional.
 *
 * @author  Luciano
 * @date    Mayo 2026
 */

#include "serialmanager.h"

/* =========================================================================
 * Constructor / Destructor
 * ====================================================================== */

/**
 * @brief Constructor. Inicializa el puerto serie y el timer de heartbeat.
 * @param parent Widget padre para el sistema de ownership de Qt.
 */
SerialManager::SerialManager(QObject *parent)
    : QObject(parent)
    , port(new QSerialPort(this))
    , aliveTimer(new QTimer(this))
{
    connect(port, &QSerialPort::readyRead, this, &SerialManager::onReadyRead);
    connect(aliveTimer, &QTimer::timeout, this, &SerialManager::onAliveTimer);
}

/**
 * @brief Destructor. Cierra el puerto serie si esta abierto.
 */
SerialManager::~SerialManager() {
    disconnectPort();
}

/* =========================================================================
 * Gestion de conexion
 * ====================================================================== */

/**
 * @brief Retorna la lista de puertos serie disponibles en el sistema.
 * @return QStringList con los nombres de los puertos detectados.
 */
QStringList SerialManager::availablePorts() const {
    QStringList list;
    for (const auto &info : QSerialPortInfo::availablePorts())
        list << info.portName();
    return list;
}

/**
 * @brief Abre el puerto serie con la configuracion requerida por el MCU.
 *
 * @details Configuracion: 115200 baud, 8 bits de datos, sin paridad, 1 stop bit.
 *          Al conectar exitosamente, reinicia la FSM de recepcion e inicia
 *          el timer de heartbeat (500 ms).
 *
 * @param portName Nombre del puerto (ej: "COM3").
 * @return true si la conexion fue exitosa, false si fallo.
 */
bool SerialManager::connectPort(const QString &portName) {
    if (port->isOpen()) port->close();

    port->setPortName(portName);
    port->setBaudRate(115200);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);

    if (!port->open(QIODevice::ReadWrite)) {
        emit connectionError(port->errorString());
        return false;
    }

    fsmState = WAIT_U;
    aliveTimer->start(500);
    emit connected();
    return true;
}

/**
 * @brief Cierra el puerto serie y detiene el heartbeat.
 *
 * @details Emite la senal disconnected() si el puerto estaba abierto.
 */
void SerialManager::disconnectPort() {
    aliveTimer->stop();
    if (port->isOpen()) {
        port->close();
        emit disconnected();
    }
}

/**
 * @brief Consulta si el puerto serie esta abierto.
 * @return true si el puerto esta abierto y listo para comunicar.
 */
bool SerialManager::isConnected() const {
    return port->isOpen();
}

/* =========================================================================
 * Envio de tramas UNER
 * ====================================================================== */

/**
 * @brief Construye y envia una trama UNER completa al MCU.
 *
 * @details Formato de trama:
 *          @code
 *          [ 'U' | 'N' | 'E' | 'R' | LEN | ':' | CMDID | DATA... | CKS ]
 *          @endcode
 *          LEN = 1 (CMDID) + data.size() + 1 (CKS).
 *          CKS = XOR de todos los bytes desde 'U' hasta el ultimo dato.
 *
 * @param cmdId Identificador del comando a enviar.
 * @param data  Bytes de datos adicionales (puede estar vacio).
 */
void SerialManager::sendCommand(uint8_t cmdId, const QByteArray &data) {
    if (!port->isOpen()) return;

    uint8_t totalLen = 1 + data.size() + 1; /* CMDID + data + CKS */

    /* Calcular checksum XOR */
    uint8_t cks = 'U' ^ 'N' ^ 'E' ^ 'R' ^ totalLen ^ ':' ^ cmdId;
    for (auto b : data) cks ^= static_cast<uint8_t>(b);

    /* Construir trama completa */
    QByteArray frame;
    frame.append('U');
    frame.append('N');
    frame.append('E');
    frame.append('R');
    frame.append(static_cast<char>(totalLen));
    frame.append(':');
    frame.append(static_cast<char>(cmdId));
    frame.append(data);
    frame.append(static_cast<char>(cks));

    port->write(frame);
}

/* =========================================================================
 * Recepcion y decodificacion
 * ====================================================================== */

/**
 * @brief Slot invocado por QSerialPort cuando hay datos disponibles.
 *
 * @details Lee todos los bytes disponibles y los pasa uno a uno a la FSM
 *          de decodificacion UNER.
 */
void SerialManager::onReadyRead() {
    QByteArray raw = port->readAll();
    for (auto b : raw)
        processRxByte(static_cast<uint8_t>(b));
}

/**
 * @brief Procesa un byte recibido a traves de la FSM de decodificacion UNER.
 *
 * @details La FSM sincroniza con el encabezado "UNER", captura LEN, el
 *          separador ':', y finalmente el payload byte a byte hasta verificar
 *          el checksum. Al completar una trama valida, emite frameReceived().
 *
 * @param byte Byte recibido del puerto serie.
 */
void SerialManager::processRxByte(uint8_t byte) {
    switch (fsmState) {
    case WAIT_U:
        if (byte == 'U') fsmState = WAIT_N;
        break;
    case WAIT_N:
        fsmState = (byte == 'N') ? WAIT_E : WAIT_U;
        break;
    case WAIT_E:
        fsmState = (byte == 'E') ? WAIT_R : WAIT_U;
        break;
    case WAIT_R:
        fsmState = (byte == 'R') ? WAIT_LEN : WAIT_U;
        break;
    case WAIT_LEN:
        fsmLen = byte;
        fsmCks = 'U' ^ 'N' ^ 'E' ^ 'R' ^ byte ^ ':';
        fsmPayload.clear();
        fsmState = WAIT_COLON;
        break;
    case WAIT_COLON:
        fsmState = (byte == ':') ? RECV_PAYLOAD : WAIT_U;
        break;
    case RECV_PAYLOAD:
        if (fsmLen > 1) {
            fsmPayload.append(static_cast<char>(byte));
            fsmCks ^= byte;
            fsmLen--;
        } else {
            /* Byte de checksum: validar e invocar senal */
            if (fsmCks == byte && fsmPayload.size() >= 1) {
                uint8_t cmdId = static_cast<uint8_t>(fsmPayload.at(0));
                QByteArray data = fsmPayload.mid(1);
                emit frameReceived(cmdId, data);
            }
            fsmState = WAIT_U;
        }
        break;
    }
}

/* =========================================================================
 * Heartbeat
 * ====================================================================== */

/**
 * @brief Envia un comando CMD_ALIVE al MCU como heartbeat periodico.
 *
 * @details Se ejecuta cada 500 ms mediante el QTimer aliveTimer.
 *          El MCU responde con EVT_ALIVE para confirmar que esta operativo.
 */
void SerialManager::onAliveTimer() {
    sendCommand(CMD_ALIVE);
}
