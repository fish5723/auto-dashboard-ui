//
// Created by A on 2026/5/9.
//
#include "can_receiver.h"
#include "can.h"  // CubeMX 生成的头文件

VehicleData_t g_vehicle_data = {0};

// CAN 过滤器配置 - 接收所有标准帧
void CAN_FilterConfig(void)
{
    CAN_FilterTypeDef can_filter = {0};

    can_filter.FilterBank = 0;
    can_filter.FilterMode = CAN_FILTERMODE_IDMASK;
    can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
    can_filter.FilterIdHigh = 0x0000;
    can_filter.FilterIdLow = 0x0000;
    can_filter.FilterMaskIdHigh = 0x0000;
    can_filter.FilterMaskIdLow = 0x0000;
    can_filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    can_filter.FilterActivation = ENABLE;

    HAL_CAN_ConfigFilter(&hcan1, &can_filter);
}

// 初始化 CAN 并启动
void CAN_Init(void)
{
    CAN_FilterConfig();
    HAL_CAN_Start(&hcan1);
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
}

// 发送 OBD-II 请求帧
void CAN_RequestPID(uint8_t pid)
{
    CAN_TxHeaderTypeDef tx_header = {0};
    uint8_t tx_data[8] = {0x02, 0x01, pid, 0x55, 0x55, 0x55, 0x55, 0x55};
    uint32_t tx_mailbox = 0;

    tx_header.StdId = 0x7DF;      // OBD-II 功能请求 ID
    tx_header.ExtId = 0;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = 8;

    HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, &tx_mailbox);

}

// 解析接收到的 CAN 消息
void CAN_ProcessMessage(CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data)
{
    // 检查是否是 OBD-II 响应 (ID 0x7E8-0x7EF)
    if(rx_header->StdId >= 0x7E8 && rx_header->StdId <= 0x7EF)
    {
        uint8_t pid = rx_data[2];  // PID 在第三个字节

        switch(pid)
        {
            case PID_ENGINE_RPM:
                // RPM = ((A * 256) + B) / 4
                g_vehicle_data.rpm = ((rx_data[3] * 256) + rx_data[4]) / 4;
                break;

            case PID_VEHICLE_SPEED:
                // Speed = A
                g_vehicle_data.speed = rx_data[3];
                break;

            case PID_COOLANT_TEMP:
                // Temp = A - 40
                g_vehicle_data.coolant_temp = rx_data[3] - 40;
                break;
        }

        g_vehicle_data.data_ready = 1;
    }
}

// CAN 接收中断回调
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];

    if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK)
    {
        CAN_ProcessMessage(&rx_header, rx_data);
    }
}