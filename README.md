# STM32 Pepper Dryer Controller

![C](https://img.shields.io/badge/Language-C-blue.svg)
![MCU](https://img.shields.io/badge/MCU-STM32F103C8T6-03234B.svg)
![IDE](https://img.shields.io/badge/IDE-STM32CubeIDE-03234B.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

STM32F103C8T6 보드를 기반으로 제작한 고추 건조기 제어 및 임베디드 타이밍 실험 프로젝트입니다.

초기 프로젝트는 DS18B20 온도 센서로 현재 온도를 측정하고, 목표 온도 기준으로 릴레이를 제어하여 히터의 ON/OFF를 결정하는 자동 건조기 제어 시스템으로 시작했습니다. 이후 cooperative_multitasking/ 디렉터리에서 DS18B20 온도 측정과 4-Digit FND 표시를 분리하는 구조를 실험했고, TIM3 timer interrupt 기반의 비동기 FND refresh 방식으로 기존 FND flickering 문제를 해결했습니다.

---

## Repository Layout

| Path | Description |
| :--- | :--- |
| Core/, Drivers/, hot_pepper_drier.ioc | 기존 고추 건조기 제어 프로젝트 (legacy_blocking_fnd) |
| cooperative_multitasking/ | DS18B20 + FND 비동기 refresh 구조를 검증한 신규 STM32CubeIDE 프로젝트 |
| docs/media/cooperative_multitasking_fnd_demo.mov | FND flickering 개선 후 동작 검증 영상 |

---

## Project Status

| Area | Status | Evidence |
| :--- | :--- | :--- |
| DS18B20 온도 센서 통합 | Completed | cooperative_multitasking/Core/Src/ds18b20.c, cooperative_multitasking/Core/Src/onewire.c |
| OneWire 기반 온도 측정 | Completed | Ds18b20_ManualConvert() |
| FND 온도 표시 | Completed | cooperative_multitasking/Core/Src/fnd_controller.c |
| FND flickering 개선 | Resolved | digit4_temper_scan(), HAL_TIM_PeriodElapsedCallback() |
| 50.0°C 기준 릴레이 제어 | Completed in original project | Core/Src/main.c, Core/Src/heaterController.c |
| Overshoot 완화 | Planned | Hysteresis 또는 PID 제어 검토 예정 |

---

## Demo

### Relay Switching Demo
소리를 통해 릴레이 동작 확인 가능

https://github.com/user-attachments/assets/3ec0a47a-daf2-4ed5-bfcd-c123cee26c76

### FND Flickering Fix Demo
The updated FND refresh behavior is recorded here:

https://github.com/user-attachments/assets/eac2c7af-5809-4692-aa8c-e0ef04f6da17

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
  - 예: 57.6°C

- **TIM3 인터럽트 기반 비동기 FND refresh**
  - 기존 blocking 표시 방식에서 발생하던 FND flickering 문제를 해결했습니다.
  - main loop는 온도 측정과 제어 판단을 담당하고, TIM3 interrupt는 FND 한 자리 refresh만 담당합니다.

---

## Hardware

| Module | Description |
| :--- | :--- |
| MCU | STM32F103C8T6, ARM Cortex-M3 |
| Temperature Sensor | DS18B20, OneWire Interface |
| Display | 4-Digit Multiplexed 7-Segment FND (다이나믹 구동 방식) |
| Shift Register | 74HC595D |
| Actuator | 1 Channel Relay Module |
| Load | Heater for pepper dryer prototype |

---

## Pin Map

### cooperative_multitasking/

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

## 소프트웨어 아키텍처 (Software Architecture)

### 메인 루프 (Main Loop)

메인 루프는 DS18B20 온도 변환을 수행하고 공유되는 디스플레이 값을 업데이트합니다.

    while (1)
    {
      if (Ds18b20_ManualConvert())
      {
        displayTempX10 = (int)(temperSensor->Temperature * 10);
      }
    }

displayTempX10은 온도를 10배 스케일링된 정수 형태로 저장합니다.

| 온도 (Temperature) | 저장된 값 (Stored Value) |
| :--- | :--- |
| 25.3°C | 253 |
| 50.0°C | 500 |
| 57.6°C | 576 |

이렇게 하는 이유는 인터럽트 외부에서 부동소수점(floating-point)연산을 행하기 위함입니다.

### TIM3 인터럽트 콜백 (TIM3 Interrupt Callback)

TIM3는 주기적인 인터럽트 소스로 구성됩니다. HAL 타이머 콜백은 매 인터럽트 발생 시 FND의 한 자릿수를 갱신(refresh)합니다.

    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
    {
      if (htim->Instance == TIM3)
      {
        digit4_temper_scan(displayTempX10);
      }
    }

HAL_TIM_PeriodElapsedCallback()은 사전 정의된 STM32 HAL 콜백 함수입니다. TIM3_IRQHandler()가 HAL_TIM_IRQHandler(&htim3)를 호출하면, HAL은 타이머 업데이트 이벤트를 확인하고 인터럽트 플래그를 지운 후 이 콜백을 호출합니다.

### 논블로킹 FND 스캔 (Non-Blocking FND Scan)

기존의 digit4_temper(temp, replay) 함수는 블로킹 루프 내에서 모든 자릿수를 반복적으로 갱신했습니다. 개선된 digit4_temper_scan() 함수는 호출될 때마다 단 한 자리만 갱신하고 즉시 반환(return)됩니다.

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

예를 들어 온도가 57.6°C라면 displayTempX10은 576이 됩니다.

| 인터럽트 횟수 (Interrupt Count) | 표시되는 자릿수 (Displayed Digit) |
| :--- | :--- |
| 1st | 6 |
| 2nd | 7. |
| 3rd | 5 |
| 4th | blank |
| 5th | 6 |

전체 4자리의 디스플레이는 사람의 눈에 안정적인 57.6으로 보일 만큼 충분히 빠른 속도로 갱신됩니다.

---

## 트러블슈팅 (Troubleshooting)

### FND 플리커링(깜빡임) 문제 - 해결 완료

#### 증상 (Symptom)

메인 루프에서 주기적으로 온도 센서를 읽어오고 액추에이터 제어를 수행할 때마다, 상시 켜져 있어야 할 FND 디스플레이가 일시적으로 아무것도 출력되지 않는 현상이 발생했습니다.

실제 레거시 버전(legacy_blocking_fnd)의 메인 루프 구조는 다음과 같았습니다:

    while (1)
    {
      // 1초 주기로 센서 변환 및 히터 제어 로직 실행
      if(HAL_GetTick() - last_tick > 1000)
      {
        Ds18b20_ManualConvert();           // OneWire 통신으로 인한 블로킹 발생
        current_temp = getCurrentTemper();
        last_tick = HAL_GetTick();

        if((int)(current_temp * 10) >= 500 && getHeaterState() == true)
        {
          heaterController(t_OFF);
        }
        if((int)(current_temp * 10) < 500 && getHeaterState() == false)
        {
          heaterController(t_ON);
        }
      }
      
      // 메인 루프에서 FND 전체 자릿수 반복 갱신 (Delay 20000)
      digit4_temper(current_temp*10, 20000, 0);
    }

digit4_temper() 함수는 잔상 효과를 위해 FND를 빠르게 반복해서 켜주어야 하지만, 매 1초마다 CPU가 if문 안으로 진입하여 타이밍이 민감한 Ds18b20_ManualConvert()를 수행하는 동안 호출이 중단되어 해당 현상이 발생했습니다.

#### 원인 분석 (Root Cause)

프로젝트에 사용된 디스플레이는 **멀티플렉스(Multiplexed) 또는 다이나믹 구동(Dynamic Drive) 방식의 4-Digit FND**입니다. 
이는 4개의 자릿수가 8개의 데이터 핀(a~g, dp)을 완전히 공유하고, CS(Chip Select / Common) 제어 핀을 통해 활성화할 특정 자릿수를 결정하는 하드웨어 구조를 갖습니다. 즉, 한 번에 오직 하나의 숫자만 물리적으로 켤 수 있으며, 4개의 숫자를 모두 보여주기 위해서는 각 자릿수를 아주 빠르게 번갈아 켜는 **시분할 다중화(Time-Division Multiplexing)**가 필수적입니다.

<img width="640" height="199" alt="스크린샷 2026-06-24 오후 7 21 23 중간" src="https://github.com/user-attachments/assets/1ace69e1-2f3f-430e-85dc-3bf47a4b369a" />

이전 아키텍처는 타이밍 요구사항이 전혀 다른 두 가지 태스크를 혼재하여 처리했습니다:

    온도 변환 및 제어 (블로킹이 수반되는 통신 태스크)
    FND 다이나믹 스캔 (지속적인 주기가 생명인 다중화 태스크)

DS18B20 변환 로직이 메인 루프를 점유할 때마다 필수적인 FND 스캔 주기가 끊어졌고, 이로 인해 사람의 눈에 띄는 플리커링이 발생한 것입니다.

#### 해결 방안 (Solution)

FND 갱신 태스크를 메인 루프에서 완전히 분리하여 TIM3 인터럽트 구동 기반의 스캔 루틴으로 이동시켰습니다.

    메인 루프 (main loop)
      - DS18B20 온도 변환
      - 히터 제어
      - displayTempX10 업데이트

    TIM3 인터럽트 (TIM3 interrupt)
      - FND 한 자릿수 갱신

이를 통해 속도가 느린 센서/제어 로직을 시간에 민감한 디스플레이 갱신 경로와 완벽히 격리했습니다.

#### 결과 (Result)

- FND 갱신이 더 이상 메인 루프의 실행 시간에 의존하지 않습니다.
- TIM3 인터럽트에 의해 하드웨어적으로 일정한 주기의 다이나믹 스캔이 지속됩니다.
- digit4_temper_scan()은 매 인터럽트마다 제한된 아주 적은 양의 연산만 수행합니다.
- 기존의 플리커링 이슈가 완벽히 해결되었습니다.

#### 시사점 (Engineering Takeaway)

이것은 단순한 디스플레이 버그가 아닌 하드웨어 구조(멀티플렉스 FND)의 제약 사항을 인지하지 못하고, 블로킹 제어 루프와 시간에 민감한 다중화 태스크를 혼합하여 발생한 '스케줄링(Scheduling)' 문제였습니다.

이 해결 과정은 다음의 역량들을 증명합니다:

- 하드웨어적 제약(핀 공유)에 대한 이해와 해결책 도출
- 타이밍 요구사항에 따른 태스크 분리
- 주기적 갱신을 위한 타이머 인터럽트 콜백 활용
- ISR(인터럽트 서비스 루틴) 작업을 짧고 결정론적(deterministic)으로 유지
- volatile 변수를 통한 메인 루프와 ISR 간의 안전한 데이터 공유

---

## 빌드 및 실행 방법 (Build & Flash)

1. 이 저장소를 로컬 PC로 클론합니다.

    git clone https://github.com/Ryu-Yoon-Min/stm32-project.git

2. STM32CubeIDE를 실행합니다.

3. 프로젝트를 Import 합니다.

    File > Open Projects from File System...

플리커링이 해결된 버전을 확인하려면 다음 경로를 선택하세요:

    stm32-project/cooperative_multitasking

4. 상단 메뉴의 망치 아이콘(Build)을 눌러 프로젝트를 컴파일합니다.

5. ST-Link 등 지원되는 디버거를 보드에 연결합니다.

6. 디버그(벌레 아이콘) 또는 실행(재생 아이콘) 버튼을 눌러 펌웨어를 플래싱하고 실행합니다.

---

## 연구 포트폴리오 노트 (Research-Oriented Notes)

이 프로젝트는 단순한 일회성 데모가 아닌, 대학원 진학을 위한 연구 포트폴리오 산출물로 구성되었습니다. 단순히 보드가 동작한다는 사실보다, 시스템 수준의 문제를 고립시키고 구조적으로 개선해 낸 과정이 핵심입니다.

FND 플리커링 이슈는 임베디드 시스템 디버깅의 구체적인 사례를 제공했습니다:

1. 불안정한 디스플레이 동작 현상 관찰
2. 멀티플렉스 하드웨어의 특성(핀 공유) 분석 및 한계점 파악
3. 메인 루프의 블로킹 갱신 구조로 원인 추적
4. 디스플레이 다중화를 주기적인 하드웨어 타이밍 태스크로 재설계
5. 갱신 로직을 TIM3 인터럽트로 분리 및 비동기화
6. 실제 하드웨어에서 안정화된 FND 동작 검증

이러한 워크플로우는 타이밍 분석, 하드웨어-소프트웨어 상호작용, 제어 루프 구조 설계, 그리고 제한된 자원 환경에서의 시스템 신뢰성 확보와 같은 **연구 중심의 임베디드 개발(Research-oriented embedded development)** 역량과 직결됩니다.

---

## 향후 개선 사항 (Future Works)

- [x] FND flickering 해결
  - 다이나믹 구동(멀티플렉스) 한계 극복
  - TIM3 timer interrupt 기반 비동기 refresh 구조 적용
  - digit4_temper_scan()으로 한 자리씩 비동기 표시

- [ ] 히터 overshoot 완화
  - hysteresis 제어 적용
  - 목표 온도 근처에서 선제적 heater OFF
  - PID 제어 검토
  - 온도 응답 곡선 기반 thermal model 분석
