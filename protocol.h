/**
 * @file    protocol.h
 * @brief   Definiciones del protocolo de comunicacion UNER entre MCU y HMI Qt.
 *
 * @details Define los identificadores de comandos (Qt -> MCU) y eventos
 *          (MCU -> Qt) utilizados sobre el protocolo UNER, asi como las
 *          constantes de configuracion por defecto del sistema.
 *
 *          Comandos (Qt -> MCU):
 *          - CMD_ALIVE (0xF0):         heartbeat bidireccional.
 *          - CMD_START (0x50):         iniciar clasificacion y encender cinta.
 *          - CMD_STOP  (0x51):         detener clasificacion y apagar cinta.
 *          - CMD_RESET (0x53):         resetear sistema completo.
 *          - CMD_SET_CONFIG (0x60):    configurar tipos de salida [tipo0, tipo1, tipo2].
 *          - CMD_SET_TIMES  (0x61):    configurar tiempo de arribo [servo, timeHi, timeLo].
 *          - CMD_SET_THRESHOLD (0x62): configurar umbrales de distancia [grande, mediana, pequena].
 *          - CMD_TEST_SERVO (0x63):    test manual de un servo [servoNum].
 *          - CMD_SET_SERVO_ANGLE (0x64): ajustar pulso de servo [servoNum, pulseTicks].
 *          - CMD_CALIB_MODE (0x65):    entrar/salir del modo calibracion [1=entrar, 0=salir].
 *
 *          Eventos (MCU -> Qt):
 *          - EVT_ALIVE (0xF0):        respuesta heartbeat.
 *          - EVT_NEW_BOX (0x70):      nueva caja detectada [type, servo, timeHi, timeLo].
 *          - EVT_IR_SENSOR (0x71):    cambio de estado IR [sensorNum, state].
 *          - EVT_SERVO_FIRED (0x72):  servo disparado [servoNum].
 *          - EVT_DISTANCE (0x74):     distancia medida [distHi, distLo].
 *          - EVT_CALIB_TIME (0x75):   tiempo de calibracion [sensorNum, timeHi, timeLo].
 *
 * @author  Luciano
 * @date    Mayo 2026
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

/* =========================================================================
 * Comandos Qt -> MCU
 * ====================================================================== */

/** @brief Heartbeat bidireccional: Qt envia, MCU responde con EVT_ALIVE. */
constexpr uint8_t CMD_ALIVE           = 0xF0;

/** @brief Iniciar clasificacion: activa la cinta (PC0 = LOW) y habilita la logica. */
constexpr uint8_t CMD_START           = 0x50;

/** @brief Detener clasificacion: apaga la cinta (PC0 = HIGH). */
constexpr uint8_t CMD_STOP            = 0x51;

/** @brief Reset completo: detiene cinta, limpia cola de cajas, sale de calibracion. */
constexpr uint8_t CMD_RESET           = 0x53;

/** @brief Configurar tipos de caja por salida. Payload: [boxType0, boxType1, boxType2]. */
constexpr uint8_t CMD_SET_CONFIG      = 0x60;

/** @brief Configurar tiempo de arribo de un servo. Payload: [servo, timeHi, timeLo]. */
constexpr uint8_t CMD_SET_TIMES       = 0x61;

/** @brief Configurar umbrales de distancia. Payload: [grande, mediana, pequena]. */
constexpr uint8_t CMD_SET_THRESHOLD   = 0x62;

/** @brief Disparo manual de un servo para test. Payload: [servoNum]. */
constexpr uint8_t CMD_TEST_SERVO      = 0x63;

/** @brief Ajustar ancho de pulso de un servo. Payload: [servoNum, pulseTicks]. */
constexpr uint8_t CMD_SET_SERVO_ANGLE = 0x64;

/** @brief Entrar o salir del modo calibracion. Payload: [1=entrar, 0=salir]. */
constexpr uint8_t CMD_CALIB_MODE      = 0x65;

/* =========================================================================
 * Eventos MCU -> Qt
 * ====================================================================== */

/** @brief Respuesta de heartbeat: el MCU esta operativo. */
constexpr uint8_t EVT_ALIVE           = 0xF0;

/** @brief Nueva caja detectada y clasificada. Payload: [type, servo, timeHi, timeLo]. */
constexpr uint8_t EVT_NEW_BOX         = 0x70;

/** @brief Cambio de estado de un sensor IR. Payload: [sensorNum, state]. */
constexpr uint8_t EVT_IR_SENSOR       = 0x71;

/** @brief Servo disparado (caja clasificada). Payload: [servoNum]. */
constexpr uint8_t EVT_SERVO_FIRED     = 0x72;

/** @brief Distancia medida por HC-SR04. Payload: [distHi, distLo] en cm. */
constexpr uint8_t EVT_DISTANCE        = 0x74;

/** @brief Tiempo de calibracion medido. Payload: [sensorNum, timeHi, timeLo]. */
constexpr uint8_t EVT_CALIB_TIME      = 0x75;

/* =========================================================================
 * Constantes del sistema
 * ====================================================================== */

/** @brief Cantidad de salidas (servos) del clasificador. */
constexpr int NUM_OUTPUTS    = 3;

/** @brief Cantidad total de sensores IR (S0 + S1 + S2 + S3). */
constexpr int NUM_IR_SENSORS = 4;

/** @brief Tiempo de arribo por defecto del Servo 0: 2 s = 1000 ticks de 2 ms. */
constexpr int DEFAULT_TIME_SERVO0 = 1000;

/** @brief Tiempo de arribo por defecto del Servo 1: 4 s = 2000 ticks de 2 ms. */
constexpr int DEFAULT_TIME_SERVO1 = 2000;

/** @brief Tiempo de arribo por defecto del Servo 2: 6 s = 3000 ticks de 2 ms. */
constexpr int DEFAULT_TIME_SERVO2 = 3000;

#endif // PROTOCOL_H
