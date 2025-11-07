#include "teensy_hardware.h"
#include <usb_midi.h>

namespace gruvbok {

TeensyHardware::TeensyHardware()
    : led_state_(false)
    , led_brightness_(255)
    , start_time_ms_(0) {

    button_states_.fill(false);
    button_last_states_.fill(false);
    button_last_debounce_time_.fill(0);
    rotary_pot_values_.fill(0);
    slider_pot_values_.fill(0);
}

bool TeensyHardware::init() {
    // Initialize button pins (digital inputs with pullup)
    for (int i = 0; i < 16; i++) {
        pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    }

    // Initialize LED pin (digital output)
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Set ADC resolution
    analogReadResolution(ADC_RESOLUTION);

    // Initialize pot values by reading them once
    for (int i = 0; i < 4; i++) {
        rotary_pot_values_[i] = readPotRaw(ROTARY_POT_PINS[i]);
        slider_pot_values_[i] = readPotRaw(SLIDER_POT_PINS[i]);
    }

    // USB MIDI is automatically initialized by Teensy USB stack
    // Just record start time
    start_time_ms_ = millis();

    return true;
}

void TeensyHardware::shutdown() {
    // Turn off LED
    digitalWrite(LED_PIN, LOW);

    // Send MIDI All Notes Off on all channels
    for (int channel = 0; channel < 16; channel++) {
        usbMIDI.sendControlChange(123, 0, channel + 1);  // CC 123 = All Notes Off
    }
}

bool TeensyHardware::readButton(int button) {
    if (button < 0 || button >= 16) {
        return false;
    }
    return button_states_[button];
}

uint8_t TeensyHardware::readRotaryPot(int pot) {
    if (pot < 0 || pot >= 4) {
        return 0;
    }
    return mapAdcToMidi(rotary_pot_values_[pot]);
}

uint8_t TeensyHardware::readSliderPot(int pot) {
    if (pot < 0 || pot >= 4) {
        return 0;
    }
    return mapAdcToMidi(slider_pot_values_[pot]);
}

void TeensyHardware::sendMidiMessage(const MidiMessage& msg) {
    if (msg.data.empty()) {
        return;
    }

    // Parse MIDI message and send via USB MIDI
    uint8_t status = msg.data[0];
    uint8_t type = status & 0xF0;
    uint8_t channel = (status & 0x0F) + 1;  // Teensy uses 1-16, we use 0-15

    switch (type) {
        case 0x80:  // Note Off
            if (msg.data.size() >= 3) {
                usbMIDI.sendNoteOff(msg.data[1], msg.data[2], channel);
            }
            break;

        case 0x90:  // Note On
            if (msg.data.size() >= 3) {
                usbMIDI.sendNoteOn(msg.data[1], msg.data[2], channel);
            }
            break;

        case 0xB0:  // Control Change
            if (msg.data.size() >= 3) {
                usbMIDI.sendControlChange(msg.data[1], msg.data[2], channel);
            }
            break;

        case 0xF0:  // System messages (no channel)
            if (status == 0xF8) {
                // MIDI Clock
                usbMIDI.sendRealTime(usbMIDI.Clock);
            } else if (status == 0xFA) {
                // MIDI Start
                usbMIDI.sendRealTime(usbMIDI.Start);
            } else if (status == 0xFB) {
                // MIDI Continue
                usbMIDI.sendRealTime(usbMIDI.Continue);
            } else if (status == 0xFC) {
                // MIDI Stop
                usbMIDI.sendRealTime(usbMIDI.Stop);
            }
            break;

        default:
            // Unsupported message type
            break;
    }
}

void TeensyHardware::setLED(bool on) {
    led_state_ = on;
    if (on) {
        analogWrite(LED_PIN, led_brightness_);  // PWM with current brightness
    } else {
        analogWrite(LED_PIN, 0);  // Off
    }
}

void TeensyHardware::setLEDBrightness(uint8_t brightness) {
    led_brightness_ = brightness;
    // If LED is currently on, update brightness immediately
    if (led_state_) {
        analogWrite(LED_PIN, led_brightness_);
    }
}

uint32_t TeensyHardware::getMillis() {
    return millis() - start_time_ms_;
}

void TeensyHardware::update() {
    uint32_t current_time = millis();

    // Update button states with debouncing
    for (int i = 0; i < 16; i++) {
        bool reading = readButtonRaw(i);

        // If the button state changed, reset debounce timer
        if (reading != button_last_states_[i]) {
            button_last_debounce_time_[i] = current_time;
        }

        // If enough time has passed, accept the new state
        if ((current_time - button_last_debounce_time_[i]) > DEBOUNCE_DELAY_MS) {
            if (reading != button_states_[i]) {
                button_states_[i] = reading;
            }
        }

        button_last_states_[i] = reading;
    }

    // Update pot values (simple averaging for noise reduction)
    for (int i = 0; i < 4; i++) {
        uint16_t new_value = readPotRaw(ROTARY_POT_PINS[i]);
        rotary_pot_values_[i] = (rotary_pot_values_[i] * 3 + new_value) / 4;  // Simple IIR filter

        new_value = readPotRaw(SLIDER_POT_PINS[i]);
        slider_pot_values_[i] = (slider_pot_values_[i] * 3 + new_value) / 4;
    }

    // Read and discard any incoming USB MIDI messages (we don't handle MIDI input yet)
    while (usbMIDI.read()) {
        // Discard
    }
}

// Private helper functions

uint8_t TeensyHardware::mapAdcToMidi(uint16_t adc_value) {
    // Map 0-1023 (10-bit ADC) to 0-127 (7-bit MIDI)
    uint32_t midi_value = (adc_value * 127) / ADC_MAX;
    return static_cast<uint8_t>(midi_value);
}

bool TeensyHardware::readButtonRaw(int button) {
    if (button < 0 || button >= 16) {
        return false;
    }
    // Buttons are active-low (INPUT_PULLUP)
    return digitalRead(BUTTON_PINS[button]) == LOW;
}

uint16_t TeensyHardware::readPotRaw(int pin) {
    return analogRead(pin);
}

} // namespace gruvbok
