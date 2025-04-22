# rbdimmerESP32
Universal AC Dimmer Library for ESP32 (Arduino and ESP-IDF)

## Library Overview

---

_The AC Dimmer Library is an efficient solution for controlling the brightness of alternating current (AC) devices using an ESP32 family microcontroller. The library leverages ESP32 hardware capabilities, such as GPIO interrupt processing and high-precision timers, to precisely control the TRIAC activation moment in each half-cycle of alternating current._

   
## 🔗 Fraimworks: Guides and Examples

---

_​_  
1. [📄 Arduino Giude & Examples](https://www.rbdimmer.com/knowledge/article/59) 
2. [📄 ESP-IDF Guide & Examples](/knowledge/article/61)
3. [📄 ESPHome Guide & Examples](/knowledge/article/62)

   
## ⭐ Features and Benefits

---

- Compatible with Arduino, ESP-IDF, and ESPHome frameworks  
    
- Compatible with multi-phase systems  
    
- Support for multiple independent dimming channels
- Minimal processor resource usage thanks to hardware interrupts and ESP timers
- High-precision brightness control for AC devices
- Various brightness regulation curves (linear, RMS, logarithmic)
- Automatic detection of grid frequency (50/60 Hz and others)
- Smooth transitions between brightness levels
- Support for callback functions for synchronization with other events

##   

##  ⚙ Requirements

---

- ESP32 family microcontroller  
    
- Compatible to Arduino ESP32 Core (version 2.0.0 or higher), ESP-IDF (5.0 or higher)
- - ESP32​
    - ESP32-C3
    - ESP32-C6
    - ESP32-H2
    - ESP32-P4
    - ESP32-S2
    - ESP32-S3
- Compatible to ESP-IDF (5.0 or higher)
- - ESP32
    - ESP32-S2
    - ESP32-C3
    - ESP32-S3
    - ESP32-C2
    - ESP32-C6
    - ESP32-H2
    - ESP32-P4
- Compatible to ESPHome
- - ESP32
    - ESP32-S2
    - ESP32-S3
    - ESP32-C3
    - ESP32-H2

##   



## 📝 Functional Specifications

---

### __**Brightness Curve Selection**__

 The library supports three types of curves for brightness adjustment:

1. **Linear (AC_DIMMER_CURVE_LINEAR)**
    
    - Uniform change in delay angle
    - Suitable for simple applications
    - Perceived brightness is not linear
2. **RMS (AC_DIMMER_CURVE_RMS)**
    
    - Compensates for the RMS characteristic of a sinusoidal signal
    - Provides linear power change
    - Ideal for incandescent lamps and resistive loads
3. **Logarithmic (AC_DIMMER_CURVE_LOGARITHMIC)**
    
    - Compensates for the logarithmic perception of brightness by the human eye
    - Provides visually linear brightness change
    - Recommended for LED lighting

###   

### **Smooth Transitions**

For creating smooth transitions between brightness levels, use the _ac_dimmer_set_brightness_transition()_ function:

The function creates a smooth transition by breaking it into multiple small steps. The function uses a FreeRTOS task; during the transition, the main code continues to execute.

  

### **Multi-Channel Systems**

The library supports multiple independent dimming channels. The number of channels is limited in the library settings in the ac_dimmer.h file. Each dimming channel must have a separate output pin.

###   

### **Using Interrupt Callback Functions**

Callback functions allow you to synchronize your code with zero-crossing events. This is useful for tasks requiring precise synchronization with the AC network.

_⚠️_

Use minimal code in the callback function. We recommend using FreeRTOS task calls.

  

## 🧠 Optimization and Debugging

---

### Optimal Resource Usage

1. **ESP-Timer Limitations**:
    
    - The library uses **_esp_timer_**, which allows creating multiple software timers
    - For each channel, 2 timers are used: one for delay and one for pulse width
2. **Interrupt Optimization**:
    
    - Interrupt handlers are kept as short as possible to reduce load
    - The **IRAM_ATTR** attribute is used to place the handler code in **IRAM**
    - Complex calculations are performed outside of interrupt handlers. Important: do not use heavy code in the zero-cross interrupt callback function. We recommend using FreeRTOS task calls.
3. **Performance**:
    
    - Pre-calculated tables for brightness curves reduce computation time
    - Parameter caching reduces repeated calculations
4. **Log:**
5. - Library have enhanced log system
    - For project debugging, enable logging of library function operations to the serial port

  

### ❓ Solving Common Problems

---

> #### The lamp flickers or brightness is unstable. This may occur at dimming levels 0~8.
> 
> **Causes and Solutions**:
> 
> - **Incorrect AC Neutral and Line connection**: Check the AC power source connection. Neutral to N, Line to AC-L IN
> - **Problems with the zero-crossing detector**: Check the signal waveform and trigger threshold.
> - **ESP32 microcontroller architecture and Arduino core**: suboptimal compatibility and priorities of the ISR module and timer module at the Arduino Core level.
> - **High CPU load**: Reduce the number of calculations in the main loop
> - **Interrupt conflicts**: Ensure that other libraries do not conflict with interrupts

> #### Incorrect Brightness for Certain Load Types
> 
> **Causes and Solutions**: At 50% brightness, the lamp appears too bright or too dim
> 
> - **Solution**: Change the brightness curve type for your load

> #### Zero-Crossing Detector Not Working
> 
> **The dimmer does not respond to adjustment**:
> 
> - Check the zero-cross pin connection.
> - Make sure the signal reaches the ESP32
> - Check the ac_dimmer_get_frequency() function - if it returns 0, the network frequency is not determined

##   

## 📝 Technical Information

---

### AC Dimmer Operating Principles

Power control in AC circuits is based on the principle of phase regulation using a TRIAC (thyristor). Article link

### ESP-Timer Features in ESP32

The library uses esp_timer for precise delay control:

1. **ESP-Timer Advantages**:
    
    - Microsecond resolution
    - Software implementation allows creating numerous timers
    - Low overhead
2. **ESP-Timer Architecture**:
    
    - One hardware timer for all software timers
    - Queue of timers, ordered by triggering time
    - Callback functions are called in the context of the timer task
3. **Limitations**:
    
    - Small jitter (±10-50 μs) under high system load
    - Callback function should not block execution for long

### Interrupt Handling

1. **Zero-Cross Interrupts**:
    
    - Generated at the moment of zero-crossing
    - GPIO_INTR_POSEDGE interrupt type is used (rising edge only)
    - The handler starts timers for all active channels of the given phase
2. **Timers**:
    
    - The first timer is started for the delay time and activates the output
    - The second timer is started to determine the pulse duration
3. **Stability Enhancement Methods**:
    
    - Interrupt handlers are kept as short as possible and placed in IRAM
    - Critical sections protect shared data
    - Minimal calculations in interrupt handlers
