#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace bass {

// ---------------------------------------------------------------------------
// Pattern: the bass line. Time is in 1/16 steps (4 steps = 1 beat).
// ---------------------------------------------------------------------------
struct Note {
    int start = 0;      // step
    int length = 1;     // steps
    int pitch = 36;     // MIDI note
    float velocity = 0.9f;
    bool slide = false; // glide into this note
};

struct Pattern {
    int bars = 4;
    std::vector<Note> notes;
    std::vector<int> barRoots;   // pitch class of each bar's chord root (for the labels)
    std::vector<bool> barMinor;  // chord quality per bar
    int key = 6;                 // pitch class actually used
    int scale = 0;               // index into scales() actually used
    int steps() const { return bars * 16; }
};

// ---------------------------------------------------------------------------
// Styles, grooves and scales
// ---------------------------------------------------------------------------
enum Style { psy = 0, house, afroHouse, brazilianBass, electronic, numStyles };

struct ScaleDef { const char* name; std::array<int, 7> steps; };
const std::vector<ScaleDef>& scales();          // 0 = MENOR ...

struct StyleDef {
    const char* name;
    int defaultBpm;
    std::array<const char*, 3> grooves;
    std::vector<int> sounds;     // indices into presets()
    int defaultScale;
    // FX defaults: lowpass (0..1), sweep, reverb, delay on, sidechain amount
    float lowpass, reverb, sidechain;
    bool sweep, delayOn;
};
const StyleDef& style(int s);

const char* noteName(int pitchClass);           // "C", "C#" ...
std::string noteLabel(int midiPitch);           // "F#1"

// ---------------------------------------------------------------------------
// Generator
// ---------------------------------------------------------------------------
struct GenSettings {
    int style = psy;
    int key = -1;        // -1 = PADRÃO (random)
    int scale = -1;      // -1 = PADRÃO (style default)
    int groove = 0;      // 0..2
    int bars = 4;
    int density = 1;     // 0 low, 1 mid, 2 high
    uint32_t seed = 1;
};

// Writes a full line. If keepRoots is given (VARIAÇÃO), the bar roots/key/scale are kept.
Pattern generate(const GenSettings&, const Pattern* keepRoots = nullptr);

} // namespace bass
