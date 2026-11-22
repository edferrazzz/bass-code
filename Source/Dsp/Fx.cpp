#include "Fx.h"
#include "Core/Genres.h"

#include <algorithm>
#include <cmath>

namespace bass {

void FxChain::prepare(double sampleRate, int maxBlock) {
    sr = sampleRate > 0 ? sampleRate : 48000.0;
    for (auto* v : { &dlL, &dlR, &thL, &thR }) v->assign((size_t) (sr * 3.0), 0.0f);
    wetL.assign((size_t) std::max(maxBlock, 16), 0.0f);
    wetR.assign((size_t) std::max(maxBlock, 16), 0.0f);
    reverb.setSampleRate(sr);
    reset();
}

void FxChain::reset() {
    lpL = {}; lpR = {}; hpL = {}; hpR = {};
    for (auto* v : { &dlL, &dlR, &thL, &thR }) std::fill(v->begin(), v->end(), 0.0f);
    dlPos = 0;
    reverb.reset();
    limGain = 1.0f;
    hpPrevIn = hpPrevOut = 0.0f;
    cutoffSmoothed = 18000.0f;
    hpSmoothed = 20.0f;
}

void FxChain::process(float* left, float* right, int n, const FxSettings& fx, double bpm, double ppqStart, double loopBeats) {
    if ((int) wetL.size() < n) { wetL.resize((size_t) n); wetR.resize((size_t) n); }
    bpm = bpm > 20.0 ? bpm : 120.0;
    loopBeats = loopBeats > 0.0 ? loopBeats : 16.0;
    const double ppqInc = bpm / 60.0 / sr;
    const int size = (int) dlL.size();
    const int delaySamples = std::clamp((int) (0.75 * 60.0 / bpm * sr), 1, size - 1);
    const int throwSamples = std::clamp((int) (0.25 * 60.0 / bpm * sr), 1, size - 1);
    const float sendHp = std::exp(-6.2831853f * 180.0f / (float) sr);
    const float amt = std::clamp(fx.automationAmount, 0.0f, 1.0f);
    const int a = fx.automation;
    const float L = (float) loopBeats;
    const float barLen = std::min(4.0f, L);
    double ppq = ppqStart;

    for (int i = 0; i < n; ++i) {
        const float pos = (float) std::fmod(std::max(0.0, ppq), loopBeats);   // beats into the loop
        const float endBar = std::clamp((pos - (L - barLen)) / barLen, 0.0f, 1.0f);   // 0..1 across the last bar
        const bool lastBeat = pos >= L - 1.0f;
        const float half = L * 0.5f;
        const float midBar = (pos >= half - barLen && pos < half) ? (pos - (half - barLen)) / barLen : 0.0f;

        // ---- what the automation does at this point of the loop
        float lp = 1.0f, hpAmt = 0.0f, revBoost = 0.0f, gate = 1.0f;
        bool throwOn = false;
        switch (a) {
            case autoHpEnd:         hpAmt = endBar * endBar; break;
            case autoLpEnd:         lp = 1.0f - 0.6f * std::pow(endBar, 1.5f); break;
            case autoLpOpen:        lp = 0.3f + 0.7f * (pos / L); break;
            case autoReverbBuild:   revBoost = endBar * endBar; break;
            case autoDelayThrow:    throwOn = lastBeat; break;
            case autoStutterEnd:    if (lastBeat) gate = std::fmod(pos * 8.0f, 1.0f) < 0.5f ? 1.0f : 0.0f; break;
            case autoHpReverbBuild: hpAmt = endBar * endBar * 0.8f; revBoost = endBar * endBar; break;
            case autoFilterWobble:  lp = 1.0f - 0.45f * (0.5f - 0.5f * std::cos(6.2831853f * std::fmod(pos, 1.0f))); break;
            case autoLpDipHalf:     lp = 1.0f - 0.55f * std::pow(midBar, 1.5f); break;
            default: break;
        }
        lp = 1.0f - (1.0f - lp) * amt;
        hpAmt *= amt;
        revBoost *= amt;
        gate = 1.0f - (1.0f - gate) * amt;

        float xl = left[i], xr = right[i];

        // lowpass (24 dB)
        const float lpTarget = 60.0f * std::pow(2.0f, lp * 8.2f);
        cutoffSmoothed += (lpTarget - cutoffSmoothed) * 0.004f;
        if (cutoffSmoothed < 16000.0f) {
            const float fc = std::clamp(cutoffSmoothed, 30.0f, (float) sr * 0.45f);
            const float g = std::tan(3.14159265f * fc / (float) sr), G = g / (1.0f + g);
            auto lp4 = [G](std::array<float, 4>& st, float in) {
                for (auto& z : st) { const float v = (in - z) * G; const float y = v + z; z = y + v; in = y; }
                return in;
            };
            xl = lp4(lpL, xl); xr = lp4(lpR, xr);
        }
        // highpass (12 dB), rises up to ~900 Hz
        const float hpTarget = 20.0f * std::pow(45.0f, hpAmt);
        hpSmoothed += (hpTarget - hpSmoothed) * 0.004f;
        if (hpSmoothed > 25.0f) {
            const float g = std::tan(3.14159265f * hpSmoothed / (float) sr), G = g / (1.0f + g);
            auto hp2 = [G](std::array<float, 2>& st, float in) {
                for (auto& z : st) { const float v = (in - z) * G; const float low = v + z; z = low + v; in = in - low; }
                return in;
            };
            xl = hp2(hpL, xl); xr = hp2(hpR, xr);
        }

        // sidechain: duck on every beat
        if (fx.sidechain > 0.0f) {
            const float ph = (float) std::fmod(std::max(0.0, ppq), 1.0);
            const float shape = ph < 0.015f ? ph / 0.015f : std::exp(-(ph - 0.015f) / 0.14f);
            const float duck = std::clamp(1.0f - fx.sidechain * 0.92f * shape, 0.0f, 1.0f);
            xl *= duck; xr *= duck;
        }
        xl *= fx.volume * 1.25f * gate;
        xr *= fx.volume * 1.25f * gate;

        const float x = 0.5f * (xl + xr);
        const float hpOut = sendHp * (hpPrevOut + x - hpPrevIn);
        hpPrevIn = x; hpPrevOut = hpOut;

        float l = xl, r = xr;
        // base delay (dotted 1/8)
        {
            int rp = dlPos - delaySamples; if (rp < 0) rp += size;
            const float dL = dlL[(size_t) rp], dR = dlR[(size_t) rp];
            dlL[(size_t) dlPos] = fx.delay > 0.0f ? hpOut + dR * 0.38f : 0.0f;
            dlR[(size_t) dlPos] = dL * 0.38f;
            l += dL * fx.delay * 0.55f; r += dR * fx.delay * 0.55f;
        }
        // delay throw (fast 1/16 ping-pong, fed only on the last beat)
        {
            int rp = dlPos - throwSamples; if (rp < 0) rp += size;
            const float dL = thL[(size_t) rp], dR = thR[(size_t) rp];
            thL[(size_t) dlPos] = (throwOn ? hpOut * amt : 0.0f) + dR * 0.55f;
            thR[(size_t) dlPos] = dL * 0.55f;
            l += dL * 0.7f; r += dR * 0.7f;
        }
        if (++dlPos >= size) dlPos = 0;

        const float send = std::min(1.0f, fx.reverb + revBoost * 0.9f);
        wetL[(size_t) i] = hpOut * send;
        wetR[(size_t) i] = hpOut * send;
        left[i] = l; right[i] = r;
        ppq += ppqInc;
    }

    juce::Reverb::Parameters p;
    p.roomSize = 0.72f; p.damping = 0.45f; p.width = 1.0f; p.dryLevel = 0.0f; p.wetLevel = 0.75f;
    reverb.setParameters(p);
    reverb.processStereo(wetL.data(), wetR.data(), n);
    for (int i = 0; i < n; ++i) { left[i] += wetL[(size_t) i]; right[i] += wetR[(size_t) i]; }

    const float rel = std::exp(-1.0f / (0.12f * (float) sr));
    for (int i = 0; i < n; ++i) {
        const float peak = std::max(std::abs(left[i]), std::abs(right[i]));
        const float want = peak > 0.89f ? 0.89f / peak : 1.0f;
        limGain = want < limGain ? want : want + (limGain - want) * rel;
        limGain = std::min(limGain, want);
        left[i] *= limGain; right[i] *= limGain;
    }
}

} // namespace bass
