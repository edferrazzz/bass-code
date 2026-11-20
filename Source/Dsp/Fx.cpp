#include "Fx.h"

#include <algorithm>
#include <cmath>

namespace bass {

void FxChain::prepare(double sampleRate, int maxBlock) {
    sr = sampleRate > 0 ? sampleRate : 48000.0;
    dlL.assign((size_t) (sr * 3.0), 0.0f);
    dlR.assign((size_t) (sr * 3.0), 0.0f);
    wetL.assign((size_t) std::max(maxBlock, 16), 0.0f);
    wetR.assign((size_t) std::max(maxBlock, 16), 0.0f);
    reverb.setSampleRate(sr);
    reset();
}

void FxChain::reset() {
    lpS1 = lpS2 = lpS3 = lpS4 = 0.0f;
    std::fill(dlL.begin(), dlL.end(), 0.0f);
    std::fill(dlR.begin(), dlR.end(), 0.0f);
    dlPos = 0;
    reverb.reset();
    limGain = 1.0f;
    hpPrevIn = hpPrevOut = 0.0f;
}

void FxChain::process(const float* mono, float* left, float* right, int n, const FxSettings& fx,
                      double bpm, double ppqStart, double loopBeats) {
    if ((int) wetL.size() < n) { wetL.resize((size_t) n); wetR.resize((size_t) n); }
    bpm = bpm > 20.0 ? bpm : 120.0;
    loopBeats = loopBeats > 0.0 ? loopBeats : 16.0;
    const double ppqInc = bpm / 60.0 / sr;

    const int delaySamples = std::clamp((int) (0.75 * 60.0 / bpm * sr), 1, (int) dlL.size() - 1);
    const float delayMix = fx.delayOn ? fx.delay * 0.55f : 0.0f;
    const float hpCoef = std::exp(-6.2831853f * 180.0f / (float) sr);
    double ppq = ppqStart;

    for (int i = 0; i < n; ++i) {
        float x = mono[i];

        // LOWPASS (+ SWEEP)
        if (fx.lowpassOn) {
            float amount = fx.lowpass;
            if (fx.sweep) {
                const float frac = (float) std::fmod(std::max(0.0, ppq), loopBeats) / (float) loopBeats;
                amount = 0.1f + (fx.lowpass - 0.1f) * frac;
            }
            const float target = 60.0f * std::pow(2.0f, amount * 8.2f);
            cutoffSmoothed += (target - cutoffSmoothed) * 0.002f;
            const float fc = std::clamp(cutoffSmoothed, 30.0f, (float) sr * 0.45f);
            const float g = std::tan(3.14159265f * fc / (float) sr);
            const float G = g / (1.0f + g);
            float v;
            v = (x - lpS1) * G; float y1 = v + lpS1; lpS1 = y1 + v;
            v = (y1 - lpS2) * G; float y2 = v + lpS2; lpS2 = y2 + v;
            v = (y2 - lpS3) * G; float y3 = v + lpS3; lpS3 = y3 + v;
            v = (y3 - lpS4) * G; float y4 = v + lpS4; lpS4 = y4 + v;
            x = y4;
        }

        // SIDECHAIN: duck on every beat (where the kick is)
        if (fx.sidechainOn && fx.sidechain > 0.0f) {
            const float ph = (float) std::fmod(std::max(0.0, ppq), 1.0);
            const float shape = ph < 0.015f ? ph / 0.015f : std::exp(-(ph - 0.015f) / 0.14f);
            const float duck = 1.0f - fx.sidechain * 0.92f * shape;
            x *= std::clamp(duck, 0.0f, 1.0f);
        }

        x *= fx.volume * 1.25f;

        // send for delay/reverb: one-pole highpass so the low end stays mono and clean
        const float hpOut = hpCoef * (hpPrevOut + x - hpPrevIn);
        hpPrevIn = x; hpPrevOut = hpOut;

        float l = x, r = x;

        // DELAY (dotted 1/8, light ping-pong)
        if (delayMix > 0.0f) {
            int readPos = dlPos - delaySamples;
            if (readPos < 0) readPos += (int) dlL.size();
            const float dL = dlL[(size_t) readPos], dR = dlR[(size_t) readPos];
            dlL[(size_t) dlPos] = hpOut + dR * 0.38f;
            dlR[(size_t) dlPos] = dL * 0.38f;
            l += dL * delayMix;
            r += dR * delayMix;
        } else {
            dlL[(size_t) dlPos] = 0.0f;
            dlR[(size_t) dlPos] = 0.0f;
        }
        if (++dlPos >= (int) dlL.size()) dlPos = 0;

        wetL[(size_t) i] = hpOut;
        wetR[(size_t) i] = hpOut;
        left[i] = l;
        right[i] = r;
        ppq += ppqInc;
    }

    // REVERB on the highpassed send only
    if (fx.reverbOn && fx.reverb > 0.0f) {
        juce::Reverb::Parameters p;
        p.roomSize = 0.72f;
        p.damping = 0.45f;
        p.width = 1.0f;
        p.dryLevel = 0.0f;
        p.wetLevel = fx.reverb * 0.7f;
        reverb.setParameters(p);
        reverb.processStereo(wetL.data(), wetR.data(), n);
        for (int i = 0; i < n; ++i) { left[i] += wetL[(size_t) i]; right[i] += wetR[(size_t) i]; }
    }

    // limiter: never clips
    const float rel = std::exp(-1.0f / (0.12f * (float) sr));
    for (int i = 0; i < n; ++i) {
        const float peak = std::max(std::abs(left[i]), std::abs(right[i]));
        const float want = peak > 0.89f ? 0.89f / peak : 1.0f;
        limGain = want < limGain ? want : want + (limGain - want) * rel;
        limGain = std::min(limGain, want);
        left[i] *= limGain;
        right[i] *= limGain;
    }
}

} // namespace bass
