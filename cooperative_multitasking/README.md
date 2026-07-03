# Cooperative Multitasking STM32 Demo

이 STM32CubeIDE 프로젝트는 DS18B20 온도 측정 경로와 4-Digit FND 갱신 경로를 독립적으로 분리한 데모

## 목적 (Purpose)

기존의 FND 디스플레이 로직은 메인 루프에서 모든 자릿수를 갱신하는 블로킹(Blocking) 함수를 사용

이로 인해 DS18B20 온도 변환 로직이 실행될 때마다 FND 갱신 주기가 불규칙해져 디스플레이가 깜빡이는 플리커링(Flickering) 현상이 발생

이 프로젝트는 FND 갱신 작업을 TIM3 하드웨어 타이머 인터럽트로 이동시켜 해당 문제를 구조적으로 해결

## 핵심 아이디어 (Core Idea)

    메인 루프 (main loop)
      - DS18B20 온도 변환
      - displayTempX10 업데이트

    TIM3 인터럽트 (TIM3 interrupt)
      - digit4_temper_scan(displayTempX10) 호출
      - 오직 FND 한 자릿수만 비동기 갱신

## 주요 파일 구성 (Key Files)

| 파일 (File) | 역할 (Role) |
| :--- | :--- |
| `Core/Src/main.c` | TIM3 시작, 메인 루프 기반 온도 업데이트, HAL 타이머 콜백 처리 |
| `Core/Src/fnd_controller.c` | FND 세그먼트 출력 및 `digit4_temper_scan()` 함수 정의 |
| `Core/Src/ds18b20.c` | DS18B20 온도 변환 및 스크래치패드 데이터 읽기 |
| `Core/Src/onewire.c` | OneWire 통신 (Reset, Read/Write 슬롯, Search 로직) |
| `cooperative_multitasking.ioc` | STM32CubeMX 주변장치(Peripheral) 핀 및 클럭 설정 |

## 소프트웨어 아키텍처 (Software Architecture)

### 메인 루프 (Main Loop)

메인 루프는 DS18B20 온도 변환을 수행하고 공유되는 디스플레이 값을 업데이트

    while (1)
    {
      if (Ds18b20_ManualConvert())
      {
        displayTempX10 = (int)(temperSensor->Temperature * 10);
      }
    }

displayTempX10은 온도를 10배 스케일링된 정수 형태로 저장

| 온도 (Temperature) | 저장된 값 (Stored Value) |
| :--- | :--- |
| 25.3°C | 253 |
| 50.0°C | 500 |
| 57.6°C | 576 |

이렇게 하는 이유는 인터럽트 외부에서 부동소수점(floating-point) 연산을 수행하기 위함

### TIM3 인터럽트 콜백 (TIM3 Interrupt Callback)

TIM3는 주기적인 인터럽트 소스로 구성
HAL 타이머 콜백은 매 인터럽트 발생 시 FND의 한 자릿수를 갱신(refresh)

    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
    {
      if (htim->Instance == TIM3)
      {
        if (!isBusy())
        {
            digit4_temper_scan(displayTempX10);
        }
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

<img width="640" height="199" alt="스크린샷 2026-06-24 오후 7 21 23 중간" src="https://github.com/user-attachments/assets/1ace69e1-2f3f-430e-85dc-3bf47a4b369a" />

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

이를 통해 속도가 느린 센서/제어 로직을 시간에 민감한 디스플레이 갱신 경로와 격리

#### 결과 (Result)

- FND 갱신이 더 이상 메인 루프의 실행 시간에 의존하지 않음
- TIM3 인터럽트에 의해 하드웨어적으로 일정한 주기의 다이나믹 스캔이 지속
- digit4_temper_scan()은 매 인터럽트마다 제한된 아주 적은 양의 연산만 수행
- 기존의 플리커링 이슈가 해결

#### 시사점 (Engineering Takeaway)

이것은 단순한 디스플레이 버그가 아닌 하드웨어 구조(멀티플렉스 FND)의 제약 사항을 인지하지 못하고, 블로킹 제어 루프와 시간에 민감한 다중화 태스크를 혼합하여 발생한 '스케줄링(Scheduling)' 문제

필요 역량:

- 하드웨어적 제약(핀 공유)에 대한 이해와 해결책 도출
- 타이밍 요구사항에 따른 태스크 분리
- 주기적 갱신을 위한 타이머 인터럽트 콜백 활용
- main함수에서 float 연산을 수행, digit4_temper_scan()함수를 이용하여 ISR 작업을 짧고 결정론적(deterministic)으로 유지
- volatile 변수를 통한 메인 루프와 ISR 간의 안전한 데이터 공유

---

## Git 추적 제외 항목 (Excluded From Git)

다음의 로컬/생성 파일들은 의도적으로 Git 추적에서 제외 (`.gitignore` 적용):

- `Debug/`
- `.metadata/`
- `.settings/`
- `*.launch`

이 파일들은 빌드 산출물이거나 로컬 IDE/디버거 상태 파일이므로, 프로젝트를 이해하거나 다시 빌드하는 데 필요하지 않습니다.
