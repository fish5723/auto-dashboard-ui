/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lvgl.h"
#include "lv_port_disp.h"
#include "tftlcd.h"
#include "can_receiver.h"
#include"ui_dashboard.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
void StartLVGLTask(void *argument);
void StartLEDTask(void *argument);
void StartCANTask(void *argument);

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
osThreadId_t lvglTaskHandle;
const osThreadAttr_t lvglTask_attributes = {
        .name = "lvglTask",
        .stack_size = 1024 * 4,  // 4KB 栈
        .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t ledTaskHandle;
const osThreadAttr_t ledTask_attributes = {
        .name = "ledTask",
        .stack_size = 256 * 4,
        .priority = (osPriority_t) osPriorityLow,
};
osThreadId_t canTaskHandle;
const osThreadAttr_t canTask_attributes = {
        .name = "canTask",
        .stack_size = 128 * 4,
        .priority = (osPriority_t) osPriorityBelowNormal,  // 优先级高于 LVGL
};
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* 创建任务 */
  lvglTaskHandle = osThreadNew(StartLVGLTask, NULL, &lvglTask_attributes);  // 新增
  ledTaskHandle = osThreadNew(StartLEDTask, NULL, &ledTask_attributes);      // 新增
  canTaskHandle = osThreadNew(StartCANTask, NULL, &canTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* LVGL 任务 */
void StartLVGLTask(void *argument)
{
  ui_landscape_init();  // 初始化横屏仪表盘

  for(;;)
  {
    if(g_vehicle_data.data_ready)
    {
      ui_set_zuansu(g_vehicle_data.rpm);
      ui_set_speed(g_vehicle_data.speed);
      ui_set_tmp(g_vehicle_data.coolant_temp);
      ui_set_elc(75);
      g_vehicle_data.data_ready = 0;
    }

    static uint32_t last_flash = 0;
    if(lv_tick_get() - last_flash > 500) {
      ui_anquandai();
      last_flash = lv_tick_get();
    }

    lv_timer_handler();
    osDelay(20);
  }
}

/* LED 任务 */
void StartLEDTask(void *argument)
{
  for(;;)
  {
    HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
    osDelay(500);
  }
}
void StartCANTask(void *argument)
{
  static uint16_t rpm = 0;

  for(;;)
  {
    rpm += 100;
    if(rpm > 8000) rpm = 0;

    g_vehicle_data.rpm = rpm;
    g_vehicle_data.speed = rpm / 33;
    if(g_vehicle_data.speed > 240) g_vehicle_data.speed = 240;
    g_vehicle_data.coolant_temp = 85;
    g_vehicle_data.data_ready = 1;

    osDelay(100);  // 100ms 更新一次
  }
}
/* USER CODE END Application */

