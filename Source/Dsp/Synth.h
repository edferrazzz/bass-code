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
    // ---- v2 (wavetable engine); defaults keep the original sounds as they were
    int tableA = -1;       // AKWF wave index (Waves.inc); -1 = use `wave`
    int tableB = -1;       // second oscillator wave (-1 = off)
    float mixB = 0.0f;     // 0..1
    float semiB = 0.0f;    // osc B pitch offset (semitones)
    float detuneB = 0.0f;  // osc B detune (cents)
    float width = 0.0f;    // stereo spread of the unison (low end always mono)
    float fold = 0.0f;     // wavefolder on the mid/high band, 0..1
    float crush = 0.0f;    // bit/rate crush on the mid/high band, 0..1
    float keytrack = 0.0f; // filter follows the note, 0..1
    float noise = 0.0f;    // noise into the filter, 0..1
    float lfoToPitch = 0.0f; // LFO vibrato in semitones
};
int waveIndex(const char* name);   // AKWF wave by name, -1 if unknown
const std::vector<Preset>& presets();

struct Macros { float sub = 0.6f, drive = 0.4f, movement = 0.5f, punch = 0.6f; };

// Monophonic bass voice: osc(s) + sub -> 4-pole lowpass with envelope -> drive -> amp.
class BassSynth {
public:
    void prepare(double sampleRate);
    void setPreset(int index);
    void setPresetDirect(const Preset& p) { pre = p; }
    void setMacros(const Macros& m) { macros = m; }
    void setTempo(double bpm) { tempo = bpm > 20.0 ? bpm : 120.0; }
    void setSongPosition(double ppq) { lfoPpq = ppq; }

    void noteOn(int pitch, float velocity, bool slide);
    void noteOff(int pitch);
    void allNotesOff();
    void reset();

    // Stereo render, added into left/right (below ~150 Hz it is always mono).
    void render(float* left, float* right, int numSamples);

private:
    float oscillator(int wave, int table, double& phase, double inc, float fm);
    static float filterStep(std::array<float, 4>& st, float x, float g, float k);

    double sr = 48000.0, tempo = 120.0, lfoPpq = 0.0;
    Preset pre {};
    Macros macros;

    // voice state
    int heldPitch = -1;
    std::vector<int> stack;      // held keys (legato)
    float vel = 0.0f;
    double freq = 55.0, targetFreq = 55.0;
    std::array<double, 3> phase {}, phaseB {};
    double subPhase = 0.0, fmPhase = 0.0;
    bool gate = false;
    float amp = 0.0f, ampTarget = 0.0f;
    int ampStage = 0;            // 0 idle, 1 attack, 2 decay/sustain, 3 release
    float fEnv = 0.0f, pEnv = 0.0f;
    float click = 0.0f;
    float glideCoef = 1.0f;
    std::array<float, 4> s {}, sSide {};   // filter state (mid, side)
    float dcX = 0.0f, dcY = 0.0f, splitLp1 = 0.0f, splitLp2 = 0.0f, sideHp = 0.0f, sideHpIn = 0.0f;
    float crushHold = 0.0f, crushPhase = 0.0f;
    uint32_t noise = 22222u;
};

} // namespace bass
