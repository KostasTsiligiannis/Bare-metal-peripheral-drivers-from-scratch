# Bare-metal-peripheral-drivers-from-scratch
Bare-metal peripheral drivers (GPIO, SPI, I2C, USART) written from scratch in C for STM32F303K8 (ARM Cortex-M4) using RM0316, validated via Logic Analyzer &amp; ESP32 for master-slave communication.
# Bare-Metal Embedded Peripheral Drivers & Protocol Stack (ARM Cortex-M4)

A production-oriented collection of **bare-metal device drivers** engineered from the ground up in Embedded C for the **STM32F303K8 (ARM Cortex-M4F)** microcontroller. All low-level abstractions, memory maps, clock configurations, and interrupt service routines are developed directly from the **STM32F303 Reference Manual (RM0316)** and **ARMv7E-M Architecture Reference Manual**, eliminating any dependency on vendor HAL or middleware libraries.

---

## Core Engineering Competencies

### 1. Low-Level MCU Architecture & Memory-Mapped I/O
* **Register-Level Abstraction:** Direct manipulation of peripheral control, status, and data registers using `volatile`-qualified C structures mapped to physical bus addresses (`AHB`, `APB1`, `APB2`).
* **Clock Tree & Power Management:** Configuration of internal/external oscillators (`HSI`, `HSE`), bus prescalers, peripheral clock gating (`RCC`), and hardware clock routing verification via Microcontroller Clock Output (`MCO`).
* **Interrupt Architecture (NVIC & EXTI):** Configuration of Nested Vectored Interrupt Controller (`NVIC`) priority grouping, IRQ enable/disable masking, and External Interrupt (`EXTI`) edge-detection lines for deterministic event-driven execution.
* **Hardware Timebase:** Bare-metal **Cortex-M4 SysTick** timer implementation providing deterministic millisecond resolution for protocol timeouts and inter-frame delays.

### 2. Peripheral Driver Architecture & Design Patterns
* **Layered Firmware Design:** Clean separation between hardware register definitions, reusable peripheral driver APIs (`Init`, `DeInit`, `SendData`, `ReceiveData`, `IRQHandling`), and application-layer state machines.
* **Blocking (Polling) & Non-Blocking (Interrupt-Driven) Modes:** Dual-mode driver implementations supporting both deterministic polling and asynchronous, interrupt-driven state machines to maximize CPU utilization.
* **Robust Error Handling & Bus Recovery:** Hardware fault detection and recovery mechanisms for framing errors, overrun (`OVR`), arbitration loss, `NACK` responses, and bus-busy lockups.

---

## Implemented Peripheral Drivers

### GPIO & External Interrupts (EXTI)
* **Capabilities:** Input, Output (Push-Pull / Open-Drain), Alternate Function multiplexing (`AF0`–`AF15`), Analog mode, internal Pull-Up/Pull-Down networks, and programmable output slew rates.
* **Event-Driven Control:** Asynchronous hardware button/signal triggers mapped to `EXTI` lines with `NVIC` interrupt prioritization and pending-bit clearing.

### SPI (Serial Peripheral Interface)
* **Bus Topologies:** Full-Duplex, Half-Duplex, and Simplex communication in both Master and Slave roles.
* **Protocol Configuration:** Programmable `CPOL`/`CPHA` clock modes, baud rate prescalers, 8-bit/16-bit data frames, MSB/LSB bit ordering, and Hardware (`NSS`) vs. Software (`SSM`/`SSI`) slave management.
* **Bidirectional Command-Response:** Multi-byte command framing, slave acknowledgment handshakes, and asynchronous interrupt-driven slave data retrieval.

### I2C (Inter-Integrated Circuit — STM32 I2C v2 Architecture)
* **Timing & Speed Modes:** Standard Mode (`100 kHz`) and Fast Mode (`400 kHz`) configured via `I2C_TIMINGR` prescalers, setup/hold times, and analog/digital noise filters.
* **Atomic Bus Transactions:** Hardware-managed byte counting (`NBYTES`), automatic stop generation (`AUTOEND`), and **Repeated Start (`Sr`)** sequences for multi-stage register/command reads without releasing bus ownership.
* **Dual-Role & Interrupt Support:** Master and Slave transmit/receive state machines with `NACK` detection, clock stretching compliance, and non-blocking interrupt-driven transfers.

### USART / UART (Universal Synchronous/Asynchronous Receiver-Transmitter)
* **Serial Framing:** Configurable baud rates (fractional baud rate generation), word lengths (8/9-bit), parity control (Even/Odd/None), and stop bits.
* **Flow Control & Modes:** Full-duplex asynchronous communication, hardware flow control (`RTS`/`CTS`), and interrupt-driven ring-buffer/state-machine reception.

---

## Hardware Validation, Signal Integrity & Debugging

Every driver is rigorously verified on physical hardware rather than simulators, reflecting real-world industrial firmware bring-up practices:

* **Protocol Decoding & Timing Analysis:** Multi-channel logic analyzer captures (**PulseView / sigrok**) used to inspect frame alignment, clock duty cycles, `ACK`/`NACK` bits, `START`/`Repeated START`/`STOP` conditions, and inter-byte timing.
* **Signal Integrity Engineering:** Diagnosed and eliminated open-drain SDA bus handoff glitches (~300 ns positive spikes prior to the 9th clock cycle) by analyzing rise/fall parasitics and tuning `I2C_TIMINGR.SDADEL` hold timing.
* **Cross-Platform Interoperability (Master-Slave Testbench):** Validated protocol compliance against an **ESP32** target acting as an external peripheral slave, including native **ESP-IDF** hardware FIFO manipulation to support zero-delay Repeated Start transactions.
* **On-Chip Debugging:** Live register inspection, peripheral state verification, and fault diagnostics via **SWD (ST-LINK)** and GDB.

---

## Target Hardware & Development Environment

| Category | Specification |
| :--- | :--- |
| **Microcontroller** | STMicroelectronics STM32F303K8T6 (ARM Cortex-M4F, 32-pin LQFP) |
| **Development Board** | NUCLEO-F303K8 |
| **Testbench Companion** | ESP32-WROOM-32 (Dual-Core Xtensa LX6 — SPI & I2C Slave Emulation) |
| **Test & Measurement** | 8-Channel USB Logic Analyzer (24 MHz), PulseView / sigrok Protocol Decoders |
| **Toolchain & Build** | `arm-none-eabi-gcc`, GNU Make, STM32CubeIDE, OpenOCD / ST-LINK GDB Server |

---

## References & Documentation
* **ST RM0316:** *STM32F303xB/C/D/E, STM32F303x6/8, STM32F328x8, STM32F358xC, STM32F398xE Advanced ARM-based 32-bit MCUs Reference Manual*
* **ST DS9866:** *STM32F303x6 / STM32F303x8 Datasheet*
* **ARM DUI 0553:** *Cortex-M4 Devices Generic User Guide*
