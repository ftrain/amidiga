#include "desktop_hardware.h"
#include "RtMidi.h"
#include <iostream>
#include <iomanip>

namespace gruvbok {

DesktopHardware::DesktopHardware()
    : midi_out_(nullptr)
    , led_state_(false)
    , midi_initialized_(false) {

    buttons_.fill(false);
    rotary_pots_.fill(64);  // Default to middle value
    slider_pots_.fill(64);
}

DesktopHardware::~DesktopHardware() {
    shutdown();
}

bool DesktopHardware::init() {
    start_time_ = std::chrono::steady_clock::now();

    // Initialize RtMidi
    try {
        midi_out_ = std::make_unique<RtMidiOut>();

        // List available MIDI ports
        unsigned int port_count = midi_out_->getPortCount();
        std::cout << "Available MIDI ports:" << std::endl;

        if (port_count == 0) {
            std::cout << "  No MIDI ports available. Creating virtual port." << std::endl;
            midi_out_->openVirtualPort("GRUVBOK Output");
        } else {
            for (unsigned int i = 0; i < port_count; i++) {
                std::cout << "  " << i << ": " << midi_out_->getPortName(i) << std::endl;
            }

            // Open first available port
            std::cout << "Opening MIDI port 0" << std::endl;
            midi_out_->openPort(0);
        }

        midi_initialized_ = true;
        std::cout << "Desktop hardware initialized" << std::endl;
        return true;

    } catch (RtMidiError& error) {
        std::cerr << "RtMidi error: " << error.getMessage() << std::endl;
        midi_initialized_ = false;
        return false;
    }
}

void DesktopHardware::shutdown() {
    if (midi_out_ && midi_initialized_) {
        midi_out_->closePort();
    }
}

bool DesktopHardware::readButton(int button) {
    if (button < 0 || button >= 16) {
        return false;
    }
    return buttons_[button];
}

uint8_t DesktopHardware::readRotaryPot(int pot) {
    if (pot < 0 || pot >= 4) {
        return 0;
    }
    return rotary_pots_[pot];
}

uint8_t DesktopHardware::readSliderPot(int pot) {
    if (pot < 0 || pot >= 4) {
        return 0;
    }
    return slider_pots_[pot];
}

void DesktopHardware::sendMidiMessage(const MidiMessage& msg) {
    if (!midi_initialized_ || !midi_out_) {
        return;
    }

    try {
        midi_out_->sendMessage(&msg.data);

        // Debug print MIDI messages
        std::cout << "[MIDI] ";
        for (uint8_t byte : msg.data) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
        }
        std::cout << std::dec << std::endl;

    } catch (RtMidiError& error) {
        std::cerr << "Error sending MIDI: " << error.getMessage() << std::endl;
    }
}

void DesktopHardware::setLED(bool on) {
    led_state_ = on;
    if (on) {
        std::cout << "[LED] ON" << std::endl;
    }
}

uint32_t DesktopHardware::getMillis() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    return static_cast<uint32_t>(duration.count());
}

void DesktopHardware::update() {
    // Nothing to update for desktop (keyboard input handled elsewhere)
}

void DesktopHardware::simulateButton(int button, bool pressed) {
    if (button >= 0 && button < 16) {
        buttons_[button] = pressed;
    }
}

void DesktopHardware::simulateRotaryPot(int pot, uint8_t value) {
    if (pot >= 0 && pot < 4) {
        rotary_pots_[pot] = std::min(value, static_cast<uint8_t>(127));
    }
}

void DesktopHardware::simulateSliderPot(int pot, uint8_t value) {
    if (pot >= 0 && pot < 4) {
        slider_pots_[pot] = std::min(value, static_cast<uint8_t>(127));
    }
}

} // namespace gruvbok
