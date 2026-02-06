# PWR/SWR Meter with PeP (Peak Envelope Power) - Features and Documentation

## Overview

This Arduino-based project implements a **PWR/SWR meter** that measures Forward (Direct) and Reflected RF voltages using ADC inputs, calculates SWR (Standing Wave Ratio), and tracks Peak Envelope Power (PeP) and Peak SWR. The measured values are displayed on an analog milliammeter via PWM output.

---

## Hardware Connections

| Pin | Function | Description |
|-----|----------|-------------|
| **A0** | `pin_dir` | ADC input for Direct (Forward) voltage from RF coupler |
| **A2** | `pin_rev` | ADC input for Reflected voltage from RF coupler |
| **D3** | `pinF1` | Function pin: **AVG PWR** (average power) |
| **D4** | `pinF2` | Function pin: **PeP PWR** (peak envelope power) |
| **D5** | `pinF3` | Function pin: **SWR** (current standing wave ratio) |
| **D6** | `pinF6` | Function pin: **PeP SWR** (peak SWR / SWR Max) |
| **D7** | `PeakResetPin` | Push button to reset PeP PWR and PeP SWR values |
| **D9** | `maPin` | PWM output to drive the analog milliammeter |
| **D11** | `backlightPin` | PWM output to drive the Backlight LEDs (Ver. 1.2) |

### Notes on Connections
- All function switch pins (D3, D4, D5, D6) and the PeakResetPin (D7) use **internal pull-up resistors** - active LOW logic
- The rotary/function switch grounds one pin at a time to select the display mode
- The milliammeter is driven by a PWM signal (0-255) on pin D9

---

## Compile-Time Configuration

### Debug Serial Output
The Serial debug output can be enabled or disabled at compile time using a preprocessor directive:

```cpp
#define DEBUG_SERIAL  // Comment this line to disable Serial debug output
```

When `DEBUG_SERIAL` is defined:
- Serial communication is initialized at 115200 baud
- Debug output is sent every 500ms (configurable via `printInterval`)
- The `serialDebug()` function is compiled and called in the main loop

When `DEBUG_SERIAL` is commented out:
- No Serial code is compiled (saves flash memory and RAM)
- No Serial output overhead during runtime

---

## Functions Description

### `readAndAverage()`
Reads the ADC values from both Direct and Reflected voltage pins, takes multiple samples (default: 3), and computes the averaged voltage values.
- Respects `adcSamplingInterval` - skips sampling if interval not reached
- Applies ADC correction factor (`cor_adc = 0.858`)
- Converts ADC readings to voltage (0-5V range)
- Updates global variables: `dir_v_averaged`, `rev_v_averaged`

### `calculatePWR()`
Calculates the average power and tracks peak power.
- Computes `PWR = pwr_factor × dir_v_averaged`
- If current PWR exceeds stored peak (`pPWR`), updates the peak value
- The `pwr_factor` (default: 8) is used for calibration

### `calculateSWR()`
Calculates the Standing Wave Ratio using the standard formula:
```
SWR = (1 + ρ) / (1 - ρ)
```
Where `ρ = rev_v_averaged / dir_v_averaged` (reflection coefficient)

**Edge cases handled:**
- If reflected voltage < 0.05V → SWR = 1 (noise threshold)
- If reflected ≥ direct → SWR = INFINITY (open/short circuit)

**Peak SWR tracking:**
- If current SWR > swrMax (and not INFINITY), updates `swrMax`

### `send_to_mA()`
Sends the appropriate value to the analog milliammeter based on the function switch position:
- **pinF1 LOW** → Display AVG PWR (average power)
- **pinF2 LOW** → Display PeP PWR (peak envelope power)
- **pinF3 LOW** → Display SWR (current)
- **pinF6 LOW** → Display PeP SWR (peak SWR / swrMax)

**PWM Scaling:**
- Power values: scaled to 130W full scale (`pwmValue = (value / 130.0) × 255`)
- SWR values: scaled from 1-10 range (`pwmValue = ((SWR - 1) / 9.0) × 255`)

### `handlePeakReset()`
Monitors the PeakResetPin button with debounce protection (200ms).
- When pressed (LOW), resets both `pPWR` to 0 and `swrMax` to 1.0
- Allows tracking of new peak values

### `serialDebug()` (conditional compilation)
Outputs debug information to Serial monitor at intervals defined by `printInterval`.
- Only compiled when `DEBUG_SERIAL` is defined
- Displays: DirV, RevV, SWR, swrMax, PWR, pPWR

### `setup()`
Initializes:
- Serial communication at 115200 baud (only if `DEBUG_SERIAL` defined)
- All input pins with internal pull-up resistors

### `loop()`
Main execution cycle (runs continuously):
1. Read and average ADC values (respects sampling interval)
2. Check for peak reset button press
3. Calculate power values
4. Calculate SWR and track peak SWR
5. Send selected value to milliammeter
6. Output Serial debug (if `DEBUG_SERIAL` defined)

---

## Operating Principle

### Signal Flow
1. **RF Coupler** → Provides DC voltages proportional to Forward and Reflected RF power
2. **ADC Sampling** → Arduino reads both voltages, averages multiple samples for noise reduction
3. **Calculations** → Power and SWR are computed from the voltage readings
4. **Peak Tracking** → Maximum power and SWR values are continuously tracked and stored
5. **Display Selection** → Rotary switch selects which value to display
6. **PWM Output** → Selected value is converted to PWM and drives the analog meter

### Timing
- ADC sampling interval: configurable via `adcSamplingInterval` (default: 5ms)
- Serial debug output interval: configurable via `printInterval` (default: 500ms)
- Button debounce: 200ms delay prevents multiple triggers

### Calibration Parameters
| Parameter | Default | Purpose |
|-----------|---------|---------|
| `cor_adc` | 0.858 | ADC reference voltage correction factor |
| `pwr_factor` | 8 | Power indicator calibration |
| `swr_factor` | 0 | SWR indicator calibration (not currently used) |
| `num_samples` | 3 | Number of ADC samples to average |
| `adcSamplingInterval` | 5 | Minimum interval between ADC readings (ms) |
| `printInterval` | 500 | Serial debug output interval (ms) |

#### ADC Reference Correction (`cor_adc`)
The `cor_adc` variable compensates for the difference between the theoretical ADC reference voltage (5V) and the actual voltage supplied by the Arduino. The ADC conversion formula is:

```
voltage = cor_adc × (adc_reading / num_samples) × (5 / 1023)
```

**Calibration procedure:**
1. Measure the actual voltage on the Arduino's 5V pin with a multimeter
2. Adjust `cor_adc` so that ADC readings match real measured values
3. Example: If the 5V pin actually measures 4.29V, the correction factor accounts for this discrepancy

### Full Scale Deflection (FSD)
- **Power meter**: 130W FSD
- **SWR meter**: 1:1 to 10:1 SWR range (1 = no deflection, 10 = full scale)

---

## Global Variables

| Variable | Type | Description |
|----------|------|-------------|
| `dir_v_averaged` | float | Averaged Direct (Forward) voltage |
| `rev_v_averaged` | float | Averaged Reflected voltage |
| `peak_v_averaged` | float | Peak voltage (declared but not actively used) |
| `swr_v` | float | Calculated SWR value (current) |
| `swrMax` | float | Peak SWR value (PeP SWR) |
| `PWR` | float | Current average power |
| `pPWR` | float | Peak power (PeP PWR) |

---

## Serial Monitor Output

When `DEBUG_SERIAL` is enabled, the system outputs debug information every 500ms:
```
DirV: x.xxx   RevV: x.xxx   SWR: x.xx   swrMax: x.xx   PWR: xxx.x   pPWR: xxx.x
```

This allows real-time monitoring and calibration verification.

