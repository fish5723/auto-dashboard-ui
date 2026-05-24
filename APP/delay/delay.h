//
// Created by A on 2026/5/4.
//

#ifndef LCD_EX_DELAY_H
#define LCD_EX_DELAY_H
#include "stm32f4xx_hal.h"

// 初始化延时函数（需在系统时钟配置后调用）
void delay_init(void);

// 微秒级延时（阻塞）
void delay_us(uint32_t nus);

// 毫秒级延时（阻塞，直接调用 HAL_Delay）
#define delay_ms(ms) HAL_Delay(ms)
#endif //LCD_EX_DELAY_H
