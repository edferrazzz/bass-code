#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

#include "Core/Music.h"
#include "Dsp/Fx.h"
#include "Dsp/Synth.h"

namespace bass {

// Synth + FX + pattern player. Used live (plugin) and offline (ARRASTAR WAV, tests).
class Engine {
public:
    void prepare(double sampleRate, int maxBlock);
    void reset();
    void setSound(int presetIndex) { synth.setPreset(presetIndex); }
    void setMacros(const Macros& m) { synth.setMacros(m); }
    // Plays `pattern` (when playPattern) from song position ppq, plus any live MIDI already sent to synth().
    void render(float* left, float* right, int n, double ppq, double bpm, const Pattern& pattern,
                bool playPattern, const FxSettings& fx);
    BassSynth& synthRef() { return synth; }

private:
    void schedule(const Pattern&, double from, double to);
    BassSynth synth;
    FxChain fx;
    std::vector<float> mono;
    int sounding = -1;
    double sr = 48000.0;
};

class Processor final : public juce::AudioProcessor {
public:
    Processor();
    ~Processor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Bass Code"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState state;

    // ---- message thread API (editor) ----
    const Pattern& getPattern() const { return pattern; }
    void setPattern(const Pattern& p);                  // manual edits
    void generate(bool variation);                      // GERAR / VARIAÇÃO
    void regenerateKeepSeed();                          // settings changed
    void applyStyleDefaults();                          // after STYLE change
    GenSettings currentGenSettings() const;
    int currentPresetIndex() const;
    int currentBpm() const;
    FxSettings currentFx() const;
    Macros currentMacros() const;

    void setPreviewing(bool on);
    bool isPreviewing() const { return previewing.load(); }
    double getPlayPositionSteps() const { return playSteps.load(); }   // -1 when stopped
    bool isHostPlaying() const { return hostPlaying.load(); }

    juce::File writeMidiFile() const;
    juce::File writeWavFile();
    juce::String exportName() const;

    // offline render of the current pattern (one clean loop, tails wrapped around)
    juce::AudioBuffer<float> renderLoop(double sampleRate) const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void publishPattern();

    Engine engine;
    Pattern pattern;            // message thread copy
    Pattern audioPattern;       // audio thread copy
    juce::SpinLock patternLock;
    Pattern pendingPattern;
    std::atomic<bool> patternDirty { false };
    uint32_t seed = 7;

    std::atomic<bool> previewing { false }, restartPreview { false }, hostPlaying { false };
    std::atomic<double> playSteps { -1.0 };
    double internalPpq = 0.0;
    bool wasPlayingPattern = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)
};

} // namespace bass
