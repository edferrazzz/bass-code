#include "Processor.h"
#include "Editor.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <cmath>

namespace bass {

// ===========================================================================
// Engine
// ===========================================================================
void Engine::prepare(double sampleRate, int maxBlock) {
    sr = sampleRate > 0 ? sampleRate : 48000.0;
    synth.prepare(sr);
    fx.prepare(sr, maxBlock);
    mono.assign((size_t) std::max(maxBlock, 64), 0.0f);
    sounding = -1;
}

void Engine::reset() {
    synth.reset();
    fx.reset();
    sounding = -1;
}

void Engine::schedule(const Pattern& p, double from, double to) {
    const double L = p.steps() / 4.0;
    if (L <= 0.0 || to <= from) return;
    const double a = std::fmod(std::max(0.0, from), L);
    const double b = a + (to - from);
    auto inside = [&](double t) { return (t >= a && t < b) || (t + L >= a && t + L < b); };

    // slides first (legato), then note-offs, then fresh notes
    for (auto& n : p.notes)
        if (n.slide && inside(n.start / 4.0)) {
            synth.noteOn(n.pitch, n.velocity, true);
            if (sounding >= 0 && sounding != n.pitch) synth.noteOff(sounding);
            sounding = n.pitch;
        }
    for (auto& n : p.notes) {
        const double off = (n.start + n.length) / 4.0 - 0.004;
        if (inside(off) && sounding == n.pitch) {
            bool slideNext = false;   // a slide starting right here keeps the voice alive
            for (auto& m : p.notes) if (m.slide && m.start == (n.start + n.length) % p.steps()) slideNext = true;
            if (!slideNext) { synth.noteOff(n.pitch); sounding = -1; }
        }
    }
    for (auto& n : p.notes)
        if (!n.slide && inside(n.start / 4.0)) {
            if (sounding >= 0 && sounding != n.pitch) synth.noteOff(sounding);
            synth.noteOn(n.pitch, n.velocity, false);
            sounding = n.pitch;
        }
}

void Engine::render(float* left, float* right, int n, double ppq, double bpm, const Pattern& p, bool playPattern,
                    const FxSettings& fxs) {
    if ((int) mono.size() < n) mono.resize((size_t) n);
    std::fill(mono.begin(), mono.begin() + n, 0.0f);
    bpm = bpm > 20.0 ? bpm : 120.0;
    synth.setTempo(bpm);
    const double perSample = bpm / 60.0 / sr;

    constexpr int chunk = 32;
    for (int i = 0; i < n; i += chunk) {
        const int len = std::min(chunk, n - i);
        const double t0 = ppq + i * perSample, t1 = t0 + len * perSample;
        synth.setSongPosition(t0);
        if (playPattern) schedule(p, t0, t1);
        synth.render(mono.data() + i, len);
    }
    fx.process(mono.data(), left, right, n, fxs, bpm, ppq, p.steps() / 4.0);
}

// ===========================================================================
// Parameters
// ===========================================================================
namespace id {
constexpr auto style = "style", key = "key", scale = "scale", groove = "groove", sound = "sound", bars = "bars",
               density = "density", bpm = "bpm", sub = "sub", drive = "drive", movement = "movement", punch = "punch",
               lpOn = "lpOn", lp = "lp", sweep = "sweep", revOn = "revOn", rev = "rev", dlyOn = "dlyOn", dly = "dly",
               scOn = "scOn", sc = "sc", volume = "volume";
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    StringArray styles, keys { "PADRAO" }, scaleNames { "PADRAO" };
    for (int s = 0; s < numStyles; ++s) styles.add(style(s).name);
    for (int k = 0; k < 12; ++k) keys.add(noteName(k));
    for (auto& s : scales()) scaleNames.add(s.name);
    auto choice = [&](const char* pid, const char* name, StringArray items, int def) {
        l.add(std::make_unique<AudioParameterChoice>(ParameterID { pid, 1 }, name, items, def));
    };
    auto flt = [&](const char* pid, const char* name, float def) {
        l.add(std::make_unique<AudioParameterFloat>(ParameterID { pid, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def));
    };
    auto flag = [&](const char* pid, const char* name, bool def) {
        l.add(std::make_unique<AudioParameterBool>(ParameterID { pid, 1 }, name, def));
    };
    choice(id::style, "Style", styles, 0);
    choice(id::key, "Tom", keys, 0);
    choice(id::scale, "Escala", scaleNames, 0);
    l.add(std::make_unique<AudioParameterInt>(ParameterID { id::groove, 1 }, "Groove", 0, 2, 0));
    l.add(std::make_unique<AudioParameterInt>(ParameterID { id::sound, 1 }, "Sound", 0, 5, 0));
    choice(id::bars, "Bars", { "2", "4", "8" }, 1);
    choice(id::density, "Density", { "LOW", "MID", "HIGH" }, 1);
    l.add(std::make_unique<AudioParameterInt>(ParameterID { id::bpm, 1 }, "BPM", 60, 200, 145));
    flt(id::sub, "Sub", 0.6f);
    flt(id::drive, "Drive", 0.45f);
    flt(id::movement, "Movimento", 0.55f);
    flt(id::punch, "Punch", 0.7f);
    flag(id::lpOn, "Lowpass On", true);
    flt(id::lp, "Lowpass", 0.62f);
    flag(id::sweep, "Sweep", false);
    flag(id::revOn, "Reverb On", true);
    flt(id::rev, "Reverb", 0.10f);
    flag(id::dlyOn, "Delay On", false);
    flt(id::dly, "Delay", 0.3f);
    flag(id::scOn, "Sidechain On", true);
    flt(id::sc, "Sidechain", 0.35f);
    flt(id::volume, "Volume", 0.8f);
    return l;
}

Processor::Processor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "BASSCODE", createLayout()) {
    seed = (uint32_t) juce::Random::getSystemRandom().nextInt(1 << 30);
    pattern = bass::generate(currentGenSettings());
    pendingPattern = pattern;
    patternDirty = true;
}

bool Processor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void Processor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    engine.prepare(sampleRate, samplesPerBlock);
    internalPpq = 0.0;
}

static int intParam(const juce::AudioProcessorValueTreeState& s, const char* pid) {
    return (int) std::lround(s.getRawParameterValue(pid)->load());
}
static float fParam(const juce::AudioProcessorValueTreeState& s, const char* pid) {
    return s.getRawParameterValue(pid)->load();
}

GenSettings Processor::currentGenSettings() const {
    GenSettings g;
    g.style = intParam(state, id::style);
    g.key = intParam(state, id::key) - 1;
    g.scale = intParam(state, id::scale) - 1;
    g.groove = intParam(state, id::groove);
    g.bars = std::array<int, 3> { 2, 4, 8 }[(size_t) juce::jlimit(0, 2, intParam(state, id::bars))];
    g.density = intParam(state, id::density);
    g.seed = seed;
    return g;
}

int Processor::currentPresetIndex() const {
    const auto& sd = style(intParam(state, id::style));
    return sd.sounds[(size_t) juce::jlimit(0, (int) sd.sounds.size() - 1, intParam(state, id::sound))];
}

int Processor::currentBpm() const { return intParam(state, id::bpm); }

FxSettings Processor::currentFx() const {
    FxSettings f;
    f.lowpassOn = fParam(state, id::lpOn) > 0.5f;
    f.lowpass = fParam(state, id::lp);
    f.sweep = fParam(state, id::sweep) > 0.5f;
    f.reverbOn = fParam(state, id::revOn) > 0.5f;
    f.reverb = fParam(state, id::rev);
    f.delayOn = fParam(state, id::dlyOn) > 0.5f;
    f.delay = fParam(state, id::dly);
    f.sidechainOn = fParam(state, id::scOn) > 0.5f;
    f.sidechain = fParam(state, id::sc);
    f.volume = fParam(state, id::volume);
    return f;
}

Macros Processor::currentMacros() const {
    return { fParam(state, id::sub), fParam(state, id::drive), fParam(state, id::movement), fParam(state, id::punch) };
}

void Processor::publishPattern() {
    const juce::SpinLock::ScopedLockType lock(patternLock);
    pendingPattern = pattern;
    patternDirty = true;
}

void Processor::setPattern(const Pattern& p) {
    pattern = p;
    std::sort(pattern.notes.begin(), pattern.notes.end(), [](auto& a, auto& b) { return a.start < b.start; });
    publishPattern();
}

void Processor::generate(bool variation) {
    seed = seed * 1103515245u + 12345u + (uint32_t) juce::Time::getMillisecondCounter();
    pattern = variation ? bass::generate(currentGenSettings(), &pattern) : bass::generate(currentGenSettings());
    publishPattern();
}

void Processor::regenerateKeepSeed() {
    pattern = bass::generate(currentGenSettings());
    publishPattern();
}

void Processor::applyStyleDefaults() {
    const auto& sd = style(intParam(state, id::style));
    auto set = [&](const char* pid, float plain) {
        if (auto* p = state.getParameter(pid)) p->setValueNotifyingHost(p->convertTo0to1(plain));
    };
    set(id::groove, 0.0f);
    set(id::sound, 0.0f);
    set(id::bpm, (float) sd.defaultBpm);
    set(id::lp, sd.lowpass);
    set(id::sweep, sd.sweep ? 1.0f : 0.0f);
    set(id::rev, sd.reverb);
    set(id::dlyOn, sd.delayOn ? 1.0f : 0.0f);
    set(id::sc, sd.sidechain);
}

void Processor::setPreviewing(bool on) {
    if (on) restartPreview = true;
    previewing = on;
}

void Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    if (patternDirty.load()) {
        const juce::SpinLock::ScopedTryLockType lock(patternLock);
        if (lock.isLocked()) { audioPattern = pendingPattern; patternDirty = false; }
    }

    engine.setSound(currentPresetIndex());
    engine.setMacros(currentMacros());

    for (const auto meta : midi) {
        const auto m = meta.getMessage();
        if (m.isNoteOn()) engine.synthRef().noteOn(m.getNoteNumber(), m.getFloatVelocity(), false);
        else if (m.isNoteOff()) engine.synthRef().noteOff(m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.synthRef().allNotesOff();
    }

    double bpm = currentBpm(), ppq = internalPpq;
    bool hostRunning = false;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            hostRunning = pos->getIsPlaying();
            if (hostRunning) {
                if (auto b = pos->getBpm()) bpm = *b;
                if (auto q = pos->getPpqPosition()) ppq = *q;
            }
        }
    }
    hostPlaying = hostRunning;

    const bool play = previewing.load();
    if (play && restartPreview.exchange(false)) {
        internalPpq = 0.0;
        if (!hostRunning) ppq = 0.0;
        engine.synthRef().allNotesOff();
    }
    if (!play && wasPlayingPattern) engine.synthRef().allNotesOff();
    wasPlayingPattern = play;

    const double L = audioPattern.steps() / 4.0;
    playSteps = play && L > 0 ? std::fmod(std::max(0.0, ppq), L) * 4.0 : -1.0;

    float* l = buffer.getWritePointer(0);
    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    if (r == nullptr) {
        juce::HeapBlock<float> tmp((size_t) n);
        engine.render(l, tmp.get(), n, ppq, bpm, audioPattern, play, currentFx());
    } else {
        engine.render(l, r, n, ppq, bpm, audioPattern, play, currentFx());
    }
    if (!hostRunning) internalPpq = ppq + n * bpm / 60.0 / getSampleRate();
    else internalPpq = ppq;
}

// ===========================================================================
// Export
// ===========================================================================
juce::String Processor::exportName() const {
    const auto g = currentGenSettings();
    juce::String s = juce::String("Bass Code - ") + style(g.style).name + " " + noteName(pattern.key) + " "
                     + juce::String(scales()[(size_t) pattern.scale].name).toLowerCase() + " " + juce::String(currentBpm()) + "bpm";
    return juce::File::createLegalFileName(s);
}

juce::AudioBuffer<float> Processor::renderLoop(double sampleRate) const {
    Engine e;
    e.prepare(sampleRate, 512);
    e.setSound(currentPresetIndex());
    e.setMacros(currentMacros());
    const double bpm = currentBpm();
    const double beats = pattern.steps() / 4.0;
    const int loopSamples = (int) std::lround(beats * 60.0 / bpm * sampleRate);
    juce::AudioBuffer<float> two(2, loopSamples * 2);
    const auto fx = currentFx();
    for (int i = 0; i < two.getNumSamples(); i += 512) {
        const int len = std::min(512, two.getNumSamples() - i);
        e.render(two.getWritePointer(0, i), two.getWritePointer(1, i), len, i * bpm / 60.0 / sampleRate, bpm, pattern, true, fx);
    }
    // second pass = a seamless loop (reverb/delay tails from the end already wrap into the start)
    juce::AudioBuffer<float> out(2, loopSamples);
    for (int c = 0; c < 2; ++c) out.copyFrom(c, 0, two, c, loopSamples, loopSamples);
    return out;
}

static juce::File exportDir() {
    auto d = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("Bass Code");
    d.createDirectory();
    return d;
}

juce::File Processor::writeMidiFile() const {
    juce::MidiMessageSequence seq;
    constexpr int tpq = 960;
    seq.addEvent(juce::MidiMessage::tempoMetaEvent((int) std::lround(60000000.0 / currentBpm())), 0);
    seq.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4, 4), 0);
    seq.addEvent(juce::MidiMessage::textMetaEvent(3, "Bass"), 0);
    for (auto& n : pattern.notes) {
        const double on = n.start * tpq / 4.0, off = (n.start + n.length) * tpq / 4.0 - (n.slide ? 0 : 1);
        seq.addEvent(juce::MidiMessage::noteOn(1, n.pitch, (juce::uint8) juce::jlimit(1, 127, (int) std::lround(n.velocity * 127.0f))), on);
        seq.addEvent(juce::MidiMessage::noteOff(1, n.pitch), off);
    }
    seq.addEvent(juce::MidiMessage::endOfTrack(), pattern.steps() * tpq / 4.0);
    seq.sort();
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote(tpq);
    mf.addTrack(seq);
    auto f = exportDir().getChildFile(exportName() + ".mid");
    f.deleteFile();
    if (auto os = f.createOutputStream()) mf.writeTo(*os);
    return f;
}

juce::File Processor::writeWavFile() {
    const double sr = getSampleRate() > 0 ? getSampleRate() : 48000.0;
    auto buf = renderLoop(sr);
    auto f = exportDir().getChildFile(exportName() + ".wav");
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::OutputStream>(f.createOutputStream().release())) {
        std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(os.get(), sr, 2, 24, {}, 0));
        if (w != nullptr) {
            os.release();
            w->writeFromAudioSampleBuffer(buf, 0, buf.getNumSamples());
        }
    }
    return f;
}

// ===========================================================================
// State
// ===========================================================================
void Processor::getStateInformation(juce::MemoryBlock& dest) {
    auto s = state.copyState();
    juce::ValueTree pt("PATTERN");
    juce::StringArray notes;
    for (auto& n : pattern.notes)
        notes.add(juce::String(n.start) + "," + juce::String(n.length) + "," + juce::String(n.pitch) + ","
                  + juce::String(n.velocity, 3) + "," + (n.slide ? "1" : "0"));
    juce::StringArray roots, minor;
    for (auto r : pattern.barRoots) roots.add(juce::String(r));
    for (auto m : pattern.barMinor) minor.add(m ? "1" : "0");
    pt.setProperty("bars", pattern.bars, nullptr);
    pt.setProperty("key", pattern.key, nullptr);
    pt.setProperty("scale", pattern.scale, nullptr);
    pt.setProperty("notes", notes.joinIntoString(";"), nullptr);
    pt.setProperty("roots", roots.joinIntoString(","), nullptr);
    pt.setProperty("minor", minor.joinIntoString(","), nullptr);
    pt.setProperty("seed", (int) (seed & 0x7fffffff), nullptr);
    s.removeChild(s.getChildWithName("PATTERN"), nullptr);
    s.addChild(pt, -1, nullptr);
    if (auto xml = s.createXml()) copyXmlToBinary(*xml, dest);
}

void Processor::setStateInformation(const void* data, int size) {
    auto xml = getXmlFromBinary(data, size);
    if (xml == nullptr) return;
    auto s = juce::ValueTree::fromXml(*xml);
    if (!s.isValid()) return;
    auto pt = s.getChildWithName("PATTERN");
    state.replaceState(s);
    if (pt.isValid()) {
        Pattern p;
        p.bars = (int) pt.getProperty("bars", 4);
        p.key = (int) pt.getProperty("key", 0);
        p.scale = (int) pt.getProperty("scale", 0);
        seed = (uint32_t) (int) pt.getProperty("seed", 7);
        for (auto& tok : juce::StringArray::fromTokens(pt.getProperty("notes").toString(), ";", "")) {
            auto f = juce::StringArray::fromTokens(tok, ",", "");
            if (f.size() < 5) continue;
            p.notes.push_back({ f[0].getIntValue(), f[1].getIntValue(), f[2].getIntValue(), f[3].getFloatValue(), f[4] == "1" });
        }
        for (auto& tok : juce::StringArray::fromTokens(pt.getProperty("roots").toString(), ",", "")) p.barRoots.push_back(tok.getIntValue());
        for (auto& tok : juce::StringArray::fromTokens(pt.getProperty("minor").toString(), ",", "")) p.barMinor.push_back(tok == "1");
        setPattern(p);
    }
}

juce::AudioProcessorEditor* Processor::createEditor() { return new Editor(*this); }

} // namespace bass

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new bass::Processor(); }
