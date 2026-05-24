//
// Created by A on 2026/5/9.
//
#ifndef __CAN_RECEIVER_H
#define __CAN_RECEIVER_H

#include "main.h"
#include "cmsis_os.h"

// 标准 OBD-II PID
#define PID_ENGINE_RPM      0x0C
#define PID_VEHICLE_SPEED   0x0D
#define PID_COOLANT_TEMP    0x05

// 车辆数据结构
typedef struct {
    volatile uint16_t rpm;           // 发动机转速 (0-16383.75 RPM)
    volatile uint8_t  speed;         // 车速 (0-255 km/h)
    volatile int8_t   coolant_temp;  // 冷却液温度 (-40-215 °C)
    volatile uint8_t  data_ready;    // 数据更新标志
} VehicleData_t;

extern VehicleData_t g_vehicle_data;

void CAN_Init(void);
void CAN_RequestPID(uint8_t pid);
void CAN_ProcessMessage(CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data);

#endif
