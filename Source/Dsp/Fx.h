#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace bass {

struct FxSettings {
    bool lowpassOn = true;
    float lowpass = 0.62f;    // 0..1 -> 80 Hz .. 18 kHz
    bool sweep = false;       // lowpass opens across the loop
    bool reverbOn = true;
    float reverb = 0.15f;
    bool delayOn = false;
    float delay = 0.25f;      // 1/8 dotted
    bool sidechainOn = true;
    float sidechain = 0.5f;
    float volume = 0.8f;
};

// LOWPASS (+SWEEP) -> SIDECHAIN duck -> DELAY -> REVERB -> limiter. Stereo.
class FxChain {
public:
    void prepare(double sampleRate, int maxBlock);
    void reset();
    // mono in, stereo out. loopPos = position in the loop 0..1 (for SWEEP), ppq = song position in beats.
    void process(const float* mono, float* left, float* right, int n,
                 const FxSettings&, double bpm, double ppqStart, double loopBeats);

private:
    double sr = 48000.0;
    float lpS1 = 0, lpS2 = 0, lpS3 = 0, lpS4 = 0, cutoffSmoothed = 1000.0f;
    std::vector<float> dlL, dlR;
    int dlPos = 0;
    juce::Reverb reverb;
    std::vector<float> wetL, wetR;
    float limGain = 1.0f;
    float hpPrevIn = 0.0f, hpPrevOut = 0.0f;
};

} // namespace bass
