#include "Synth.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <juce_dsp/juce_dsp.h>

namespace bass {

namespace {
#include "Waves.inc"

// Band-limited copies of each AKWF wave, one per octave (256, 128 ... 1 harmonics).
struct WaveBank {
    static constexpr int levels = 9;
    std::vector<float> data;   // [wave][level][kWaveLen + 1]
    WaveBank() : data((size_t) kNumWaves * levels * (kWaveLen + 1)) {
        juce::dsp::FFT fft(11);   // 2048
        std::vector<float> buf(2 * kWaveLen);
        for (int w = 0; w < kNumWaves; ++w) {
            std::fill(buf.begin(), buf.end(), 0.0f);
            std::memcpy(buf.data(), kWaveData[w], sizeof(float) * kWaveLen);
            fft.performRealOnlyForwardTransform(buf.data());
            const std::vector<float> spectrum(buf);
            for (int l = 0; l < levels; ++l) {
                const int maxH = 256 >> l;
                std::vector<float> b(spectrum);
                for (int h = maxH + 1; h <= kWaveLen / 2; ++h) { b[(size_t) (2 * h)] = 0.0f; b[(size_t) (2 * h + 1)] = 0.0f; }
                b[0] = b[1] = 0.0f;
                fft.performRealOnlyInverseTransform(b.data());
                float peak = 1.0e-6f;
                for (int i = 0; i < kWaveLen; ++i) peak = std::max(peak, std::abs(b[(size_t) i]));
                float* dst = table(w, l);
                for (int i = 0; i < kWaveLen; ++i) dst[i] = b[(size_t) i] / peak * 0.9f;
                dst[kWaveLen] = dst[0];
            }
        }
    }
    float* table(int w, int l) { return data.data() + ((size_t) w * levels + (size_t) l) * (kWaveLen + 1); }
    float read(int w, double phase, double inc) {
        const double maxH = 0.5 / std::max(inc, 1.0e-7);
        int l = 0;
        while (l < levels - 1 && (double) (256 >> l) > maxH) ++l;
        const float* t = table(w, l);
        const double pos = phase * kWaveLen;
        const int i = (int) pos;
        const float f = (float) (pos - i);
        return t[i] + (t[i + 1] - t[i]) * f;
    }
};
WaveBank& bank() { static WaveBank b; return b; }
} // namespace

int waveIndex(const char* name) {
    for (int i = 0; i < kNumWaves; ++i) if (std::strcmp(kWaveNames[i], name) == 0) return i;
    return -1;
}

namespace {
constexpr double twoPi = 6.283185307179586;

double midiToHz(double m) { return 440.0 * std::pow(2.0, (m - 69.0) / 12.0); }

float polyBlep(double t, double dt) {
    if (t < dt) { t /= dt; return (float) (t + t - t * t - 1.0); }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
    return 0.0f;
}
} // namespace

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
    s = {}; sSide = {};
    dcX = dcY = splitLp1 = splitLp2 = sideHp = sideHpIn = 0.0f;
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
        if (!gate) { subPhase = 0.0; for (auto& ph : phase) ph = 0.0; for (auto& ph : phaseB) ph = 0.0; fmPhase = 0.0; }
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

float BassSynth::oscillator(int wave, int table, double& ph, double inc, float fm) {
    const double t = ph;
    float v;
    if (table >= 0) {
        // FM index bends the read position (phase distortion) so wavetables can growl too
        double rp = fm > 0.0f ? t + fm * 0.08 * std::sin(twoPi * t) : t;
        rp -= std::floor(rp);
        v = bank().read(table, rp, inc * (1.0 + fm * 0.5));
    } else switch (wave) {
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

float BassSynth::filterStep(std::array<float, 4>& st, float x, float G, float k) {
    float in = std::tanh(x - k * st[3]);
    for (int i = 0; i < 4; ++i) {
        const float v = (in - st[(size_t) i]) * G;
        const float y = v + st[(size_t) i];
        st[(size_t) i] = y + v;
        in = y;
    }
    return in * (1.0f + k * 0.35f);
}

void BassSynth::render(float* left, float* right, int n) {
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
    const float k = pre.resonance * 3.6f;
    const float splitCoef = 1.0f - std::exp(-6.2831853f * 140.0f / (float) sr);
    const float sideHpCoef = std::exp(-6.2831853f * 160.0f / (float) sr);
    const bool v2 = pre.tableA >= 0;
    const double semiRatioB = std::pow(2.0, (pre.semiB + pre.detuneB / 100.0) / 12.0);

    for (int i = 0; i < n; ++i) {
        if (ampStage == 1) {
            amp += attackInc;
            if (amp >= 1.0f) { amp = 1.0f; ampStage = 2; }
        } else if (ampStage == 2) {
            if (decayCoef > 0) amp = pre.sustain + (amp - pre.sustain) * decayCoef;
        } else if (ampStage == 3) {
            amp *= releaseCoef;
            if (amp < 1.0e-4f) { amp = 0.0f; ampStage = 0; }
        }
        lfoPpq += ppqInc;
        if (ampStage == 0) continue;

        if (glideCoef > 0.0f) freq = targetFreq + (freq - targetFreq) * glideCoef;
        else freq = targetFreq;

        float lfo = 0.0f;
        if (pre.wobble > 0) {
            const double beats = pre.wobble / 4.0;
            lfo = 0.5f - 0.5f * (float) std::cos(twoPi * std::fmod(lfoPpq, beats) / beats);
        }

        const double f = freq * std::pow(2.0, (pre.pitchEnv * pEnv * (0.6 + 0.8 * pu) + pre.lfoToPitch * (lfo - 0.5f)) / 12.0);
        const double inc = f / sr;
        const float fmIndex = pre.fm * (0.6f + 1.4f * m) * (pre.wobble > 0 ? (0.25f + lfoDepth * lfo * 1.6f) : (0.5f + fEnv));

        float mid = 0.0f, side = 0.0f;
        for (int v = 0; v < voices; ++v) {
            const float cents = voices == 1 ? 0.0f : voices == 2 ? (v == 0 ? -0.5f : 0.5f) * pre.detune : (float) (v - 1) * pre.detune;
            const double vi = inc * std::pow(2.0, cents / 1200.0);
            float o = oscillator(pre.wave, pre.tableA, phase[(size_t) v], vi, fmIndex);
            if (pre.tableB >= 0 && pre.mixB > 0.0f)
                o = o * (1.0f - pre.mixB * 0.5f) + pre.mixB * oscillator(0, pre.tableB, phaseB[(size_t) v], vi * semiRatioB, 0.0f);
            const float pan = voices == 1 ? 0.0f : voices == 2 ? (v == 0 ? -1.0f : 1.0f) : (float) (v - 1);
            mid += o;
            side += o * pan * pre.width;
        }
        mid *= uniNorm; side *= uniNorm;
        if (pre.noise > 0.0f) {
            noise = noise * 1664525u + 1013904223u;
            mid += pre.noise * 0.5f * ((float) (noise >> 9) / 4194304.0f - 1.0f);
        }

        float cutoff = cutoffBase * std::pow(2.0f, envOct * fEnv);
        if (pre.keytrack > 0.0f) cutoff *= (float) std::pow(f / 55.0, pre.keytrack);
        if (pre.wobble > 0) cutoff *= std::pow(2.0f, (lfo - 0.5f) * 3.0f * lfoDepth);
        const float fc = std::clamp(cutoff, 20.0f, (float) (sr * 0.45));
        const float g = (float) std::tan(3.141592653589793 * fc / sr);
        const float G = g / (1.0f + g);
        float x = filterStep(s, mid, G, k);
        float sx = pre.width > 0.0f ? filterStep(sSide, side, G, k) : 0.0f;

        float y;
        if (v2) {
            // multiband: the low band stays clean, only the mids/highs get the dirt
            splitLp1 += (x - splitLp1) * splitCoef;
            splitLp2 += (splitLp1 - splitLp2) * splitCoef;
            const float low = splitLp2;
            float high = x - low;
            float hd = high * driveGain * 0.9f;
            if (pre.fold > 0.0f) hd = std::sin(hd * (1.0f + pre.fold * 3.0f)) * (0.6f + 0.4f * pre.fold) + hd * (1.0f - pre.fold) * 0.3f;
            hd = std::tanh(hd);
            if (pre.crush > 0.0f) {
                crushPhase += 1.0f - pre.crush * 0.92f;
                if (crushPhase >= 1.0f) { crushPhase -= 1.0f; crushHold = hd; }
                const float steps = 4.0f + (1.0f - pre.crush) * 60.0f;
                hd = std::round(crushHold * steps) / steps;
            }
            y = low * 1.1f + hd * (0.55f + 0.25f * drive);
            sx = std::tanh(sx * driveGain * 0.8f);
        } else {
            y = std::tanh(x * driveGain * 0.6f) * driveNorm * 0.55f;
        }

        const float sub = (float) std::sin(twoPi * subPhase) * subGain;
        subPhase += inc * 0.5;
        if (subPhase >= 1.0) subPhase -= 1.0;
        const float clickSig = click * pu * 0.25f * (float) std::sin(twoPi * subPhase * 4.0);

        const float a = amp * (0.55f + 0.45f * vel);
        float mOut = (y + sub * 0.6f + clickSig) * a;
        const float dc = mOut - dcX + 0.995f * dcY;
        dcX = mOut; dcY = dc;
        // side: highpassed so the low end never goes stereo
        const float sIn = sx * a * 0.5f;
        sideHp = sideHpCoef * (sideHp + sIn - sideHpIn);
        sideHpIn = sIn;
        left[i] += (dc + sideHp) * 0.7f;
        right[i] += (dc - sideHp) * 0.7f;

        fEnv *= fDecayCoef;
        pEnv *= pDecayCoef;
        click *= clickCoef;
    }
}

} // namespace bass
