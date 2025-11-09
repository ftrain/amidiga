#include "desktop_hardware.h"
#include "../hardware/hardware_utils.h"
#include "RtMidi.h"
#include <iostream>
#include <iomanip>

namespace gruvbok {

DesktopHardware::DesktopHardware()
    : midi_out_(nullptr)
    , midi_in_(nullptr)
    , led_state_(false)
    , midi_initialized_(false)
    , current_port_(-1)
    , current_input_port_(-1)
    , mirror_mode_enabled_(false) {

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

        unsigned int port_count = midi_out_->getPortCount();

        if (port_count == 0) {
            addLog("No MIDI ports available. Creating virtual port.");
            midi_out_->openVirtualPort("GRUVBOK Output");
            current_port_ = -1;  // Virtual port
        } else {
            addLog("Found " + std::to_string(port_count) + " MIDI port(s)");
            // Auto-open first port by default
            std::string port_name = midi_out_->getPortName(0);
            midi_out_->openPort(0);
            current_port_ = 0;
            addLog("Opened MIDI port 0: " + port_name);
        }

        midi_initialized_ = true;
        addLog("Hardware initialized successfully");
        return true;

    } catch (RtMidiError& error) {
        addLog("RtMidi error: " + error.getMessage());
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
    if (!HardwareUtils::isValidButton(button)) {
        return false;
    }
    return buttons_[button];
}

uint8_t DesktopHardware::readRotaryPot(int pot) {
    if (!HardwareUtils::isValidPot(pot)) {
        return 0;
    }
    return rotary_pots_[pot];
}

uint8_t DesktopHardware::readSliderPot(int pot) {
    if (!HardwareUtils::isValidPot(pot)) {
        return 0;
    }
    return slider_pots_[pot];
}

void DesktopHardware::sendMidiMessage(const MidiMessage& msg) {
    if (!midi_initialized_ || !midi_out_) {
        return;
    }

    try {
        // Use raw pointer overload to avoid uint8_t vs unsigned char type issues
        midi_out_->sendMessage(msg.data.data(), msg.data.size());
        // No console spam!

    } catch (RtMidiError& error) {
        addLog("Error sending MIDI: " + error.getMessage());
    }
}

void DesktopHardware::setLED(bool on) {
    led_state_ = on;
    // No console spam for LED!
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
    if (HardwareUtils::isValidPot(pot)) {
        rotary_pots_[pot] = HardwareUtils::clampToMidi(value);
    }
}

void DesktopHardware::simulateSliderPot(int pot, uint8_t value) {
    if (HardwareUtils::isValidPot(pot)) {
        slider_pots_[pot] = HardwareUtils::clampToMidi(value);
    }
}

// MIDI port management
int DesktopHardware::getMidiPortCount() {
    if (!midi_out_) return 0;
    return static_cast<int>(midi_out_->getPortCount());
}

std::string DesktopHardware::getMidiPortName(int port) {
    if (!midi_out_ || port < 0 || port >= getMidiPortCount()) {
        return "";
    }
    try {
        return midi_out_->getPortName(static_cast<unsigned int>(port));
    } catch (RtMidiError&) {
        return "";
    }
}

bool DesktopHardware::selectMidiPort(int port) {
    if (!midi_out_) return false;

    try {
        // Close existing port if open
        if (midi_initialized_ && current_port_ >= 0) {
            midi_out_->closePort();
        }

        if (port < 0) {
            // Virtual port
            midi_out_->openVirtualPort("GRUVBOK Output");
            current_port_ = -1;
            addLog("Opened virtual MIDI port");
        } else if (port < getMidiPortCount()) {
            midi_out_->openPort(static_cast<unsigned int>(port));
            current_port_ = port;
            addLog("Opened MIDI port: " + getMidiPortName(port));
        } else {
            return false;
        }

        midi_initialized_ = true;
        return true;

    } catch (RtMidiError& error) {
        addLog("Error selecting MIDI port: " + error.getMessage());
        return false;
    }
}

// Logging
void DesktopHardware::addLog(const std::string& message) {
    log_messages_.push_back(message);
    if (log_messages_.size() > MAX_LOG_MESSAGES) {
        log_messages_.pop_front();
    }
    // GUI-only logging - no console spam!
}

void DesktopHardware::clearLog() {
    log_messages_.clear();
}

// MIDI input (mirror mode)
int DesktopHardware::getMidiInputPortCount() {
    if (!midi_in_) {
        try {
            midi_in_ = std::make_unique<RtMidiIn>();
        } catch (RtMidiError&) {
            return 0;
        }
    }
    return static_cast<int>(midi_in_->getPortCount());
}

std::string DesktopHardware::getMidiInputPortName(int port) {
    if (!midi_in_ || port < 0 || port >= getMidiInputPortCount()) {
        return "";
    }
    try {
        return midi_in_->getPortName(static_cast<unsigned int>(port));
    } catch (RtMidiError&) {
        return "";
    }
}

bool DesktopHardware::selectMidiInputPort(int port) {
    try {
        if (!midi_in_) {
            midi_in_ = std::make_unique<RtMidiIn>();
        }

        // Close existing port if open
        if (midi_in_->isPortOpen()) {
            midi_in_->closePort();
        }

        if (port >= 0 && port < getMidiInputPortCount()) {
            midi_in_->openPort(static_cast<unsigned int>(port));
            midi_in_->setCallback(&DesktopHardware::midiInputCallback, this);
            midi_in_->ignoreTypes(false, false, false);  // Don't ignore any messages
            current_input_port_ = port;
            addLog("Opened MIDI input port: " + getMidiInputPortName(port));
            return true;
        }

        return false;

    } catch (RtMidiError& error) {
        addLog("Error selecting MIDI input port: " + error.getMessage());
        return false;
    }
}

void DesktopHardware::setMirrorMode(bool enabled) {
    mirror_mode_enabled_ = enabled;
    if (enabled) {
        addLog("Mirror mode ENABLED - listening for MIDI input");
    } else {
        addLog("Mirror mode DISABLED");
    }
}

void DesktopHardware::midiInputCallback(double deltatime, std::vector<unsigned char>* message, void* userData) {
    (void)deltatime;  // Unused

    DesktopHardware* hardware = static_cast<DesktopHardware*>(userData);
    if (!hardware || !message || message->empty()) {
        return;
    }

    // Simple MIDI message logging
    std::string log = "MIDI IN: ";
    for (unsigned char byte : *message) {
        char hex[8];
        snprintf(hex, sizeof(hex), "%02X ", byte);
        log += hex;
    }

    // Parse and describe the message
    uint8_t status = (*message)[0];
    uint8_t type = status & 0xF0;
    uint8_t channel = (status & 0x0F) + 1;

    if (type == 0x90 && message->size() >= 3) {
        // Note On
        uint8_t note = (*message)[1];
        uint8_t velocity = (*message)[2];
        log += "| Note On: " + std::to_string(note) + " vel=" + std::to_string(velocity) + " ch=" + std::to_string(channel);
    } else if (type == 0x80 && message->size() >= 3) {
        // Note Off
        uint8_t note = (*message)[1];
        log += "| Note Off: " + std::to_string(note) + " ch=" + std::to_string(channel);
    } else if (type == 0xB0 && message->size() >= 3) {
        // Control Change
        uint8_t cc = (*message)[1];
        uint8_t value = (*message)[2];
        log += "| CC: " + std::to_string(cc) + "=" + std::to_string(value) + " ch=" + std::to_string(channel);
    } else if (status == 0xF8) {
        // Clock (don't spam log)
        return;
    }

    hardware->addLog(log);
}

} // namespace gruvbok
