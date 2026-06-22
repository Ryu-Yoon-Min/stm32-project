# STM32 Pepper Dryer Controller

![C](https://img.shields.io/badge/Language-C-blue.svg)
![MCU](https://img.shields.io/badge/MCU-STM32F103C8T6-03234B.svg)
![IDE](https://img.shields.io/badge/IDE-STM32CubeIDE-03234B.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

STM32F103C8T6 보드를 기반으로 제작한 고추 건조기 제어 및 임베디드 타이밍 실험 프로젝트입니다.

초기 프로젝트는 DS18B20 온도 센서로 현재 온도를 측정하고, 목표 온도 기준으로 릴레이를 제어하여 히터의 ON/OFF를 결정하는 자동 건조기 제어 시스템으로 시작했습니다. 이후 `cooperative_multitasking/` 디렉터리에서 DS18B20 온도 측정과 4-Digit FND 표시를 분리하는 구조를 실험했고, TIM3 timer interrupt 기반의 비동기 FND refresh 방식으로 기존 FND flickering 문제를 해결했습니다.

이 저장소는 단순 동작 구현보다 **문제 정의 -> 원인 분석 -> 구조 개선 -> 동작 검증**의 흐름을 보여주는 것을 목표로 합니다. 특히 대학원 연구실 지원 포트폴리오 관점에서, 센서 기반 embedded control system, timing-sensitive peripheral 제어, interrupt 기반 비동기 구조 설계 경험을 정리했습니다.

---

## Repository Layout

| Path | Description |
| :--- | :--- |
| `Core/`, `Drivers/`, `hot_pepper_drier.ioc` | 기존 고추 건조기 제어 프로젝트 |
| `cooperative_multitasking/` | DS18B20 + FND 비동기 refresh 구조를 검증한 신규 STM32CubeIDE 프로젝트 |
| `docs/media/cooperative_multitasking_fnd_demo.mov` | FND flickering 개선 후 동작 검증 영상 |

---

## Project Status

| Area | Status | Evidence |
| :--- | :--- | :--- |
| DS18B20 온도 센서 통합 | Completed | `cooperative_multitasking/Core/Src/ds18b20.c`, `cooperative_multitasking/Core/Src/onewire.c` |
| OneWire 기반 온도 측정 | Completed | `Ds18b20_ManualConvert()` |
| FND 온도 표시 | Completed | `cooperative_multitasking/Core/Src/fnd_controller.c` |
| FND flickering 개선 | Resolved | `digit4_temper_scan()`, `HAL_TIM_PeriodElapsedCallback()` |
| 50.0°C 기준 릴레이 제어 | Completed in original project | `Core/Src/main.c`, `Core/Src/heaterController.c` |
| Overshoot 완화 | Planned | Hysteresis 또는 PID 제어 검토 예정 |

---

## Demo

### Relay Switching Demo

https://github.com/user-attachments/assets/3ec0a47a-daf2-4ed5-bfcd-c123cee26c76

### FND Flickering Fix Demo

The updated FND refresh behavior is recorded here:

[cooperative_multitasking_fnd_demo.mov](docs/media/cooperative_multitasking_fnd_demo.mov)

---

## Key Features

- **자동 온도 조절 로직**
  - DS18B20으로 현재 온도를 측정합니다.
  - 목표 온도 50.0°C를 기준으로 릴레이를 제어합니다.

- **OneWire 프로토콜 기반 센서 통신**
  - DS18B20의 reset, presence pulse, read/write slot timing을 직접 다룹니다.
  - microsecond 단위 timing을 위해 TIM2 기반 delay를 사용합니다.

- **4-Digit FND 온도 표시**
  - 현재 온도를 소수점 첫째 자리까지 표시합니다.
  - 예: `57.6°C`

- **TIM3 인터럽트 기반 비동기 FND refresh**
  - 기존 blocking 표시 방식에서 발생하던 FND flickering 문제를 해결했습니다.
  - main loop는 온도 측정과 제어 판단을 담당하고, TIM3 interrupt는 FND 한 자리 refresh만 담당합니다.

---

## Hardware

| Module | Description |
| :--- | :--- |
| MCU | STM32F103C8T6, ARM Cortex-M3 |
| Temperature Sensor | DS18B20, OneWire Interface |
| Display | 4-Digit 7-Segment FND |
| Shift Register | 74HC595D |
| Actuator | 1 Channel Relay Module |
| Load | Heater for pepper dryer prototype |

---

## Pin Map

### `cooperative_multitasking/`

| Module | Function | STM32F103C8T6 Pin | Direction | Note |
| :--- | :--- | :--- | :--- | :--- |
| DS18B20 | DATA | PA3 | In/Out | OneWire data line |
| FND / 74HC595D | SCLK | PB13 | Output | Shift clock |
| FND / 74HC595D | RCLK | PB14 | Output | Latch clock |
| FND / 74HC595D | DIO | PB15 | Output | Serial data |

### Original Dryer Controller

| Module | Function | STM32F103C8T6 Pin | Direction | Note |
| :--- | :--- | :--- | :--- | :--- |
| Relay Module | IN | PB5 | Output | Heater ON/OFF control |

---

## Software Architecture

### Main Loop

The main loop performs DS18B20 temperature conversion and updates a shared display value.

```c
while (1)
{
  if (Ds18b20_ManualConvert())
  {
    displayTempX10 = (int)(temperSensor->Temperature * 10);
  }
}
```

`displayTempX10` stores temperature as an integer scaled by 10.

| Temperature | Stored Value |
| :--- | :--- |
| 25.3°C | 253 |
| 50.0°C | 500 |
| 57.6°C | 576 |

This keeps floating-point calculation out of the timer interrupt path.

### TIM3 Interrupt Callback

TIM3 is configured as a periodic interrupt source. The HAL timer callback refreshes one FND digit on each interrupt.

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
  {
    digit4_temper_scan(displayTempX10);
  }
}
```

`HAL_TIM_PeriodElapsedCallback()` is a predefined STM32 HAL callback. When `TIM3_IRQHandler()` calls `HAL_TIM_IRQHandler(&htim3)`, the HAL checks the timer update event, clears the interrupt flag, and calls this callback.

### Non-Blocking FND Scan

The previous function, `digit4_temper(temp, replay)`, refreshed all digits repeatedly inside a blocking loop. The improved function, `digit4_temper_scan()`, refreshes only one digit per call and returns immediately.

```c
void digit4_temper_scan(int n)
{
  static uint8_t pos = 0;

  uint8_t seg = 0xFF;
  uint8_t port = 1 << pos;

  int n1 = n % 10;
  int n2 = (n % 100) / 10;
  int n3 = (n % 1000) / 100;
  int n4 = (n % 10000) / 1000;

  switch (pos)
  {
    case 0:
      seg = _LED_0F[n1];
      break;

    case 1:
      seg = _LED_0F[n2] & 0x7F;
      break;

    case 2:
      if (n > 99)
      {
        seg = _LED_0F[n3];
      }
      break;

    case 3:
      if (n > 999)
      {
        seg = _LED_0F[n4];
      }
      break;
  }

  send_port(seg, port);

  pos++;
  if (pos >= 4)
  {
    pos = 0;
  }
}
```

For example, if the temperature is `57.6°C`, `displayTempX10` becomes `576`.

| Interrupt Count | Displayed Digit |
| :--- | :--- |
| 1st | `6` |
| 2nd | `7.` |
| 3rd | `5` |
| 4th | blank |
| 5th | `6` |

The full 4-digit display is refreshed fast enough that the human eye sees a stable `57.6`.

---

## Troubleshooting

### FND Flickering Trouble - Resolved

#### Symptom

The FND display flickered whenever the main loop performed temperature measurement or other blocking work.

The previous structure was:

```c
Ds18b20_ManualConvert();
temp = (int)(temperSensor->Temperature * 10);
digit4_temper(temp, 5000);
```

`digit4_temper()` was responsible for refreshing all FND digits repeatedly. Because it was called from the main loop, display refresh timing depended on how long the sensor/control logic took.

#### Root Cause

4-Digit FND displays use multiplexing. Each digit must be refreshed periodically and consistently.

The previous architecture mixed two tasks with different timing requirements:

```text
Temperature conversion and control
FND multiplex refresh
```

When DS18B20 conversion or other logic occupied the main loop, FND refresh timing became irregular, which caused flickering.

#### Solution

The FND refresh task was moved from the main loop to a TIM3 interrupt-driven scan routine.

```text
main loop
  - DS18B20 temperature conversion
  - heater control
  - displayTempX10 update

TIM3 interrupt
  - refresh one FND digit
```

This separates slow sensor/control logic from the time-sensitive display refresh path.

#### Result

- FND refresh no longer depends on the main loop execution time.
- The display is refreshed continuously by TIM3 interrupt.
- `digit4_temper_scan()` performs a small bounded amount of work per interrupt.
- The previous flickering issue was resolved.

#### Engineering Takeaway

This was not just a display bug. It was a scheduling problem caused by mixing a blocking control loop with a time-critical multiplexing task.

The fix demonstrates:

- separating tasks by timing requirements,
- using timer interrupt callbacks for periodic refresh,
- keeping ISR work short and deterministic,
- sharing data between main loop and ISR through a `volatile` variable,
- debugging embedded behavior from timing diagrams and observed symptoms.

---

## Build & Flash

1. Clone this repository.

```bash
git clone https://github.com/Ryu-Yoon-Min/stm32-project.git
```

2. Open STM32CubeIDE.

3. Import either project.

```text
File > Open Projects from File System...
```

For the flickering-fix version, select:

```text
stm32-project/cooperative_multitasking
```

4. Build the project with the hammer icon.

5. Connect ST-Link or another supported debugger.

6. Flash and run the firmware with Debug or Run.

---

## Research-Oriented Notes

This project is organized as a graduate-school portfolio artifact rather than a one-off demo. The important part is not only that the board works, but that the system-level problem was isolated and improved.

The FND flickering issue provided a concrete embedded-systems debugging case:

1. Observe unstable display behavior.
2. Trace the behavior to blocking main-loop refresh.
3. Identify display multiplexing as a periodic timing task.
4. Move refresh into TIM3 interrupt.
5. Keep sensor/control logic in the main loop.
6. Verify stable FND behavior on hardware.

This workflow is directly connected to research-oriented embedded development: timing analysis, hardware-software interaction, control-loop structure, and reliable system behavior under resource constraints.

---

## Future Works

- [x] FND flickering 해결
  - TIM3 timer interrupt 기반 refresh 구조 적용
  - `digit4_temper_scan()`으로 한 자리씩 비동기 표시

- [ ] 히터 overshoot 완화
  - hysteresis 제어 적용
  - 목표 온도 근처에서 선제적 heater OFF
  - PID 제어 검토
  - 온도 응답 곡선 기반 thermal model 분석

---

## License

This project is licensed under the MIT License.
