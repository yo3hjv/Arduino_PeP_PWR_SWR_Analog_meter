# PWR/SWR Meter with PeP (Peak Envelope Power) - Features and Documentation

## Overview

This Arduino-based project implements a **PWR/SWR meter** that measures Forward (Direct) and Reflected RF voltages using ADC inputs, calculates SWR (Standing Wave Ratio), and tracks Peak Envelope Power (PeP). The measured values are displayed on an analog milliammeter via PWM output.

---

## Hardware Connections

| Pin | Function | Description |
|-----|----------|-------------|
| **A0** | `pin_dir` | ADC input for Direct (Forward) voltage from RF coupler |
| **A2** | `pin_rev` | ADC input for Reflected voltage from RF coupler |
| **D3** | `pinF1` | Function switch position 1 - displays **PWR** (average power) |
| **D4** | `pinF2` | Function switch position 2 - displays **PeP** (peak power) |
| **D5** | `pinF3` | Function switch position 3 - displays **SWR** |
| **D7** | `PeakResetPin` | Push button to reset peak power value |
| **D9** | `maPin` | PWM output to drive the analog milliammeter |

### Notes on Connections
- All function switch pins (D3, D4, D5) and the PeakResetPin (D7) use **internal pull-up resistors** - active LOW logic
- The rotary/function switch grounds one pin at a time to select the display mode
- The milliammeter is driven by a PWM signal (0-255) on pin D9

---

## Functions Description

### `readAndAverage()`
Reads the ADC values from both Direct and Reflected voltage pins, takes multiple samples (default: 3), and computes the averaged voltage values.
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

### `send_to_mA()`
Sends the appropriate value to the analog milliammeter based on the function switch position:
- **pinF1 LOW** → Display average Power (PWR)
- **pinF2 LOW** → Display Peak Power (pPWR/PeP)
- **pinF3 LOW** → Display SWR

**PWM Scaling:**
- Power values: scaled to 130W full scale (`pwmValue = (value / 130.0) × 255`)
- SWR values: scaled from 1-10 range (`pwmValue = ((SWR - 1) / 9.0) × 255`)

### `handlePeakReset()`
Monitors the PeakResetPin button with debounce protection (200ms).
- When pressed (LOW), resets `pPWR` to 0
- Allows tracking of new peak power values

### `setup()`
Initializes:
- Serial communication at 115200 baud
- All input pins with internal pull-up resistors

### `loop()`
Main execution cycle (runs continuously):
1. Read and average ADC values
2. Check for peak reset button press
3. Calculate power values
4. Calculate SWR
5. Send selected value to milliammeter

---

## Operating Principle

### Signal Flow
1. **RF Coupler** → Provides DC voltages proportional to Forward and Reflected RF power
2. **ADC Sampling** → Arduino reads both voltages, averages multiple samples for noise reduction
3. **Calculations** → Power and SWR are computed from the voltage readings
4. **Peak Tracking** → Maximum power value is continuously tracked and stored
5. **Display Selection** → Rotary switch selects which value to display
6. **PWM Output** → Selected value is converted to PWM and drives the analog meter

### Timing
- ADC sampling and calculations occur every loop iteration (~4-6ms cycle time)
- Serial debug output provides: Direct voltage, Reflected voltage, Selected value, PWM output
- Button debounce: 200ms delay prevents multiple triggers

### Calibration Parameters
| Parameter | Default | Purpose |
|-----------|---------|---------|
| `cor_adc` | 0.858 | ADC reference voltage correction |
| `pwr_factor` | 8 | Power indicator calibration |
| `swr_factor` | 0 | SWR indicator calibration (not currently used) |
| `num_samples` | 3 | Number of ADC samples to average |

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
| `swr_v` | float | Calculated SWR value |
| `PWR` | float | Current average power |
| `pPWR` | float | Peak power (PeP) |

---

## Serial Monitor Output

The system outputs debug information at each loop iteration:
```
DirV: x.xxx   RevV: x.xxx   Selected Value: xxx   PWM Output: xxx
```

This allows real-time monitoring and calibration verification.
