# STM32 Pepper Dryer Controller 🌶️

![C](https://img.shields.io/badge/Language-C-blue.svg)
![IDE](https://img.shields.io/badge/IDE-STM32CubeIDE-03234B.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

STM32F103C8T6 보드를 기반으로 제작된 자동 고추 건조기 제어 시스템입니다. 
지정된 온도(50.0°C)를 유지하기 위해 실시간으로 온도를 측정하고 릴레이를 통해 히터를 제어합니다.

---

## 1. 데모 및 동작 사진 (Demo / Screenshots)

*(여기에 동작 사진 및 영상이 추가될 예정입니다)*

---

## 2. 핵심 기능 (Key Features)

* **자동 온도 조절 로직**: 현재 온도를 측정하여 50.0°C를 기준으로 릴레이(히터) ON/OFF 자동 제어. *(참고: 현재 v1.0 소스코드 상에는 30.0°C로 하드코딩 되어 있으며, 추후 업데이트 예정)*
* **실시간 온도 모니터링**: 4-Digit FND(7-Segment)를 활용하여 현재 온도를 소수점 첫째 자리까지 실시간 표출.
* **OneWire 프로토콜 통신**: DS18B20 센서와 통신하여 노이즈에 강한 디지털 온도 데이터 수집.

---

## 3. 하드웨어 스펙 및 구성 (Hardware Prerequisites)

* **MCU**: STM32F103C8T6 (ARM Cortex-M3)
* **온도 센서**: DS18B20 (OneWire Interface)
* **디스플레이**: 4-Digit 7-Segment (FND) + 74HC595D (Shift Register)
* **액추에이터**: 1 Channel 5V/12V Relay Module (히터 전원 제어용)

---

## 4. 핀 맵 및 배선 (Pin Map / Wiring)

*(여기에 핀 맵 표가 추가될 예정입니다)*

---

## 5. 개발 환경 (Software & Tools)

* **IDE**: STM32CubeIDE (Version 1.19.0)
* **Firmware Package**: STM32Cube FW_F1 V1.8.x
* **Toolchain**: GNU ARM Embedded Toolchain
* **Libraries**: HAL Driver API 사용

---

## 6. 소프트웨어 아키텍처 및 동작 논리 (Software Flow)

시스템은 인터럽트 기반의 타이머와 메인 루프(Polling)를 결합하여 동작합니다.

1. **초기화**: 시스템 클럭, GPIO, TIM2 타이머, FND, DS18B20 센서를 초기화합니다.
2. **온도 갱신 (1초 주기)**: `HAL_GetTick()`을 사용하여 1초(1000ms)마다 DS18B20 센서에 온도 변환 명령(`Ds18b20_ManualConvert()`)을 내리고 값을 읽어옵니다.
3. **히터 제어**: 
   - 측정된 온도가 50.0°C 미만일 경우: 릴레이 모듈 제어 핀을 HIGH(또는 LOW)로 설정하여 히터 ON.
   - 측정된 온도가 50.0°C 이상일 경우: 릴레이 제어 핀을 반전시켜 히터 OFF.
4. **디스플레이 출력**: `digit4_temper()` 함수를 통해 메인 루프 내에서 FND 각 자리의 세그먼트를 순차적으로 점등하여 온도를 표시합니다.

---

## 7. 빌드 및 실행 방법 (How to Build & Flash)

1. 이 저장소를 로컬 PC로 클론합니다.
   ```bash
   git clone [https://github.com/Ryu-Yoon-Min/hot_pepper_drier.git](https://github.com/Ryu-Yoon-Min/hot_pepper_drier.git)
   ```
2. STM32CubeIDE를 실행하고, File > Open Projects from File System...을 통해 클론한 폴더를 Import 합니다.

3. 상단 메뉴의 망치 아이콘(Build)을 눌러 프로젝트를 컴파일합니다.

4. ST-Link 등 디버거를 PC와 타겟 보드에 연결합니다.

5. 상단 메뉴의 벌레 아이콘(Debug) 또는 재생 아이콘(Run)을 눌러 펌웨어를 보드에 다운로드하고 실행합니다.

## 8. 향후 개선 사항 (To-Do / Future Works)
[ ] FND 디스플레이 구조 개선 (Flickering 이슈 해결):
현재 FND의 4자리 숫자를 표시할 때 메인 루프에서 Chip-Select 방식으로 딜레이를 주며 동작하여, 온도 측정 로직이 실행되는 1초마다 FND가 깜빡거리는 현상이 있습니다. 이를 타이머 인터럽트(Timer Interrupt) 기반의 백그라운드 스레드 방식으로 변경하여 깜빡임 없이 상시 점등되도록 구조를 개선할 예정입니다.

[ ] 오버슈트(Overshoot) 방지 로직 도입:
현재는 50°C 도달 시 즉시 히터가 꺼지지만, 잔열로 인해 온도가 목표치를 한참 웃도는 현상이 발생합니다. 50°C 부근에서 전원을 미리 차단하거나 PID/Hysteresis 제어를 도입하여 온도를 더 정확하게 유지하도록 개선할 예정입니다

## 9. 작성자 및 라이선스 (Author & License)
Author: Ryu-Yoon-Min

License: 이 프로젝트는 MIT License를 따릅니다.
