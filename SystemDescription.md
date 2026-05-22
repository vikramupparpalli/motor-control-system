# Sensorless FOC Motor Control System — RA6T1 Motor Control Evaluation Platform

**Document:** System Architecture & Implementation Reference
**MCU:** Renesas RA6T1 (Arm® Cortex®-M4, 120 MHz)
**Hardware:** Inverter Board (r12tu0072) + CPU Card (r12tu0091)
**Revision:** 1.0

---

## 1. System Overview

This document describes a **Sensorless Field-Oriented Control (FOC)** motor control system built on the Renesas RA6T1 Motor Control Evaluation System. The platform consists of a two-board stack — a 3-phase inverter power board and an RA6T1-based CPU card — capable of driving PMSM (Permanent Magnet Synchronous Motor) or BLDC motors without a physical shaft encoder.

The sensorless approach reconstructs rotor position and speed from measured phase voltages and currents using an Extended Kalman Filter (EKF) or Sliding Mode Observer (SMO), eliminating the encoder connector (CN7/CN5 on the CPU card) while retaining full closed-loop torque and speed control.

---

## 2. Hardware Architecture

### 2.1 System Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     AC/DC Power Supply                          │
│                       (12 V – 80 V DC)                          │
└──────────────────────────┬──────────────────────────────────────┘
                           │ VDC (TP3)
┌──────────────────────────▼──────────────────────────────────────┐
│                    INVERTER BOARD (r12tu0072)                    │
│                                                                 │
│  ┌──────────┐   ┌──────────────┐   ┌────────────────────────┐  │
│  │ Bulk Cap │   │ 3-Phase      │   │ Current Sensing        │  │
│  │ 2×680 µF │──▶│ MOSFET Bridge│──▶│ Shunt Resistors        │  │
│  │ 80 V     │   │ RJK1054D ×6  │   │ R1/R96/R110 10 mΩ     │  │
│  └──────────┘   └──────┬───────┘   └──────────┬─────────────┘  │
│                        │ U/V/W out             │ OC_Shunt_P/N   │
│  ┌─────────────────────▼───────────────────────▼─────────────┐  │
│  │               Gate Driver Layer                            │  │
│  │   HIP4086 (3-ph) — Bootstrap caps — Dead-time via RDEL    │  │
│  └──────────────────────────────────────────────────────────-─┘  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Aux Power: LM5017MR (12 V buck) → L78M05 (5 V LDO)     │   │
│  └──────────────────────────────────────────────────────────┘   │
└──────────────────────────┬──────────────────────────────────────┘
                           │ 50-pin connector (CNE/CNB)
┌──────────────────────────▼──────────────────────────────────────┐
│                     CPU CARD (r12tu0091)                         │
│                                                                 │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  RA6T1 MCU  (100-pin LQFP)                                 │  │
│  │  • Arm Cortex-M4 @ 120 MHz, FPU                            │  │
│  │  • 12-bit ADC (×2), GPT timers (×8), POEG                  │  │
│  │  • SCI UART (P205/P206), USB, CAN (P401/P402)              │  │
│  └────────────────────────────────────────────────────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────────────┐  │
│  │ Debug Header │  │ Encoder I/F  │  │ Signal Conditioning   │  │
│  │ JTAG/SWD     │  │ CN5/CN7 XH   │  │ SN74LVC07A ×7        │  │
│  │ (CN2 FTSH)   │  │ (not used in │  │ Open-drain buffers    │  │
│  └──────────────┘  │  sensorless) │  └───────────────────────┘  │
│                    └──────────────┘                              │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Inverter Board (r12tu0072)

#### Power Stage
| Component | Part | Specification |
|-----------|------|---------------|
| High-side MOSFETs (Q1, Q2, Q4) | RJK1054DPB | N-ch, 100 V, low Rds(on) |
| Low-side MOSFETs (Q3, Q5–Q8) | RJK1054D | N-ch, 100 V |
| Bulk capacitors (C3, C4) | Electrolytic | 680 µF / 80 V × 2 |
| DC-link capacitors | C20, C49 | 0.1 µF / 80 V |
| Phase shunt resistors | R1, R96, R110 | 10 mΩ / 1 W (3-shunt topology) |
| Bus shunt resistor | R61 | 10 mΩ / 1 W (optional 1-shunt) |
| Overcurrent threshold | R63, R65 | 3.3 kΩ / 2.7 kΩ divider |

#### Gate Driver
| Component | Part | Notes |
|-----------|------|-------|
| Primary gate driver | HIP4086 (U6) | 3-phase, bootstrap, UVLO |
| Secondary gate driver | HIP4082 (U4/U10, DNF) | Optional independent H-bridge |
| Gate resistors | R38, R95 | 4.7 Ω (turn-on damping) |
| Bootstrap diodes | D8, D10, D13–D15, D18, D20, D21 | RF05VAM1S Schottky |
| Bootstrap capacitors | C29, C32–C34, C38–C40 | 1 µF / 25 V |

#### Auxiliary Power
| Rail | Circuit | Voltage |
|------|---------|---------|
| 12 V | LM5017MR (U7) switching regulator | 12 V from VDC |
| 5 V | L78M05CDT (U8) linear regulator | 5 V from 12 V |
| 5 V (isolated) | Via CNE connector | From CPU card |

#### Sensing
- **DC-link voltage** — resistor divider (R53/R55 470 kΩ) → TP3 → ADC
- **Phase currents** — three 10 mΩ shunts (low-side, 3-shunt), amplified via MAX4080 (U1) current-sense amplifier, OC comparator TS332IQ (U2A/B/C)
- **Overcurrent protection** — hardware comparator trips HIP4086 DIS pin within <1 µs; output signal `HISIDE_OC` fed to MCU IRQ

### 2.3 CPU Card (r12tu0091)

#### MCU — Renesas RA6T1 (U1, 100-pin LQFP)

| Feature | Detail |
|---------|--------|
| Core | Arm Cortex-M4 with FPU/DSP |
| Max clock | 120 MHz |
| Flash / RAM | 512 KB / 128 KB (typical RA6T1) |
| ADC | 12-bit, two independent units (ADC0/ADC1), up to 24 channels |
| GPT timers | 8 channels with complementary PWM outputs, dead-time insertion |
| POEG | Port Output Enable for Gate (hardware fault shutdown) |
| UART | SCI4 on P205 (TXD) / P206 (RXD) — 50-pin CN to inverter board |
| CAN | CANFD on P401 (CTX0) / P402 (CRX0) |
| Debug | JTAG/SWD via CN2 (FTSH-110 header) |
| Crystal | 10 MHz (X1) → PLL → 120 MHz system clock |

#### PWM Output Mapping (to Inverter Board)

| MCU Pin | GPT Channel | Signal | Inverter Signal |
|---------|-------------|--------|-----------------|
| P415 | GTIOC0A | UH (UP) | PWM_UH/A+H |
| P414 | GTIOC0B | UL (UN) | PWM_UL/A+L |
| P113 | GTIOC2A | VP | PWM_VH/B+H |
| P114 | GTIOC2B | VN | PWM_VL/B+L |
| P111 | GTIOC3A | WP | PWM_WH/B− |
| P112 | GTIOC3B | WN | PWM_WL/B− |

All six PWM signals pass through SN74LVC07A open-drain buffers (3.3 V → 5 V level shift) before reaching the 50-pin connector to the inverter board.

#### ADC Input Mapping

| MCU Pin | ADC Channel | Signal | Source |
|---------|-------------|--------|--------|
| P004 | AN100 | IU (phase U current) | 10 mΩ shunt + MAX4080 |
| P005 | AN101 | IV (phase V current) | 10 mΩ shunt + MAX4080 |
| P006 | AN102 | IW (phase W current) | 10 mΩ shunt + MAX4080 |
| P008 | AN003 | VPN (DC-link voltage) | Resistor divider |
| P500 | AN016 | VU (phase U voltage) | Resistor divider |
| P501 | AN116 | VV (phase V voltage) | Resistor divider |
| P502 | AN017 | VW (phase W voltage) | Resistor divider |
| P007 | PGAVSS100 | PGAVSS reference | Analog ground |

#### Hall / Encoder Interface (CN5, CN7 — unused in sensorless mode)

Both XH-5 connectors (B5B-XH-A) provide VCC, GND, and three signal lines (ENCA/HU, ENCB/HV, ENCZ/HW). In sensorless mode these connectors are unpopulated; the Hall interrupt pins (P411/IRQ4, P410/IRQ5, P409/IRQ6) are reconfigured as GPIOs or left floating.

---

## 3. Sensorless FOC Algorithm

### 3.1 Control Architecture

```
Speed Reference (ω*)
        │
        ▼
┌───────────────┐     iq*    ┌───────────────┐
│  Speed PI     ├───────────▶│  Current PI   │  Vd*, Vq*
│  Controller   │            │  (d-q axes)   ├──────────┐
└───────┬───────┘     id*=0  └───────────────┘          │
        │                            ▲                   ▼
        │             i_d, i_q       │          ┌────────────────┐
        │        ┌────────────────   │          │  Inverse Park  │
        │        │  Park Transform   │          │  Transform     │
        │        │  (Clarke→Park)    │          └───────┬────────┘
        │        │     θ̂            │          Vα, Vβ  │
        │        ▼                   │                   ▼
        │   ┌──────────┐     iα, iβ  │          ┌────────────────┐
        │   │  Sensorless│           │          │  Space Vector  │
        │   │  Observer  │           │          │  Modulation    │
        │   │ (EKF/SMO) │           │          │  (SVPWM)       │
        │   └──────┬─────┘           │          └───────┬────────┘
        │   θ̂, ω̂  │                 │                   │ PWM
        └──────────┘                 │                   ▼
                           ┌─────────┴──────┐   ┌──────────────┐
                           │Clarke Transform│◀──│  3-Phase     │
                           │  (ia, ib, ic)  │   │  Inverter    │
                           └────────────────┘   │  (PMSM/BLDC) │
                                   ▲            └──────────────┘
                                   │ ADC samples
                           ┌───────┴────────┐
                           │ Current Sensing│
                           │ (3-shunt)      │
                           └────────────────┘
```

### 3.2 Coordinate Transforms

#### Clarke Transform (3-phase → αβ)
```
iα =  ia
iβ = (ia + 2·ib) / √3
```

#### Park Transform (αβ → dq, using estimated angle θ̂)
```
id =  iα·cos(θ̂) + iβ·sin(θ̂)
iq = −iα·sin(θ̂) + iβ·cos(θ̂)
```

#### Inverse Park Transform (dq → αβ)
```
Vα = Vd·cos(θ̂) − Vq·sin(θ̂)
Vβ = Vd·sin(θ̂) + Vq·cos(θ̂)
```

### 3.3 Sensorless Position Observer

Two observer options are supported:

#### Option A — Sliding Mode Observer (SMO)
A reduced-order observer reconstructing the back-EMF vector from measured currents and applied voltages:

```
dî_α/dt = (1/L)·(Vα − R·î_α − ê_α)
dî_β/dt = (1/L)·(Vβ − R·î_β − ê_β)

ê_α = k·sign(î_α − iα)
ê_β = k·sign(î_β − iβ)

θ̂ = atan2(−ê_α, ê_β)
ω̂ = dθ̂/dt  (low-pass filtered)
```

Advantages: low computational cost, suitable for high-speed operation. Requires a low-pass filter on the BEMF estimate to suppress chattering.

#### Option B — Extended Kalman Filter (EKF)
State vector: **x** = [iα, iβ, ω, θ, ψ]ᵀ (currents, speed, angle, flux)

```
State prediction:   x̂(k|k−1) = f(x̂(k−1), u(k−1))
Covariance update:  P(k|k−1) = F·P·Fᵀ + Q
Kalman gain:        K = P·Hᵀ·(H·P·Hᵀ + R)⁻¹
State correction:   x̂(k) = x̂(k|k−1) + K·(y(k) − H·x̂(k|k−1))
```

Advantages: optimal under Gaussian noise, smooth angle estimate at all speeds. Higher CPU load (~15% at 120 MHz).

### 3.4 Space Vector PWM (SVPWM)

The SVPWM modulator maps the reference voltage vector (Vα, Vβ) to duty cycles for the six GPT channels. The RA6T1 GPT hardware generates complementary PWM with programmable dead-time insertion, eliminating the need for software dead-time compensation.

**Key parameters:**
| Parameter | Typical Value |
|-----------|--------------|
| PWM carrier frequency | 10–20 kHz |
| Dead-time | 500 ns – 2 µs |
| ADC trigger | GPT underflow (centre-aligned, valley trigger) |
| Control loop period | 1 / f_PWM (synchronous to PWM) |

Overmodulation up to 115% of the linear range is supported for maximum DC-link utilization.

---

## 4. RA6T1 Peripheral Configuration

### 4.1 General Purpose Timer (GPT)

Three GPT units are used in complementary PWM mode (triangle-wave carrier):

| GPT Unit | Timer | UH/UL Pair | Dead-time Register |
|----------|-------|------------|-------------------|
| GPT0 | GTIOC0A/0B | P415 / P414 | GTDTCR |
| GPT2 | GTIOC2A/2B | P113 / P114 | GTDTCR |
| GPT3 | GTIOC3A/3B | P111 / P112 | GTDTCR |

The POEG (Port Output Enable for Gate) peripheral provides hardware-level PWM shutdown on the `FO#` fault signal from the inverter board (P503/GTETRGC), with latency < 1 clock cycle.

### 4.2 ADC Synchronisation

```
PWM carrier triangle
        ▲
       /│\
      / │ \        ← ADC trigger at valley (GTCNT = 0)
     /  │  \
────/───┼───\────
        │
        └──▶ ADC0 scan: IU, IV, IW (simultaneous, 3-shunt)
             ADC1 scan: VDC, VU, VV, VW
```

Both ADC units are triggered synchronously by GPT underflow to sample at the centre of the PWM period, minimising switching noise on the shunt voltages.

### 4.3 Interrupt and Task Structure

| Priority | Source | Handler | Period |
|----------|--------|---------|--------|
| Highest | ADC0 scan complete | FOC current loop | 1/f_PWM |
| High | POEG / OC fault | Emergency stop | Async |
| Medium | GPT overflow | Speed loop update | 1 ms |
| Low | SCI4 RX/TX | UART communication | Async |
| Lowest | Main loop | Parameter update, diagnostics | 10 ms |

### 4.4 UART Communication (SCI4)

The SCI4 peripheral (P205 TXD, P206 RXD) is routed through the 50-pin connector to the inverter board for host communication. The VRL signal (P201/MD) provides a motor running flag to the inverter board.

---

## 5. Software Module Structure

```
ra6t1_sensorless_foc/
├── src/
│   ├── main.c                    # System init, main loop
│   ├── foc/
│   │   ├── foc_control.c/h       # FOC top-level (Clarke, Park, SVPWM)
│   │   ├── pi_controller.c/h     # Speed & current PI regulators
│   │   ├── svpwm.c/h             # Space vector modulation
│   │   ├── observer_smo.c/h      # Sliding mode observer
│   │   └── observer_ekf.c/h      # Extended Kalman filter
│   ├── hal/
│   │   ├── hal_adc.c/h           # ADC0/ADC1 init & DMA transfer
│   │   ├── hal_gpt.c/h           # GPT complementary PWM + dead-time
│   │   ├── hal_poeg.c/h          # Hardware fault / gate shutdown
│   │   └── hal_uart.c/h          # SCI4 UART driver
│   └── app/
│       ├── motor_params.h         # Motor electrical constants (R, L, ψ)
│       ├── startup.c/h            # IF (initial frequency) ramp-up
│       └── diagnostics.c/h        # Fault logging, LED status
├── config/
│   ├── foc_config.h               # PWM freq, dead-time, PI gains
│   └── ra_cfg/                    # FSP (Flexible Software Package) config
└── e2studio/                      # Renesas e2 studio project files
```

### 5.1 Current Control Loop (ISR)

```c
/* Triggered by ADC0 scan-complete interrupt */
void adc0_callback(adc_callback_args_t *p_args) {
    /* 1. Read phase currents from ADC */
    float ia = ADC_TO_AMPS(R_ADC0->ADDR[AN100]);
    float ib = ADC_TO_AMPS(R_ADC0->ADDR[AN101]);

    /* 2. Clarke transform */
    float i_alpha = ia;
    float i_beta  = (ia + 2.0f * ib) * INV_SQRT3;

    /* 3. Sensorless angle/speed estimation */
    observer_update(&g_obs, i_alpha, i_beta, v_alpha_prev, v_beta_prev);
    float theta_hat = g_obs.theta;
    float omega_hat = g_obs.omega;

    /* 4. Park transform */
    float id = i_alpha * cosf(theta_hat) + i_beta * sinf(theta_hat);
    float iq = -i_alpha * sinf(theta_hat) + i_beta * cosf(theta_hat);

    /* 5. Current PI controllers */
    float vd = pi_update(&g_pi_d, g_id_ref - id);
    float vq = pi_update(&g_pi_q, g_iq_ref - iq);

    /* 6. Inverse Park + SVPWM */
    float v_alpha = vd * cosf(theta_hat) - vq * sinf(theta_hat);
    float v_beta  = vd * sinf(theta_hat) + vq * cosf(theta_hat);
    svpwm_set_voltage(v_alpha, v_beta, g_vdc);
}
```

### 5.2 Motor Start-up Sequence (IF Control)

Sensorless FOC cannot operate at zero speed because back-EMF is zero. A forced-commutation ramp brings the motor to a minimum observable speed:

```
State machine:
  IDLE → IF_RAMP → OBSERVER_SYNC → CLOSED_LOOP → RUN
         (open-loop forced current at linearly increasing frequency)
                   (observer converges, switch to estimated θ)
                                     (speed PI engages)
```

Typical minimum handoff speed: 5–10% of rated speed, depending on motor parameters and observer tuning.

---

## 6. Motor Parameters

The following motor parameters must be characterised and entered in `motor_params.h` for correct observer and PI tuning:

| Parameter | Symbol | Unit | Description |
|-----------|--------|------|-------------|
| Stator resistance | Rs | Ω | DC resistance per phase |
| d-axis inductance | Ld | H | Measured at rated current |
| q-axis inductance | Lq | H | Measured at rated current |
| Permanent magnet flux | ψ_f | Wb | Back-EMF constant / ω |
| Pole pairs | p | — | Electrical / mechanical ratio |
| Rated speed | ω_rated | rpm | Mechanical |
| Rated current | I_rated | A | Peak phase current |
| Rated bus voltage | V_DC | V | Nominal DC-link |

---

## 7. Protection and Fault Handling

| Fault | Detection | Response |
|-------|-----------|----------|
| Overcurrent (hardware) | MAX4080 + TS332 comparator → `HISIDE_OC` | HIP4086 DIS pin → all gates off (<1 µs) |
| Overcurrent (software) | ADC current > I_OC threshold | POEG shutdown + error flag |
| DC undervoltage | ADC VDC < V_UVLO threshold | Stop PWM, wait for recovery |
| DC overvoltage | ADC VDC > V_OV threshold | Stop PWM, fault latch |
| MOSFET UVLO | HIP4086 internal UVLO | Gate driver disables outputs |
| Observer divergence | |θ̂_error| > threshold | Fall back to IF, re-sync |
| Speed over-limit | ω̂ > ω_max | Torque limit, controlled decel |

The `FO#` (Fault Output) signal from the gate driver is connected to RA6T1 pin P503 (GTETRGC) and triggers the POEG peripheral for immediate hardware PWM shutdown, independent of software execution.

---

## 8. Connector Pinout Reference

### 8.1 50-pin Inverter–CPU Connector (CNE on Inverter / CNA on CPU Card)

| Pin | Signal | Direction | Description |
|-----|--------|-----------|-------------|
| 1–3 | LED1#, LED2#, LED3# | CPU→INV | Status LEDs |
| 4 | VRL | CPU→INV | Motor run flag |
| 5 | FO# | INV→CPU | Gate driver fault output |
| 7 | WN | CPU→INV | PWM WN (low-side W) |
| 8 | VN | CPU→INV | PWM VN (low-side V) |
| 9 | UN | CPU→INV | PWM UN (low-side U) |
| 10 | WP | CPU→INV | PWM WP (high-side W) |
| 11 | VP | CPU→INV | PWM VP (high-side V) |
| 12 | UP | CPU→INV | PWM UP (high-side U) |
| 13–14 | SW1#, SW2# | INV→CPU | User push-buttons |
| 15–16 | +5VA | PWR | 5 V analog supply |
| 17–18 | GND | PWR | Ground |
| 19–20 | VCC33_A | PWR | 3.3 V analog supply |

### 8.2 50-pin Analog Signal Connector (CNB on Inverter / CNB on CPU Card)

| Pin | Signal | Description |
|-----|--------|-------------|
| 5 | IU | Phase U current (amplified) |
| 6 | IV | Phase V current (amplified) |
| 7 | IW | Phase W current (amplified) |
| 8 | VPN | DC-link voltage (scaled) |
| 9 | TEMP (VOT) | Thermal sensor |
| 10 | VU | Phase U voltage |
| 11 | VV | Phase V voltage |
| 12 | VW | Phase W voltage |

---

## 9. Development Environment

| Tool | Version / Notes |
|------|----------------|
| IDE | Renesas e2 studio (Eclipse-based) |
| SDK | Renesas FSP (Flexible Software Package) v4.x |
| Compiler | GCC for Arm Embedded (Arm GNU Toolchain) |
| Debugger | J-Link or E2 Lite (via CN2 SWD) |
| Motor Control Workbench | Renesas Motor Workbench 3.x (for parameter tuning, live scope) |
| UART terminal | 115200 baud, 8N1 on SCI4 |

---

## 10. Key Design Considerations

1. **Dead-time compensation** — At low speeds and low modulation index, dead-time distortion is significant. Apply voltage correction based on current sign detection, or use the RA6T1 GPT's hardware dead-time with software compensation at the SVPWM stage.

2. **3-shunt current reconstruction** — All three phase currents are sampled simultaneously at the PWM valley. For duty cycles > (1 − 2·t_dead/T_PWM), one phase shunt may be masked by switching transients; implement minimum pulse-width clamping or two-shunt reconstruction as a fallback.

3. **Observer initialisation** — The SMO/EKF observer requires a pre-magnetisation step (d-axis current injection, iq = 0) to establish initial flux linkage before the IF ramp begins.

4. **Parameter sensitivity** — Stator resistance Rs varies with temperature (~0.4%/°C for copper). For high-accuracy operation, implement online Rs estimation or use a temperature sensor (NTC on inverter board TEMP pin, P409 on CPU card) to compensate.

5. **EMC** — Gate resistors (4.7 Ω) and bootstrap capacitors are sized for the evaluation board layout. Production designs should tune gate resistance to balance switching losses against EMI, and add common-mode chokes on the motor cable.

---
