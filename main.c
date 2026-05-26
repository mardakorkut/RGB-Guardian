/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : LED Strip Pixel Shooter - Optimized Physics & Sounds
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint8_t g;
    uint8_t r;
    uint8_t b;
} Color_t;

typedef enum {
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState_t;

typedef struct {
    float pos;
    Color_t color;
    bool active;
} GameObject_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUM_LEDS 57
#define MAX_OBJECTS 30

#define LOGIC_1 40
#define LOGIC_0 19
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;

/* USER CODE BEGIN PV */
const Color_t COLOR_RED   = {0, 80, 0};
const Color_t COLOR_GREEN = {80, 0, 0};
const Color_t COLOR_BLUE  = {0, 0, 80};
const Color_t COLOR_WHITE = {80, 80, 80};
const Color_t COLOR_OFF   = {0, 0, 0};
const Color_t COLOR_BASE  = {10, 10, 10};

Color_t gameColors[3];
GameObject_t enemies[MAX_OBJECTS];
GameObject_t shots[MAX_OBJECTS];

GameState_t currentState = STATE_PLAYING;

uint32_t score = 0;

// GAME OPTIMIZATION VALUES (Calculated for 57 LEDs)
float currentEnemySpeed = 0.15f;       // Initial speed
const float maxEnemySpeed = 0.30f;     // Maximum speed limit
uint32_t currentSpawnDelay = 1200;
uint32_t spawnBaseDelay = 800;         // Base enemy spawn interval
const float shotSpeed = 0.35f;         // Projectile speed

uint32_t redButtonHoldStartTime = 0;
bool isRedHolding = false;

uint32_t lastGameUpdateTime = 0;
uint32_t lastSpawnTime = 0;

uint32_t cooldownStartTime = 0;
bool isCooldownActive = false;

uint32_t lastGreenDebounce = 0;
uint32_t lastRedDebounce = 0;
uint32_t lastBlueDebounce = 0;

Color_t renderBuffer[NUM_LEDS];
uint8_t pwmData[(NUM_LEDS * 24) + 50] = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */
void ResetGame(void);
void HandleButtons(void);
void Shoot(Color_t color);
void SpawnEnemies(void);
void UpdateGamePhysics(float dt);
void CheckCollisions(void);
void RenderScene(void);
void Set_LED_Color(int led_idx, Color_t color);
void ClearStrip(void);
void FillStripSolid(Color_t color);
bool ColorCompare(Color_t c1, Color_t c2);

void Sound_Shoot(void);
void Sound_EmptyClick(void);
void Sound_GameOver(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();

  /* USER CODE BEGIN 2 */
  gameColors[0] = COLOR_RED;
  gameColors[1] = COLOR_GREEN;
  gameColors[2] = COLOR_BLUE;

  // Seed the random number generator using system tick
  srand(HAL_GetTick());

  ResetGame();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      HandleButtons();

      // 60 FPS Game Loop
      if (HAL_GetTick() - lastGameUpdateTime >= 16) {
          lastGameUpdateTime = HAL_GetTick();

          if (currentState == STATE_PLAYING) {
              SpawnEnemies();
              UpdateGamePhysics(1.0f);

              if (currentState == STATE_PLAYING) {
                  CheckCollisions();
                  RenderScene();
              }
          }
      }
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM1_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 59;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim1);
}

static void MX_DMA_Init(void)
{
  __HAL_RCC_DMA1_CLK_ENABLE();
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // PA3 Buzzer Configuration
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Button Configurations (Pull-Up)
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

void HandleButtons(void) {
    uint32_t currentTime = HAL_GetTick();

    if (currentState == STATE_GAME_OVER) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) {
            if (!isRedHolding) {
                isRedHolding = true;
                redButtonHoldStartTime = currentTime;
            } else if (currentTime - redButtonHoldStartTime >= 3000) {
                ResetGame();
            }
        } else {
            isRedHolding = false;
        }
        return;
    }

    // 1.5 Second Cooldown / Penalty Duration
    if (isCooldownActive) {
        if (currentTime - cooldownStartTime >= 1500) {
            isCooldownActive = false;
        }
    }

    static bool green_pressed = false;
    static bool red_pressed = false;
    static bool blue_pressed = false;

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {
        if (!green_pressed && (currentTime - lastGreenDebounce > 300)) {
            if(isCooldownActive) Sound_EmptyClick();
            else Shoot(COLOR_GREEN);
            green_pressed = true; lastGreenDebounce = currentTime;
        }
    } else { green_pressed = false; }

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) {
        if (!red_pressed && (currentTime - lastRedDebounce > 300)) {
            if(isCooldownActive) Sound_EmptyClick();
            else Shoot(COLOR_RED);
            red_pressed = true; lastRedDebounce = currentTime;
        }
    } else { red_pressed = false; }

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET) {
        if (!blue_pressed && (currentTime - lastBlueDebounce > 300)) {
            if(isCooldownActive) Sound_EmptyClick();
            else Shoot(COLOR_BLUE);
            blue_pressed = true; lastBlueDebounce = currentTime;
        }
    } else { blue_pressed = false; }
}

void Shoot(Color_t color) {
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!shots[i].active) {
            shots[i].pos = 1.0f;
            shots[i].color = color;
            shots[i].active = true;
            Sound_Shoot();
            break;
        }
    }
}

void SpawnEnemies(void) {
    if (HAL_GetTick() - lastSpawnTime >= currentSpawnDelay) {
        for (int i = 0; i < MAX_OBJECTS; i++) {
            if (!enemies[i].active) {
                enemies[i].pos = (float)(NUM_LEDS - 1);
                enemies[i].color = gameColors[rand() % 3];
                enemies[i].active = true;
                lastSpawnTime = HAL_GetTick();

                // Dynamic difficulty calculation
                currentSpawnDelay = (rand() % 400) + spawnBaseDelay;
                break;
            }
        }
    }
}

void UpdateGamePhysics(float dt) {
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (shots[i].active) {
            shots[i].pos += shotSpeed * dt;
            if (shots[i].pos >= NUM_LEDS) {
                shots[i].active = false;
            }
        }
    }

    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (enemies[i].active) {
            enemies[i].pos -= currentEnemySpeed * dt;

            // GAME OVER: Enemy reached the base
            if (enemies[i].pos <= 0.5f) {
                FillStripSolid(COLOR_RED);
                Sound_GameOver();
                HAL_Delay(3500); // Visual feedback delay
                ClearStrip();    // Turn off all LEDs
                currentState = STATE_GAME_OVER;
                return;
            }
        }
    }
}

void CheckCollisions(void) {
    for (int e = 0; e < MAX_OBJECTS; e++) {
        if (!enemies[e].active) continue;
        for (int s = 0; s < MAX_OBJECTS; s++) {
            if (!shots[s].active) continue;

            float distance = enemies[e].pos - shots[s].pos;
            if (distance < 0) distance = -distance;

            if (distance <= 1.2f) {
                if (ColorCompare(enemies[e].color, shots[s].color)) {
                    enemies[e].active = false;
                    shots[s].active = false;

                    score++;
                    if (score % 10 == 0) {
                        // 1. Increase enemy speed
                        currentEnemySpeed += 0.015f;
                        if(currentEnemySpeed > maxEnemySpeed) {
                            currentEnemySpeed = maxEnemySpeed;
                        }

                        // 2. Increase spawn frequency (decrease delay)
                        if (spawnBaseDelay > 400) {
                            spawnBaseDelay -= 30;
                        }
                    }
                } else {
                    shots[s].active = false;
                    isCooldownActive = true;
                    cooldownStartTime = HAL_GetTick();
                    Sound_EmptyClick(); // Penalty sound
                }
            }
        }
    }
}

void RenderScene(void) {
    for (int i = 0; i < NUM_LEDS; i++) {
        renderBuffer[i] = COLOR_OFF;
    }
    renderBuffer[0] = COLOR_BASE;

    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (enemies[i].active) {
            int exact_cell = (int)(enemies[i].pos + 0.5f);
            if (exact_cell >= 0 && exact_cell < NUM_LEDS) {
                renderBuffer[exact_cell].r = enemies[i].color.r;
                renderBuffer[exact_cell].g = enemies[i].color.g;
                renderBuffer[exact_cell].b = enemies[i].color.b;
            }
        }
    }

    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (shots[i].active) {
            int exact_cell = (int)(shots[i].pos + 0.5f);
            if (exact_cell >= 0 && exact_cell < NUM_LEDS) {
                renderBuffer[exact_cell].r = shots[i].color.r;
                renderBuffer[exact_cell].g = shots[i].color.g;
                renderBuffer[exact_cell].b = shots[i].color.b;
            }
        }
    }

    for (int i = 0; i < NUM_LEDS; i++) {
        Set_LED_Color(i, renderBuffer[i]);
    }

    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwmData, (NUM_LEDS * 24) + 50);
}

void ResetGame(void) {
    isCooldownActive = false;
    lastSpawnTime = HAL_GetTick();

    // Reset values to default
    currentSpawnDelay = 1200;
    spawnBaseDelay = 800;
    score = 0;
    currentEnemySpeed = 0.15f;

    currentState = STATE_PLAYING;
    isRedHolding = false;

    for (int i = 0; i < MAX_OBJECTS; i++) {
        enemies[i].active = false;
        shots[i].active = false;
    }

    FillStripSolid(COLOR_GREEN);
    HAL_Delay(300);
    ClearStrip();
    lastGameUpdateTime = HAL_GetTick();
}

void Set_LED_Color(int led_idx, Color_t color) {
    if (led_idx >= NUM_LEDS || led_idx < 0) return;
    uint32_t mem_offset = led_idx * 24;
    for(int i=7; i>=0; i--) pwmData[mem_offset++] = (color.g & (1 << i)) ? LOGIC_1 : LOGIC_0;
    for(int i=7; i>=0; i--) pwmData[mem_offset++] = (color.r & (1 << i)) ? LOGIC_1 : LOGIC_0;
    for(int i=7; i>=0; i--) pwmData[mem_offset++] = (color.b & (1 << i)) ? LOGIC_1 : LOGIC_0;
}

void ClearStrip(void) {
    for(int i = 0; i < NUM_LEDS; i++) Set_LED_Color(i, COLOR_OFF);
    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwmData, (NUM_LEDS * 24) + 50);
}

void FillStripSolid(Color_t color) {
    for(int i = 0; i < NUM_LEDS; i++) Set_LED_Color(i, color);
    HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwmData, (NUM_LEDS * 24) + 50);
}

bool ColorCompare(Color_t c1, Color_t c2) {
    return (c1.r == c2.r && c1.g == c2.g && c1.b == c2.b);
}

// ==========================================
// SOUND LIBRARY
// ==========================================

void Sound_Shoot(void) {
    for (int s = 0; s < 15; s++) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);
        HAL_Delay(1);
    }
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
}

void Sound_EmptyClick(void) {
    for (int j = 0; j < 2; j++) {
        for (int s = 0; s < 4; s++) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);
            HAL_Delay(2);
        }
        HAL_Delay(40);
    }
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
}

void Sound_GameOver(void) {
    for (int s = 0; s < 100; s++) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);
        HAL_Delay(1);
    }
    for (int s = 0; s < 80; s++) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);
        HAL_Delay(2);
    }
    for (int s = 0; s < 60; s++) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_3);
        HAL_Delay(3);
    }
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
}

/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_param(uint8_t* file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
