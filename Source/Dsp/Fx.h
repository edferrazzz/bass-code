#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

namespace bass {

struct FxSettings {
    int automation = 0;       // Genres.h Automation
    float automationAmount = 0.8f;
    float reverb = 0.12f;     // base reverb send
    float delay = 0.0f;       // base dotted-1/8 delay
    float sidechain = 0.4f;   // fixed duck on every beat, only the amount changes
    float volume = 0.8f;
};

// AUTOMATION (filters / sends over the loop) -> SIDECHAIN -> DELAY -> REVERB -> limiter. Stereo, in place.
class FxChain {
public:
    void prepare(double sampleRate, int maxBlock);
    void reset();
    void process(float* left, float* right, int n, const FxSettings&, double bpm, double ppqStart, double loopBeats);

private:
    double sr = 48000.0;
    std::array<float, 4> lpL {}, lpR {};
    std::array<float, 2> hpL {}, hpR {};
    float cutoffSmoothed = 18000.0f, hpSmoothed = 20.0f;
    std::vector<float> dlL, dlR, thL, thR;
    int dlPos = 0;
    juce::Reverb reverb;
    std::vector<float> wetL, wetR;
    float limGain = 1.0f;
    float hpPrevIn = 0.0f, hpPrevOut = 0.0f;
};

} // namespace bass
