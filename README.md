# RGB Guardian: LED Strip Pixel Shooter 🎮

An interactive, fast-paced LED strip game controlled by an STM32F031K6 microcontroller. The game features dynamic enemy generation, real-time collision physics, and integrated sound effects.

### 🛠️ Tech Stack & Hardware
* **Microcontroller:** STM32F031K6
* **Language:** C / C++
* **Hardware:** WS2812B Addressable LED Strip (57 LEDs), Push Buttons, Active Buzzer
* **Core Peripherals:** * **TIM1 & DMA:** For non-blocking, precise PWM signal generation to drive the LED strip.
* **GPIO:** For button debouncing and auditory feedback (buzzer).

### 🚀 Game Mechanics
* **Objective:** Shoot incoming enemy light beams by matching their specific colors (Red, Green, Blue) before they reach the base.
* **Dynamic Difficulty:** Enemy spawn rates and movement speeds increase progressively as the player's score goes up.
* **Collision System:** Real-time distance calculation between projectiles and enemies with immediate visual and auditory feedback upon impact. RGB-Guardian

### ⚙️ How to Build & Run
1. Clone this repository to your local machine.
2. Open **STM32CubeIDE** and import the project into your workspace.
3. Build the project using the internal GCC compiler.
4. Flash the compiled firmware (`.elf`/`.bin` file) to the STM32F031K6 using an **ST-LINK** debugger.

* ### 🤝 Contributors
* **[Muhammed Arda Korkut](https://github.com/mardakorkut)** - Co-Developer
* **[Muhammed Çayhan](https://github.com/Mcayhan)** - Co-Developer
