# Cooperative Multitasking STM32 Demo

This STM32CubeIDE project isolates the DS18B20 temperature read path from the 4-Digit FND refresh path.

## Purpose

The original FND display logic used a blocking function that refreshed all digits from the main loop. When DS18B20 temperature conversion ran, the FND refresh interval became irregular and the display flickered.

This project fixes that by moving FND refresh to TIM3 interrupt.

## Core Idea

```text
main loop
  - read DS18B20 temperature
  - update displayTempX10

TIM3 interrupt
  - call digit4_temper_scan(displayTempX10)
  - refresh one FND digit only
```

## Key Files

| File | Role |
| :--- | :--- |
| `Core/Src/main.c` | TIM3 start, main loop temperature update, HAL timer callback |
| `Core/Src/fnd_controller.c` | FND segment output and `digit4_temper_scan()` |
| `Core/Src/ds18b20.c` | DS18B20 temperature conversion and scratchpad read |
| `Core/Src/onewire.c` | OneWire reset, read slot, write slot, search logic |
| `cooperative_multitasking.ioc` | STM32CubeMX peripheral configuration |

## FND Scan Callback

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
  {
    digit4_temper_scan(displayTempX10);
  }
}
```

## Excluded From Git

The following local/generated files are intentionally not tracked:

- `Debug/`
- `.metadata/`
- `.settings/`
- `*.launch`

These files are build artifacts or local IDE/debugger state and are not needed to understand or rebuild the project.
