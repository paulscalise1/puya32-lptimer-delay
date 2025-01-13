#include <stdio.h>
#include <py32f0xx.h>
#include <py32f030x8.h>
#include <py32f0xx_hal_gpio.h>
#include <py32f0xx_hal_gpio_ex.h>
#include <py32f0xx_hal.h>

LPTIM_HandleTypeDef LPTIMConf = {0};
volatile uint8_t lptim_ovf = 0;


// ************** Prototypes **************
void APP_ErrorHandler(void);
void SysTick_Handler(void);
void SystemClockConfig(void);
void LPTIMInit(void);
void HAL_LPTIM_AutoReloadMatchCallback(LPTIM_HandleTypeDef *hlptim);
void LPTIM1_IRQHandler(void);
void LPTIM_Delay(uint32_t millis);
// ************** End Prototypes **************

void APP_ErrorHandler(void)
{
  /* Infinite loop */
  while (1)
  {
  }
}

void SysTick_Handler()
{
    HAL_IncTick();
}

void SystemClockConfig(void) // Setup HSI as 4MHz
{
  	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  	RCC_PeriphCLKInitTypeDef LPTIM_RCC = {0};

  	/* Oscillator Configuration */
  	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE; /* Select oscillators HSE, HSI, LSI, LSE */
  	RCC_OscInitStruct.HSIState = RCC_HSI_ON;                          /* Enable HSI */
  	RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;                          /* HSI div by 1 */
  	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_8MHz;  /* Configure HSI clock as 8MHz */
  	RCC_OscInitStruct.HSEState = RCC_HSE_OFF;                         /* Disable HSE */
  	/*RCC_OscInitStruct.HSEFreq = RCC_HSE_16_32MHz;*/
  	RCC_OscInitStruct.LSIState = RCC_LSI_ON;                         // Enable LSI for low power timer
  	RCC_OscInitStruct.LSEState = RCC_LSE_OFF;                         /* Disable LSE */
  	/*RCC_OscInitStruct.LSEDriver = RCC_LSEDRIVE_MEDIUM;*/
  	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;                     /* Disable PLL */
  	/*RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;*/          /* Select HSI as PLL source */
  	/* Configure oscillators */
  	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK){
    	APP_ErrorHandler();
  	}

  	/* Clock source configuration */
  	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1; /* Select clock types HCLK, SYSCLK, PCLK1 */
  	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI; /* Select HSI as the system clock */
  	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;     /* AHB clock not divide */
  	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;      /* APB clock not divided */
  	/* Configure clock source */
  	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK){
    	APP_ErrorHandler();
  	}

  	/* LPTIM clock configuration */
  	LPTIM_RCC.PeriphClockSelection = RCC_PERIPHCLK_LPTIM;     /* Select peripheral clock: LPTIM */
  	LPTIM_RCC.LptimClockSelection = RCC_LPTIMCLKSOURCE_LSI;   /* Select LPTIM clock source: LSI */
  	/* Peripheral clock initialization */
  	if (HAL_RCCEx_PeriphCLKConfig(&LPTIM_RCC) != HAL_OK){
    	APP_ErrorHandler();
  	}
  
  	__HAL_RCC_LPTIM_CLK_ENABLE();	// Enable LPTIM clock
	__HAL_RCC_PWR_CLK_ENABLE();	// Enable the Power System Clock
}

void LPTIMInit(void)
{
    /* LPTIM configuration */
    LPTIMConf.Instance = LPTIM;                         /* LPTIM */
    LPTIMConf.Init.Prescaler = LPTIM_PRESCALER_DIV128;  /* Prescaler: 128 */
    LPTIMConf.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE; /* Immediate update mode */
    /* Initialize LPTIM */
    if (HAL_LPTIM_Init(&LPTIMConf) != HAL_OK){
        APP_ErrorHandler();
    }

    // Enable LPTIM1 interrupt
    HAL_NVIC_SetPriority(LPTIM1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(LPTIM1_IRQn);

    __HAL_LPTIM_DISABLE(&LPTIMConf);
    HAL_Delay(1);
}

void HAL_LPTIM_AutoReloadMatchCallback(LPTIM_HandleTypeDef *hlptim){
    lptim_ovf = 1;
}

 void LPTIM1_IRQHandler(void)
{   
    HAL_LPTIM_IRQHandler(&LPTIMConf);
}

void LPTIM_Delay(uint32_t millis){
    float counts = millis * 0.256f; // With the understanding the prescaler is 128
    HAL_LPTIM_SetOnce_Start_IT(&LPTIMConf, (uint32_t)(counts + 0.5f));
    HAL_SuspendTick();    
    // Enter STOP mode
    do{
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI); 
    } while (lptim_ovf == 0);
    // After waking up
    lptim_ovf = 0; // Reset global flag
    HAL_ResumeTick();
    APP_SystemClockConfig(); // Must reconfig the clock after waking up
    __HAL_LPTIM_DISABLE(&LPTIMConf); // This is necessary (resets it for the next time)
    HAL_Delay(1); // Shutdown time must be more than 120 microseconds
}


int main (void)
{
    HAL_Init();
    SystemClockConfig();
    LPTIMInit();
    
    while (1) {
        LPTIM_Delay(1000); // 1s blocking delay using the low power timer, and STOP mode during the wait
        // ... do stuff ...
    }
}
