/**
 * @file    serialmanager.cpp
 * @brief   Implementacion de la comunicacion serie con protocolo UNER.
 * @author  Luciano
 * @date    Mayo 2026
 */

#include "serialmanager.h"

SerialManager::SerialManager(QObject *parent)
    : QObject(parent)
    , port(new QSerialPort(this))
    , aliveTimer(new QTimer(this))
{
    connect(port, &QSerialPort::readyRead, this, &SerialManager::onReadyRead);
    connect(aliveTimer, &QTimer::timeout, this, &SerialManager::onAliveTimer);
}

SerialManager::~SerialManager() {
    disconnectPort();
}

QStringList SerialManager::availablePorts() const {
    QStringList list;
    for (const auto &info : QSerialPortInfo::availablePorts())
        list << info.portName();
    return list;
}

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

void SerialManager::disconnectPort() {
    aliveTimer->stop();
    if (port->isOpen()) {
        port->close();
        emit disconnected();
    }
}

bool SerialManager::isConnected() const {
    return port->isOpen();
}

/* ---- Envio de tramas UNER ---- */

void SerialManager::sendCommand(uint8_t cmdId, const QByteArray &data) {
    if (!port->isOpen()) return;

    uint8_t totalLen = 1 + data.size() + 1; // CMDID + data + CKS

    uint8_t cks = 'U' ^ 'N' ^ 'E' ^ 'R' ^ totalLen ^ ':' ^ cmdId;
    for (auto b : data) cks ^= static_cast<uint8_t>(b);

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

/* ---- Recepcion y decodificacion ---- */

void SerialManager::onReadyRead() {
    QByteArray raw = port->readAll();
    for (auto b : raw)
        processRxByte(static_cast<uint8_t>(b));
}

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
            /* Byte de checksum */
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

/* ---- Heartbeat ---- */

void SerialManager::onAliveTimer() {
    sendCommand(CMD_ALIVE);
}
