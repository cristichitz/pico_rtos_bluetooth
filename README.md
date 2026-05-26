# Secure BLE Peripheral (Raspberry Pi Pico 2 W)
 
A secure **Bluetooth Low Energy (BLE) server** implementation for the Raspberry Pi Pico 2 W. This project integrates device discovery, protected data access, and hardware-mapped application behavior by combining **GAP**, **GATT**, and **BLE Security Manager** components within a **FreeRTOS** environment.
 
---
 
## Table of Contents
 
- [Key Features](#key-features)
- [Architecture & Design](#architecture--design)
- [Technical Challenge: BTstack SDK Debugging](#technical-challenge-btstack-sdk-debugging)
- [Hardware Requirements](#hardware-requirements)
---
 
## Key Features
 
### Immediate Alert Service (IAS)
- Allows authenticated clients to write to protected characteristics (**UUID `2A06`**).
- Write operations map directly to local hardware, adjusting **PWM duty cycles** to control LED brightness based on alert levels:
  - `Off`
  - `Medium`
  - `High`
### Alert Notification Service (ANS)
- Maps local hardware events to remote BLE operations.
- Physical button presses generate **"Simple Alerts,"** updating unread counts.
- Triggers remote notifications if the client has enabled the **Client Characteristic Configuration Descriptor (CCCD)**.
### BLE Security Manager
- Enforces strict access control.
- Negotiates **authenticated pairing** with **MITM (Man-in-the-Middle) protection**.
- Requires an **encrypted connection** before granting read/write access to protected GATT characteristics.
### FreeRTOS Integration
- The application runs on a continuous `BleTask()` event loop.
- Manages:
  - Connection states
  - Button events (via FreeRTOS event groups)
  - Hardware I/O
---
 
## Architecture & Design
 
The codebase utilizes **C++ object-oriented principles** to separate the functional FreeRTOS mainline from the GATT service implementations.
 
Instead of cluttering the main task with callback logic, GATT objects are resolved and handed over to dedicated helper classes:
 
- `ImmediateAlertService`
- `AlertNotificationService`
These nested classes manage the callback functions for characteristic operations entirely outside the main functional loop. `BleTask()` is strictly responsible for updating connection states and hardware events, ensuring **clean state management and modularity**.
 
---
 
## Technical Challenge: BTstack SDK Debugging
 
During development, incoming **unauthorized write requests** caused the system to enter an **infinite loop**, silently dropping the application and connection without emitting standard output errors.
 
### Root Cause
 
Tracing the execution path through the `c7222` library and into the underlying Pico SDK (BTstack) revealed a flaw in the event routing:
 
> In `att_server.c`, unauthorized writes were flagged as `AUTHORIZATION_UNKNOWN` and routed directly to `sm_request_pairing` rather than emitting the defined `OnAuthorizationRequest` event. Because pairing was already established, this trapped the system in a loop.
 
### Resolution
 
1. **Diagnostic Patch:** A custom `sm_request_authorization` function was temporarily patched into the SDK to prove the event emission failure.
2. **Production Workaround:** The authorization state is handled explicitly within the `OnPairingComplete` handler, ensuring valid security states while bypassing the broken routing logic in the SDK.
---
 
## Hardware Requirements
 
| Component | Notes |
|-----------|-------|
| Raspberry Pi Pico 2 W | Main microcontroller |
| 1x LED | Connected to a PWM-capable pin |
| 1x Push Button | Connected to an interrupt-capable pin |
