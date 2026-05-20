/**
 * @file    protocol.h
 * @brief   Definiciones del protocolo de comunicacion UNER entre MCU y HMI Qt.
 * @author  Luciano
 * @date    Mayo 2026
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

/* =========================================================================
 * Comandos Qt -> MCU
 * ====================================================================== */
constexpr uint8_t CMD_ALIVE           = 0xF0;
constexpr uint8_t CMD_START           = 0x50;
constexpr uint8_t CMD_STOP            = 0x51;
constexpr uint8_t CMD_RESET           = 0x53;
constexpr uint8_t CMD_SET_CONFIG      = 0x60;  // [boxType0, boxType1, boxType2]
constexpr uint8_t CMD_SET_TIMES       = 0x61;  // [servo, timeHi, timeLo]
constexpr uint8_t CMD_SET_THRESHOLD   = 0x62;  // [grande, mediana, pequena]
constexpr uint8_t CMD_TEST_SERVO      = 0x63;  // [servoNum]
constexpr uint8_t CMD_SET_SERVO_ANGLE = 0x64;  // [pulseMin, pulseMax]
constexpr uint8_t CMD_CALIB_MODE      = 0x65;  // [1=enter, 0=exit]

/* =========================================================================
 * Eventos MCU -> Qt
 * ====================================================================== */
constexpr uint8_t EVT_ALIVE           = 0xF0;
constexpr uint8_t EVT_NEW_BOX         = 0x70;  // [type, servo, timeHi, timeLo]
constexpr uint8_t EVT_IR_SENSOR       = 0x71;  // [sensorNum, state]
constexpr uint8_t EVT_SERVO_FIRED     = 0x72;  // [servoNum]
constexpr uint8_t EVT_DISTANCE        = 0x74;  // [distHi, distLo]
constexpr uint8_t EVT_CALIB_TIME      = 0x75;  // [sensorNum, timeHi, timeLo]

/* =========================================================================
 * Constantes
 * ====================================================================== */
constexpr int NUM_OUTPUTS    = 3;
constexpr int NUM_IR_SENSORS = 4;

constexpr int DEFAULT_TIME_SERVO0 = 1000;  // 2s en ticks de 2ms
constexpr int DEFAULT_TIME_SERVO1 = 2000;  // 4s
constexpr int DEFAULT_TIME_SERVO2 = 3000;  // 6s

#endif // PROTOCOL_H
