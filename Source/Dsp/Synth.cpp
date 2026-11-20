#include "Synth.h"

#include <algorithm>
#include <cmath>

namespace bass {

namespace {
constexpr double twoPi = 6.283185307179586;

double midiToHz(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }

float polyBlep(double t, double dt) {
    if (t < dt) { t /= dt; return (float) (t + t - t * t - 1.0); }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
    return 0.0f;
}
} // namespace

// Index order matters: Music.cpp maps each style to 6 of these.
const std::vector<Preset>& presets() {
    //  name            wave uni det  sub   cut   res   env  dec   att  aDec sus  rel  drv  gli wob fm   pEnv pDec
    static const std::vector<Preset> p {
        // PSY 0-5
        { "GOA SAW",       0, 1, 0,   0.55f, 260,  0.35f, 3.4f, 0.09f, 1, 0.12f, 0.0f, 25, 0.25f, 0, 0, 0.0f, 0, 0 },
        { "FULL ON",       0, 2, 7,   0.50f, 300,  0.30f, 3.8f, 0.07f, 1, 0.10f, 0.0f, 22, 0.40f, 0, 0, 0.0f, 0, 0 },
        { "ROLLING SUB",   2, 1, 0,   0.70f, 380,  0.10f, 1.6f, 0.06f, 1, 0.11f, 0.0f, 20, 0.15f, 0, 0, 0.0f, 0, 0 },
        { "DARK PSY",      1, 1, 0,   0.55f, 200,  0.50f, 3.2f, 0.06f, 1, 0.10f, 0.0f, 20, 0.45f, 0, 0, 0.0f, 0, 0 },
        { "PROG PLUCK",    0, 2, 5,   0.45f, 340,  0.25f, 3.0f, 0.12f, 1, 0.16f, 0.0f, 40, 0.20f, 0, 0, 0.0f, 0, 0 },
        { "ACID ROLL",     0, 1, 0,   0.40f, 280,  0.72f, 3.6f, 0.13f, 1, 0.14f, 0.0f, 30, 0.35f, 0, 0, 0.0f, 0, 0 },
        // HOUSE 6-11
        { "DEEP SUB",      2, 1, 0,   0.65f, 700,  0.05f, 1.0f, 0.20f, 4, 0.00f, 0.9f, 60, 0.10f, 0, 0, 0.0f, 0, 0 },
        { "ORGAN BASS",    3, 1, 0,   0.50f, 900,  0.15f, 1.6f, 0.18f, 2, 0.35f, 0.4f, 60, 0.20f, 0, 0, 0.55f, 0, 0 },
        { "HOUSE PLUCK",   0, 1, 0,   0.50f, 330,  0.30f, 3.0f, 0.18f, 2, 0.28f, 0.1f, 50, 0.20f, 0, 0, 0.0f, 0, 0 },
        { "SOFT REESE",    0, 3, 14,  0.55f, 520,  0.20f, 1.2f, 0.30f, 6, 0.00f, 0.9f, 80, 0.25f, 0, 0, 0.0f, 0, 0 },
        { "ROUND SQUARE",  1, 1, 0,   0.50f, 450,  0.20f, 2.0f, 0.15f, 2, 0.30f, 0.3f, 50, 0.15f, 0, 0, 0.0f, 0, 0 },
        { "GARAGE",        0, 2, 6,   0.55f, 400,  0.35f, 2.6f, 0.14f, 2, 0.25f, 0.2f, 45, 0.35f, 0, 0, 0.0f, 0, 0 },
        // AFRO HOUSE 12-17
        { "LOG DRUM",      2, 1, 0,   0.55f, 1400, 0.10f, 1.0f, 0.10f, 1, 0.22f, 0.0f, 60, 0.30f, 0, 0, 0.0f, 7.0f, 0.045f },
        { "DEEP AFRO",     2, 1, 0,   0.60f, 800,  0.10f, 1.2f, 0.20f, 3, 0.45f, 0.5f, 70, 0.15f, 0, 0, 0.0f, 0, 0 },
        { "WOOD",          1, 1, 0,   0.45f, 600,  0.35f, 2.4f, 0.08f, 1, 0.16f, 0.0f, 40, 0.20f, 0, 0, 0.0f, 3.0f, 0.03f },
        { "TALKING",       3, 1, 0,   0.50f, 1000, 0.25f, 1.4f, 0.14f, 1, 0.25f, 0.0f, 50, 0.25f, 0, 0, 0.75f, 4.0f, 0.06f },
        { "WARM PLUCK",    0, 1, 0,   0.55f, 380,  0.20f, 2.4f, 0.20f, 2, 0.35f, 0.1f, 60, 0.15f, 0, 0, 0.0f, 0, 0 },
        { "MARIMBA SUB",   2, 1, 0,   0.50f, 1200, 0.10f, 1.0f, 0.10f, 1, 0.30f, 0.0f, 60, 0.10f, 0, 0, 0.0f, 12.0f, 0.02f },
        // BRAZILIAN BASS 18-23
        { "BR BOUNCE",     3, 1, 0,   0.55f, 700,  0.30f, 2.2f, 0.12f, 1, 0.00f, 0.85f, 40, 0.55f, 30, 0, 0.90f, 0, 0 },
        { "WOBBLE 1/8",    3, 1, 0,   0.55f, 400,  0.40f, 1.0f, 0.20f, 2, 0.00f, 0.9f, 50, 0.60f, 40, 2, 0.80f, 0, 0 },
        { "SLAP",          1, 2, 8,   0.50f, 900,  0.30f, 2.4f, 0.08f, 1, 0.20f, 0.3f, 40, 0.45f, 0, 0, 0.0f, 5.0f, 0.03f },
        { "GROWL",         3, 2, 9,   0.50f, 600,  0.45f, 1.8f, 0.15f, 2, 0.00f, 0.9f, 50, 0.70f, 30, 4, 1.10f, 0, 0 },
        { "DIRTY SAW",     0, 3, 16,  0.55f, 650,  0.35f, 2.0f, 0.18f, 2, 0.00f, 0.85f, 50, 0.65f, 25, 0, 0.0f, 0, 0 },
        { "WOBBLE 1/4",    3, 1, 0,   0.55f, 350,  0.45f, 0.8f, 0.25f, 3, 0.00f, 0.9f, 60, 0.55f, 40, 4, 0.85f, 0, 0 },
        // ELETRONICA 24-29
        { "DRIVE SAW",     0, 2, 9,   0.55f, 500,  0.30f, 2.4f, 0.15f, 1, 0.20f, 0.3f, 40, 0.50f, 0, 0, 0.0f, 0, 0 },
        { "STAB",          1, 2, 10,  0.45f, 700,  0.40f, 2.8f, 0.10f, 1, 0.14f, 0.0f, 40, 0.35f, 0, 0, 0.0f, 0, 0 },
        { "SLIDE 303",     0, 1, 0,   0.40f, 300,  0.78f, 3.4f, 0.16f, 1, 0.20f, 0.3f, 40, 0.40f, 70, 0, 0.0f, 0, 0 },
        { "SUB CLEAN",     2, 1, 0,   0.70f, 600,  0.05f, 0.8f, 0.20f, 4, 0.00f, 0.9f, 60, 0.05f, 0, 0, 0.0f, 0, 0 },
        { "REESE",         0, 3, 18,  0.55f, 480,  0.30f, 1.0f, 0.30f, 6, 0.00f, 0.9f, 90, 0.45f, 0, 8, 0.0f, 0, 0 },
        { "TECHNO RUMBLE", 2, 1, 0,   0.60f, 300,  0.35f, 2.0f, 0.25f, 2, 0.40f, 0.2f, 80, 0.55f, 0, 0, 0.0f, 0, 0 },
    };
    return p;
}

void BassSynth::prepare(double sampleRate) {
    sr = sampleRate > 0 ? sampleRate : 48000.0;
    reset();
}

void BassSynth::setPreset(int index) {
    const auto& all = presets();
    pre = all[(size_t) std::clamp(index, 0, (int) all.size() - 1)];
}

void BassSynth::reset() {
    stack.clear();
    heldPitch = -1;
    gate = false;
    amp = 0.0f; ampStage = 0;
    fEnv = pEnv = click = 0.0f;
    s = {};
    dcX = dcY = 0.0f;
}

void BassSynth::noteOn(int pitch, float velocity, bool slide) {
    stack.erase(std::remove(stack.begin(), stack.end(), pitch), stack.end());
    stack.push_back(pitch);
    targetFreq = midiToHz(pitch);
    const bool glide = gate && (slide || pre.glideMs > 0.0f);
    const float glideMs = slide ? std::max(pre.glideMs, 70.0f) : pre.glideMs;
    if (glide && glideMs > 0.0f) {
        glideCoef = (float) std::exp(-1.0 / (glideMs * 0.001 * sr));
    } else {
        glideCoef = 0.0f;
        freq = targetFreq;
    }
    heldPitch = pitch;
    vel = velocity;
    if (!(gate && slide)) {   // retrigger (slides stay legato)
        ampStage = 1;
        fEnv = 1.0f;
        pEnv = 1.0f;
        click = 1.0f;
        if (!gate) { subPhase = 0.0; for (auto& ph : phase) ph = 0.0; fmPhase = 0.0; }
    }
    gate = true;
}

void BassSynth::noteOff(int pitch) {
    stack.erase(std::remove(stack.begin(), stack.end(), pitch), stack.end());
    if (pitch != heldPitch) return;
    if (!stack.empty()) {
        heldPitch = stack.back();
        targetFreq = midiToHz(heldPitch);
        glideCoef = (float) std::exp(-1.0 / (std::max(pre.glideMs, 30.0f) * 0.001 * sr));
        return;
    }
    gate = false;
    ampStage = 3;
}

void BassSynth::allNotesOff() {
    stack.clear();
    gate = false;
    if (ampStage != 0) ampStage = 3;
}

float BassSynth::oscillator(int wave, double& ph, double inc, float fm) {
    const double t = ph;
    float v;
    switch (wave) {
        case 0: v = (float) (2.0 * t - 1.0) - polyBlep(t, inc); break;
        case 1: {
            v = t < 0.5 ? 1.0f : -1.0f;
            v += polyBlep(t, inc);
            v -= polyBlep(std::fmod(t + 0.5, 1.0), inc);
            v *= 0.8f;
            break;
        }
        case 2: v = (float) (std::sin(twoPi * t) + 0.18 * std::sin(2.0 * twoPi * t)); break;
        default: v = (float) std::sin(twoPi * t + fm * std::sin(twoPi * t)); break;
    }
    ph += inc;
    if (ph >= 1.0) ph -= 1.0;
    return v;
}

float BassSynth::filterStep(float x, float cutoffHz) {
    const float fc = std::clamp(cutoffHz, 20.0f, (float) (sr * 0.45));
    const float g = (float) std::tan(3.141592653589793 * fc / sr);
    const float G = g / (1.0f + g);
    const float k = pre.resonance * 3.6f;
    float in = std::tanh(x - k * s[3]);
    for (int i = 0; i < 4; ++i) {
        const float v = (in - s[(size_t) i]) * G;
        const float y = v + s[(size_t) i];
        s[(size_t) i] = y + v;
        in = y;
    }
    return in * (1.0f + k * 0.35f);   // keep level with resonance up
}

void BassSynth::render(float* out, int n) {
    const float m = macros.movement, pu = macros.punch;
    const float subGain = std::clamp(pre.sub + (macros.sub - 0.5f) * 1.0f, 0.0f, 1.1f);
    const float drive = std::clamp(pre.drive + (macros.drive - 0.5f) * 0.9f, 0.0f, 1.0f);
    const float cutoffBase = pre.cutoff * std::pow(2.0f, (m - 0.5f) * 2.6f);
    const float envOct = pre.envAmount * (0.4f + 1.2f * m);
    const float fDecay = std::max(0.015f, pre.decay * (1.6f - 1.2f * pu));
    const float attack = std::max(0.3f, pre.attackMs * (1.6f - pu));
    const float fDecayCoef = std::exp(-1.0f / (fDecay * (float) sr));
    const float pDecayCoef = pre.pitchDecay > 0 ? std::exp(-1.0f / (pre.pitchDecay * (float) sr)) : 0.0f;
    const float clickCoef = std::exp(-1.0f / (0.004f * (float) sr));
    const float attackInc = 1.0f / (attack * 0.001f * (float) sr);
    const float decayCoef = pre.ampDecay > 0 ? std::exp(-1.0f / (pre.ampDecay * (float) sr)) : 0.0f;
    const float releaseCoef = std::exp(-1.0f / (pre.releaseMs * 0.001f * (float) sr));
    const double ppqInc = tempo / 60.0 / sr;
    const float driveGain = 1.0f + drive * 7.0f;
    const float driveNorm = 1.0f / std::tanh(driveGain * 0.6f);
    const int voices = std::clamp(pre.unison, 1, 3);
    const float uniNorm = 1.0f / std::sqrt((float) voices);
    const float lfoDepth = 0.35f + 0.65f * m;

    for (int i = 0; i < n; ++i) {
        // amp envelope
        if (ampStage == 1) {
            amp += attackInc;
            if (amp >= 1.0f) { amp = 1.0f; ampStage = 2; }
        } else if (ampStage == 2) {
            if (decayCoef > 0) amp = pre.sustain + (amp - pre.sustain) * decayCoef;
            else amp = pre.sustain > 0 ? pre.sustain + (amp - pre.sustain) * 0.999f : amp;
        } else if (ampStage == 3) {
            amp *= releaseCoef;
            if (amp < 1.0e-4f) { amp = 0.0f; ampStage = 0; }
        }
        lfoPpq += ppqInc;
        if (ampStage == 0) { out[i] += 0.0f; continue; }

        if (glideCoef > 0.0f) freq = targetFreq + (freq - targetFreq) * glideCoef;
        else freq = targetFreq;

        float lfo = 0.0f;
        if (pre.wobble > 0) {
            const double beats = pre.wobble / 4.0;
            lfo = 0.5f - 0.5f * (float) std::cos(twoPi * std::fmod(lfoPpq, beats) / beats);
        }

        const double f = freq * std::pow(2.0, (pre.pitchEnv * pEnv * (0.6 + 0.8 * pu)) / 12.0);
        const double inc = f / sr;
        const float fmIndex = pre.fm * (0.6f + 1.4f * m) * (pre.wobble > 0 ? (0.25f + lfoDepth * lfo * 1.6f) : (0.5f + fEnv));

        float osc = 0.0f;
        if (voices == 1) {
            osc = oscillator(pre.wave, phase[0], inc, fmIndex);
        } else {
            for (int v = 0; v < voices; ++v) {
                const float cents = voices == 2 ? (v == 0 ? -0.5f : 0.5f) * pre.detune : (float) (v - 1) * pre.detune;
                osc += oscillator(pre.wave, phase[(size_t) v], inc * std::pow(2.0, cents / 1200.0), fmIndex);
            }
            osc *= uniNorm;
        }

        float cutoff = cutoffBase * std::pow(2.0f, envOct * fEnv);
        if (pre.wobble > 0) cutoff *= std::pow(2.0f, (lfo - 0.5f) * 3.0f * lfoDepth);
        float x = filterStep(osc, cutoff);

        // sub sine (clean, under the filter so it never disappears)
        const float sub = (float) std::sin(twoPi * subPhase) * subGain;
        subPhase += inc * 0.5;
        if (subPhase >= 1.0) subPhase -= 1.0;

        // punch: short click/thump at the start of each note
        noise = noise * 1664525u + 1013904223u;
        const float clickSig = click * pu * 0.25f * (float) std::sin(twoPi * subPhase * 4.0);

        x = std::tanh(x * driveGain * 0.6f) * driveNorm;
        float y = (x * 0.55f + sub * 0.6f + clickSig) * amp * (0.55f + 0.45f * vel);

        // DC blocker
        const float dc = y - dcX + 0.995f * dcY;
        dcX = y; dcY = dc;
        out[i] += dc * 0.7f;

        fEnv *= fDecayCoef;
        pEnv *= pDecayCoef;
        click *= clickCoef;
    }
}

} // namespace bass
