# STM32 Pepper Dryer Controller

![C](https://img.shields.io/badge/Language-C-blue.svg)
![MCU](https://img.shields.io/badge/MCU-STM32F103C8T6-03234B.svg)
![IDE](https://img.shields.io/badge/IDE-STM32CubeIDE-03234B.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

STM32F103C8T6 보드를 기반으로 제작한 고추 건조기 제어 및 임베디드 아키텍처 개선 프로젝트입니다.

이 저장소는 단순한 기능 구현을 넘어, 임베디드 시스템에서 흔히 발생하는 자원 제약 및 타이밍 충돌 문제를 해결하기 위해 **시스템 아키텍처를 동기식(Sync) 구조에서 비동기식(Async) 구조로 고도화한 트러블슈팅 과정**을 담고 있습니다.

---

## 📂 Repository Layout & Navigation

설계 패러다임의 변화에 따라 두 개의 독립된 프로젝트로 분리되어 관리됩니다. **상세한 코드와 트러블슈팅 과정은 각 폴더의 README를 클릭하여 확인해 주세요.**

| 디렉터리 (Path) | 설명 (Description) |
| :--- | :--- |
| **[📁 01_legacy_blocking_fnd](./01_legacy_blocking_fnd)** | **[Version 1]** 메인 루프 기반 동기식 제어 버전. DS18B20 온도 측정과 FND 표시가 메인 루프에서 혼재되어 **디스플레이 깜빡임(Flickering) 문제**가 존재하는 초기 릴레이 제어 모델. |
| **[📁 02_cooperative_multitasking](./02_cooperative_multitasking)** | **[Version 2]** 하드웨어 타이머(TIM3) 인터럽트를 도입하여 DS18B20 통신 태스크와 FND 스캔 태스크를 분리한 최적화 버전. **플리커링 문제를 완벽히 해결한 논블로킹(Non-blocking) 아키텍처의 핵심 상세 내용이 포함되어 있습니다.** |

---

## 🔄 Architecture Evolution (아키텍처 개선 요약)

시스템 성능과 디스플레이 안정성을 확보하기 위해 다음과 같이 구조를 개선했습니다. 상세 원인 분석 및 해결 코드는 `02_cooperative_multitasking` 디렉터리에서 확인하실 수 있습니다.

| 항목 (Metrics) | Version 1: Legacy Blocking | Version 2: Cooperative Multitasking |
| :--- | :--- | :--- |
| **제어 아키텍처** | 동기식 포그라운드 폴링 (Sync Polling) | 비동기식 백그라운드 인터럽트 구동 (Async) |
| **FND 다중화 방식**| 메인 루프 소모형 블로킹 지연 (Blocking) | 정적 상태 머신 기반 논블로킹 (Non-blocking) |
| **디스플레이 안정성**| 통신 지연 시 주기적 **Flickering 발생** | 연산량과 무관하게 **상시 점등 보장 (해결 완료)** |

---

## 🎥 Demo & Video

### [Version 1]. Relay Switching Demo (자동 온도 제어 시연)
목표 온도(50.0°C 초과 / 45°C 미만)를 기준으로 릴레이가 스위칭되며 히터(드라이기) 전원을 제어하는 동작을 소리로 확인할 수 있습니다.

https://github.com/user-attachments/assets/3ec0a47a-daf2-4ed5-bfcd-c123cee26c76

### [Version 2]. FND Flickering Fix Demo (플리커링 개선 결과)
메인 루프 블로킹 현상으로 인해 디스플레이가 불안정하던 V1과 달리, 인터럽트 기반 비동기 처리(V2)를 통해 센서 통신 중에도 화면 흔들림 없이 실시간 온도를 안정적으로 표출합니다.

https://github.com/user-attachments/assets/eac2c7af-5809-4692-aa8c-e0ef04f6da17


---

## 🛠 Hardware Specification

* **MCU**: STM32F103C8T6 (ARM Cortex-M3)
* **Temperature Sensor**: DS18B20 (OneWire Interface)
* **Display**: 4-Digit Multiplexed 7-Segment FND (다이나믹 구동 방식)
* **Shift Register**: 74HC595D
* **Actuator**: 1 Channel Relay Module (히터 전원 제어용)

---

## 🚀 향후 개선 사항 (Future Works)

- [x] **FND flickering 해결 완료**: 다이나믹 구동 한계 극복 및 TIM3 인터럽트 기반 비동기 스캔 적용
- [ ] **히터 Overshoot 완화 계획**: 
  - Hysteresis 제어 적용
  - 목표 온도 근처에서 선제적 Heater OFF 로직 구현
  - PID 제어 도입 및 온도 응답 곡선 기반 Thermal Model 분석 검토
