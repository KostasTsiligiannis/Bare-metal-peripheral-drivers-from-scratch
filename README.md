# Bare-metal-peripheral-drivers-from-scratch
Bare-metal peripheral drivers (GPIO, SPI, I2C, USART) written from scratch in C for STM32F303K8 (ARM Cortex-M4) using RM0316, validated via Logic Analyzer &amp; ESP32 for master-slave communication.
# Bare-Metal Embedded Peripheral Drivers & Protocol Stack (ARM Cortex-M4)

A production-oriented collection of **bare-metal device drivers** engineered from the ground up in Embedded C for the **STM32F303K8 (ARM Cortex-M4F)** microcontroller. All low-level abstractions, memory maps, clock configurations, and interrupt service routines are developed directly from the **STM32F303 Reference Manual (RM0316)** and **ARMv7E-M Architecture Reference Manual**, eliminating any dependency on vendor HAL or middleware libraries.

---

## Core Engineering Competencies

### 1. Low-Level MCU Architecture & Memory-Mapped I/O
* **Direct Hardware Control:** Interfacing directly with the silicon through memory-mapped structures and bit-level manipulation, eliminating opaque vendor abstractions in favor of predictable, cycle-accurate execution.
* **System Clock & Bus Management:** Understanding how clock signals are generated, scaled, and routed across internal buses (`AHB`/`APB`)—balancing peripheral performance against power consumption and verifying clock integrity at the pin level.
* **Hardware-Driven Concurrency (NVIC & EXTI):** Moving beyond sequential code by configuring hardware interrupt controllers, priority levels, and external pin triggers so the processor reacts deterministically to real-world events.
* **Deterministic Timekeeping:** Leveraging the core system timer (`SysTick`) to establish a reliable hardware timebase for timeouts, scheduling, and protocol synchronization.

### 2. Peripheral Driver Architecture & Design Patterns
* **Scalable Layered Design:** Structuring firmware into distinct abstraction layers—separating silicon-specific register maps from reusable driver interfaces and application logic—so drivers remain modular, readable, and maintainable.
* **Blocking vs. Non-Blocking Execution:** Designing dual-mode communication engines capable of both simple synchronous polling and asynchronous, interrupt-driven state machines that free up CPU cycles during data transfers.
* **Protocol Resilience & Fault Recovery:** Anticipating real-world electrical and timing anomalies (bus lockups, missed acknowledgments, data overruns) and building self-recovering state machines rather than fragile happy-path code.

---

## Implemented Peripheral Drivers

### General-Purpose I/O & External Interrupts (`GPIO` / `EXTI`)
* **Electrical & Pin Multiplexing:** Configuring physical pins for digital signaling, open-drain wired-AND buses, analog sensing, and internal peripheral routing while managing drive strength, slew rates, and pull-up/pull-down states.
* **Asynchronous Event Triggering:** Transforming external physical signals (switches, sensor data-ready lines) into immediate hardware interrupts with configurable edge sensitivity and priority arbitration.

### Serial Peripheral Interface (`SPI`)
* **Synchronous High-Speed Framing:** Architecting full-duplex, half-duplex, and simplex serial links by aligning clock polarity, phase, bit ordering, and frame sizes between Master and Slave devices.
* **Bus Arbitration & Slave Management:** Implementing both hardware-enforced and software-controlled chip-select schemes to manage bus ownership cleanly.
* **Command-Response & Non-Blocking Transfers:** Building multi-step sensor communication protocols (command framing, dummy-byte clock generation, and status handshakes) alongside interrupt-driven data reception.

### Inter-Integrated Circuit (`I2C` — STM32 v2 Architecture)
* **Timing Synthesis & Signal Integrity:** Translating protocol specifications (`100 kHz` Standard / `400 kHz` Fast Mode) into precise hardware timing parameters, noise filtering, and hold-time adjustments to prevent open-drain bus glitches.
* **Atomic Bus Transactions:** Executing multi-phase reads and writes using **Repeated Start (`Sr`)** conditions—allowing the Master to switch from writing a register address to reading data without surrendering bus control.
* **Clock Stretching & State-Machine Automation:** Handling slave-driven clock stretching, automated byte counting, and interrupt-driven Master/Slave state transitions under real-world bus error conditions.

### Universal Synchronous/Asynchronous Receiver-Transmitter (`USART` / `UART`)
* **Asynchronous Serial Framing:** Achieving reliable clockless communication through fractional baud-rate generation, configurable data widths, parity checking, and stop-bit framing.
* **Flow Control & Buffered I/O:** Preventing data loss during high-throughput bursts using hardware flow control (`RTS`/`CTS`) and interrupt-driven asynchronous reception.

---

## Hardware Validation & Debugging Methodology

Writing peripheral drivers without physical verification leaves subtle timing and electrical issues undetected. Throughout this repository, firmware development follows a disciplined **hardware-in-the-loop validation workflow** modeled after industrial bring-up practices:

* **Physical-Layer Protocol Verification:** Rather than relying solely on software logs, bus activity is captured directly on the physical lines using a **Logic Analyzer (PulseView / sigrok)**. Decoding waveforms at the signal level verifies exact frame alignment, clock polarity/phase synchronization, acknowledgment handshakes, and true `START` / `Repeated START` / `STOP` transitions.
* **Signal Integrity & Timing Margins:** Developing reliable low-level drivers requires understanding the electrical reality of shared buses—such as open-drain rise/fall times, pull-up loading, and bus handoff transitions. Analyzing waveforms exposes subtle timing violations and transition glitches early, allowing precise tuning of setup and hold margins.
* **Cross-Platform Interoperability Testing:** True protocol compliance is proven when two distinct architectures communicate seamlessly. Using an **ESP32** as an independent external slave node tests the STM32 drivers against real-world slave behavior—including clock stretching, asynchronous command-response latency, and zero-delay **Repeated Start** handoffs.
* **Register-Level On-Chip Debugging:** Leveraging **SWD (ST-LINK)** and live peripheral register inspection to trace state-machine transitions, verify interrupt flag clearing sequences, and diagnose bus fault conditions directly in hardware.

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
