/**
 * @file    serialmanager.h
 * @brief   Comunicacion serie con protocolo UNER.
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

class SerialManager : public QObject {
    Q_OBJECT

public:
    explicit SerialManager(QObject *parent = nullptr);
    ~SerialManager();

    QStringList availablePorts() const;
    bool connectPort(const QString &portName);
    void disconnectPort();
    bool isConnected() const;

    void sendCommand(uint8_t cmdId, const QByteArray &data = {});

signals:
    void connected();
    void disconnected();
    void frameReceived(uint8_t cmdId, QByteArray payload);
    void connectionError(const QString &msg);

private slots:
    void onReadyRead();
    void onAliveTimer();

private:
    void processRxByte(uint8_t byte);

    QSerialPort *port;
    QTimer *aliveTimer;

    /* FSM de decodificacion UNER */
    enum FsmState { WAIT_U, WAIT_N, WAIT_E, WAIT_R, WAIT_LEN, WAIT_COLON, RECV_PAYLOAD };
    FsmState fsmState = WAIT_U;
    uint8_t fsmLen    = 0;
    uint8_t fsmCks    = 0;
    QByteArray fsmPayload;
};

#endif // SERIALMANAGER_H
