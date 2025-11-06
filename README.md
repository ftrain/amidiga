# GRUVBOK

## The most important part

The "OS" layer of the software is purely about creating a data structure by hitting buttons and moving pots.

At all time, the system keeps playing and looping through that data structure.

The Lua interface recieves data events and transforms them into midi.

Each Lua mode correlates to a midi channel.

## Desktop first

Create a desktop app for testing first. Yes, this runs on a teensy, but the first thing to do is create a simple desktop app that emulates the buttons, but then use the exact same C++ and Lua to actually power the midi generation engine. You may want to use JUCE and SDL.

## Hardware

I have a simple MIDI controller. It has:
- 16 momentary switches, B1..B16.
- 4 rotary pots, R1..R4.
- 4 slider pots, S1..S4.

All of the above are wired into a Teensy 4.1 Arduino board.

The song on the groovebox is always playing.

## Software

The four rotary pots have global functions:
R1: Mode switch: 0-15.
R2: Tempo: 0 to 1000 BPM.
R3: Pattern select: 1-32.
R4: Track select: 1-8.

## Core Data Model

- The system plays a Song.
- Songs repeat forever.
- It plays up until there is no more data entered. I.e. if the user has entered only one pattern it plays only one bar on repeat.
- A drum machine mode might only have one bar entered which would loop forever, while a sequencer might have eight bars, and all bars would loop.
- The system plays in 15 simultaneous Modes; each mode is a MIDI channel.
- A Mode contains 32 Patterns.
- Each Pattern has 8 Tracks.
- Each Track has 16 Events.
- Each Event has:
  - Switch status (on or off)
  - Values for the four slider pots (0-127) (this could be a struct or a bit-packed 32-bit integer, think about what's most efficient.)

Or boiling it down:

```
Song = Mode[8]
Mode = Pattern[32]
Pattern = Track[8]
Track = Event[16]
Event = {Switch, Pot[4]}

Mode = 0..15
Switch = 0|1
Pot = 0..127

```

### OS Layer

The OS layer is in C++. 

The easiest way to understand the OS Layer is that it allows you to program events onto steps by hitting buttons B1..B16 and moving sliders S1...S4, and it keeps track of which patterns, tracks, and modes you are in.

An simple, textual `.ini` file makes it possible to identify which # button is wired to which pin, and the wiring of the pots. It should be possible to add comments to that file.

The OS handles mode switches, load, and save.

Each of the modes plays on a different MIDI channel.

Then it should invoke the Lua function:
- It should pass a reference to the function the Track value and exactly one Event, thus passing it the Switch status and the Pot statuses.
- A reference to the global context should be available.
- The function should return MIDI events with delta timing to capture "note off."
- The OS should schedule those events and transmit them.
- The OS should provide a helpful interface with note(), off(), stopall(), and other convenience methods. It should be very, very concise and obvious and hide as much of the MIDI standard as possible.

All changes are instantly persisted to the current song in memory.


### Modes
0. Boot.
- Load a song by tapping once. Teensy LED should blink once.
- Save a song by tapping twice. Teensy LED should blink twice.
- Erase a song by tapping once, then long-tapping. Teensy LED should blink three times.
- Set reasonable defaults.

1. Drum Machine.
- This is an 808-style beat machine.
- Switches set beats on and off.
- Track select pot lets me switch between tracks.
- Track 8 is accent.

2. Acid Sequencer
S1: Octave of active note
S2: Length of current note
S3: CC Portamento
S4: CC Filter

Feel free to suggest other ideas.







