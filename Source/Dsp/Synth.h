#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace bass {

// One sound ("SOUND" menu). The 4 macro knobs move several of these at once.
struct Preset {
    const char* name;
    int wave;          // 0 saw, 1 square, 2 sine/triangle, 3 FM growl
    int unison;        // 1..3 voices
    float detune;      // cents between unison voices
    float sub;         // sine one octave down, 0..1
    float cutoff;      // Hz
    float resonance;   // 0..1
    float envAmount;   // octaves of filter envelope
    float decay;       // filter envelope decay (s)
    float attackMs;
    float ampDecay;    // s (0 = no decay, sustain only)
    float sustain;     // 0..1
    float releaseMs;
    float drive;       // 0..1
    float glideMs;
    int wobble;        // LFO period in 1/16 steps (0 = off)
    float fm;          // FM amount (wave 3) / harmonic brightness
    float pitchEnv;    // semitones of pitch drop at the start (log drum, slap)
    float pitchDecay;  // s
};
const std::vector<Preset>& presets();

struct Macros { float sub = 0.6f, drive = 0.4f, movement = 0.5f, punch = 0.6f; };

// Monophonic bass voice: osc(s) + sub -> 4-pole lowpass with envelope -> drive -> amp.
class BassSynth {
public:
    void prepare(double sampleRate);
    void setPreset(int index);
    void setMacros(const Macros& m) { macros = m; }
    void setTempo(double bpm) { tempo = bpm > 20.0 ? bpm : 120.0; }
    void setSongPosition(double ppq) { lfoPpq = ppq; }

    void noteOn(int pitch, float velocity, bool slide);
    void noteOff(int pitch);
    void allNotesOff();
    void reset();

    // Mono render, added into out.
    void render(float* out, int numSamples);

private:
    float oscillator(int wave, double& phase, double inc, float fm);
    float filterStep(float x, float cutoffHz);

    double sr = 48000.0, tempo = 120.0, lfoPpq = 0.0;
    Preset pre {};
    Macros macros;

    // voice state
    int heldPitch = -1;
    std::vector<int> stack;      // held keys (legato)
    float vel = 0.0f;
    double freq = 55.0, targetFreq = 55.0;
    std::array<double, 3> phase {};
    double subPhase = 0.0, fmPhase = 0.0;
    bool gate = false;
    float amp = 0.0f, ampTarget = 0.0f;
    int ampStage = 0;            // 0 idle, 1 attack, 2 decay/sustain, 3 release
    float fEnv = 0.0f, pEnv = 0.0f;
    float click = 0.0f;
    float glideCoef = 1.0f;
    std::array<float, 4> s {};   // filter state
    float dcX = 0.0f, dcY = 0.0f;
    uint32_t noise = 22222u;
};

} // namespace bass
