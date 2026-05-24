//
// Created by A on 2026/5/4.
//
#include "delay.h"

static uint32_t fac_us = 0;  // 每个微秒所需的 DWT 计数周期数

/**
 * @brief 初始化 DWT 周期计数器，用于微秒延时
 * @note  必须在系统时钟初始化后调用，且 HAL 库已初始化
 */
void delay_init(void)
{
    // 使能 DWT 调试组件的访问
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    // 复位 DWT 计数寄存器
    DWT->CYCCNT = 0;
    // 使能 DWT 周期计数器
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // 计算每个微秒所需的时钟周期数（系统时钟频率，单位 Hz）
    fac_us = SystemCoreClock / 1000000;
}

/**
 * @brief 微秒级延时（阻塞）
 * @param nus 延时微秒数（请注意最大值受 DWT 计数器限制，32位无符号，对于168MHz约25秒）
 */
void delay_us(uint32_t nus)
{
    uint32_t ticks = nus * fac_us;               // 所需周期数
    uint32_t start = DWT->CYCCNT;                // 起始计数值
    // 等待计数器差值达到 ticks 或溢出（自动处理翻转）
    while ((DWT->CYCCNT - start) < ticks);
}