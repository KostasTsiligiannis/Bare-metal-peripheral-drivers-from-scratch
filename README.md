# Bare-Metal Peripheral Drivers-From-Scratch
Bare-metal peripheral drivers (GPIO, SPI, I2C, USART) written from scratch in C for STM32F303K8 (ARM Cortex-M4) using RM0316, validated via Logic Analyzer &amp; ESP32 for master-slave communication. All low-level abstractions, memory maps, clock configurations, and interrupt service routines are developed directly from the manuals referring the section **References & Documentation** on the bottom of this README file, eliminating any dependency on vendor HAL or middleware libraries.

---

## Repository Architecture & Directory Guide

The repository is organized to cleanly separate reusable hardware drivers, MCU startup files, application-level test cases, and external ESP32 slave testbench firmware:

```text
Bare-metal-peripheral-drivers-from-scratch/
├── Drivers/                           # Core bare-metal peripheral drivers (inc/ & src/ for RCC, GPIO, EXTI, SPI, I2C, USART)
├── Src/                               # Main STM32 application entry points & active driver tests
├── Startup/                           # ARM Cortex-M4 startup assembly code & vector table definitions
├── STM32F303K8TX_FLASH.ld             # Custom linker script defining Flash/SRAM memory regions & sections
├── ESP32_Slave_Testbench/             # Arduino/C++ sketches for the ESP32 acting as an external SPI/I2C/UART Slave
│
├── Testing_clock_configurations/      # SYSCLK, HSI/HSE/PLL tests & MCO pin clock verification
│
├── i2c_slave_rcv/                     # I2C Master/Slave blocking data reception
├── i2c_slave_rcv_it/                  # I2C non-blocking, interrupt-driven reception state machine
├── i2c_slave_rcv_send/                # Bidirectional I2C Master-Slave command & data exchange
├── i2c_slave_rcv_send_repeated_start/ # Atomic multi-phase transactions using Repeated Start (Sr)
├── i2c_slave_sending_string/          # Multi-byte string transmission & buffer management over I2C
│
├── spi_rcv_msg_it/                    # Interrupt-driven asynchronous SPI message reception
├── spi_sensor_read/                   # Command-response SPI register read emulation (dummy-byte clocking)
│
├── uart_case/                         # Interrupt-driven USART echo & character case-conversion application
└── uart_only_send/                    # Blocking USART transmission & fractional baud-rate validation
```
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

### Inter-Integrated Circuit (`I2C`)
* **Timing Synthesis & Signal Integrity:** Translating protocol specifications (`100 kHz` Standard / `400 kHz` Fast Mode) into precise hardware timing parameters, noise filtering, and hold-time adjustments to prevent open-drain bus glitches.
* **Atomic Bus Transactions:** Executing multi-phase reads and writes using **Repeated Start (`Sr`)** conditions—allowing the Master to switch from writing a register address to reading data without surrendering bus control.
* **Clock Stretching & State-Machine Automation:** Handling slave-driven clock stretching, automated byte counting, and interrupt-driven Master/Slave state transitions under real-world bus error conditions.

### Universal Synchronous/Asynchronous Receiver-Transmitter (`USART` / `UART`)
* **Asynchronous Serial Framing:** Achieving reliable clockless communication through fractional baud-rate generation, configurable data widths, parity checking, and stop-bit framing.
* **Flow Control & Buffered I/O:** Preventing data loss during high-throughput bursts using hardware flow control (`RTS`/`CTS`) and interrupt-driven asynchronous reception.

---

## Hardware Validation, Signal Integrity & Debugging Philosophy

Writing driver code is only half of embedded systems engineering; proving that it behaves reliably under real electrical and timing constraints is what makes firmware production-ready. Rather than relying on software simulators or intrusive `printf` statements, this project follows a disciplined **hardware bring-up and on-chip debugging methodology**:

* **Protocol Verification on the Wire:** Using a multi-channel logic analyzer (**PulseView / sigrok**) to inspect physical bus activity—verifying frame alignment, clock polarity/phase, handshake acknowledgments, and transaction boundaries (`START`, `Repeated START`, `STOP`) against protocol specifications.
* **Signal Integrity & Timing Awareness:** Treating digital buses as analog transmission lines—observing rise/fall times, pull-up behavior, and bus handoff transitions, and adjusting hardware timing parameters (such as data hold and setup times) to prevent glitches and race conditions.
* **Cross-Platform Interoperability Testing:** Validating driver compliance against an independent target architecture (**ESP32** acting as an external slave node) rather than testing in isolation, ensuring the protocol implementation handles real-world slave latency, clock stretching, and buffer constraints.
* **Silicon-Level On-Chip Debugging (SWD / GDB):** Leveraging **STM32CubeIDE's** low-level inspection views over **ST-LINK (SWD)** to diagnose firmware and hardware behavior in real time:
  * **Special Function Registers (`SFRs`):** Verifying bit-level configuration of peripheral registers (`RCC`, `GPIO`, `SPI`, `I2C`, `NVIC`) directly on the silicon after initialization and during state transitions.
  * **Live `Expressions` & `Memory Browser`:** Inspecting driver handle structures, ring buffers, and raw memory-mapped address spaces across breakpoints without altering execution timing.
  * **Core `Registers` & `Disassembly`:** Auditing ARM Cortex-M4 core registers (`SP`, `LR`, `PC`, `xPSR`) and generated assembly instructions to verify `volatile` memory accesses, interrupt stacking, and compiler optimization behavior.

---
## Target Hardware & Development Environment

| Category | Specification |
| :--- | :--- |
| **Microcontroller** | STMicroelectronics STM32F303K8T6 (ARM Cortex-M4F, 32-pin LQFP) |
| **Development Board** | NUCLEO-F303K8 |
| **Testbench Companion** | NodeMCU-32S (ESP32-WROOM-32, Dual-Core Xtensa LX6 — SPI, I2C & UART Slave Emulation) |
| **Test & Measurement** | 8-Channel USB Logic Analyzer (24 MHz), PulseView / sigrok Protocol Decoders |
| **Toolchain & Build** | `arm-none-eabi-gcc`, GNU Make, STM32CubeIDE, OpenOCD / ST-LINK GDB Server |

---

## References & Documentation
* **ST RM0316:** *STM32F303xB/C/D/E, STM32F303x6/8, STM32F328x8, STM32F358xC, STM32F398xE Advanced ARM-based 32-bit MCUs Reference Manual*
* **ST DS9866:** *STM32F303x6 / STM32F303x8 Datasheet*
* **ARM DUI 0553:** *Cortex-M4 Devices Generic User Guide*
* **Espressif ESP32-WROOM-32 Datasheet:** *ESP32-D0WDQ6 Dual-Core Wi-Fi & Bluetooth MCU Module Technical Specifications*
* **NodeMCU-32S Hardware Reference:** *Ai-Thinker NodeMCU-32S Development Board Pinout & Electrical Characteristics*

