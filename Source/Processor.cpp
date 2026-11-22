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
            synth.noteOn(n.pitch + transpose, n.velocity, true);
            if (sounding >= 0 && sounding != n.pitch + transpose) synth.noteOff(sounding);
            sounding = n.pitch + transpose;
        }
    for (auto& n : p.notes) {
        const double off = (n.start + n.length) / 4.0 - 0.004;
        if (inside(off) && sounding == n.pitch + transpose) {
            bool slideNext = false;   // a slide starting right here keeps the voice alive
            for (auto& m : p.notes) if (m.slide && m.start == (n.start + n.length) % p.steps()) slideNext = true;
            if (!slideNext) { synth.noteOff(sounding); sounding = -1; }
        }
    }
    for (auto& n : p.notes)
        if (!n.slide && inside(n.start / 4.0)) {
            if (sounding >= 0 && sounding != n.pitch + transpose) synth.noteOff(sounding);
            synth.noteOn(n.pitch + transpose, n.velocity, false);
            sounding = n.pitch + transpose;
        }
}

void Engine::render(float* left, float* right, int n, double ppq, double bpm, const Pattern& p, bool playPattern,
                    const FxSettings& fxs) {
    std::fill(left, left + n, 0.0f);
    std::fill(right, right + n, 0.0f);
    bpm = bpm > 20.0 ? bpm : 120.0;
    synth.setTempo(bpm);
    const double perSample = bpm / 60.0 / sr;

    constexpr int chunk = 32;
    for (int i = 0; i < n; i += chunk) {
        const int len = std::min(chunk, n - i);
        const double t0 = ppq + i * perSample, t1 = t0 + len * perSample;
        synth.setSongPosition(t0);
        if (playPattern) schedule(p, t0, t1);
        synth.render(left + i, right + i, len);
    }
    fx.process(left, right, n, fxs, bpm, ppq, p.steps() / 4.0);
}

// ===========================================================================
// Parameters
// ===========================================================================
namespace id {
constexpr auto style = "style", arrange = "arrange", key = "key", scale = "scale", sound = "sound", bars = "bars",
               density = "density", bpm = "bpm", syncBpm = "syncBpm", sub = "sub", drive = "drive", movement = "movement",
               punch = "punch", automation = "automation", autoAmt = "autoAmt", rev = "rev", dly = "dly", sc = "sc",
               volume = "volume";
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    StringArray styleNames, keys { "PADRAO" }, scaleNames { "PADRAO" }, autos;
    for (auto& g : genres()) styleNames.add(g.name);
    for (int k = 0; k < 12; ++k) keys.add(noteName(k));
    for (auto& s : scales()) scaleNames.add(s.name);
    for (int a = 0; a < numAutomations; ++a) autos.add(String::fromUTF8(automationName(a)));
    auto choice = [&](const char* pid, const char* name, StringArray items, int def) {
        l.add(std::make_unique<AudioParameterChoice>(ParameterID { pid, 1 }, name, items, def));
    };
    auto flt = [&](const char* pid, const char* name, float def) {
        l.add(std::make_unique<AudioParameterFloat>(ParameterID { pid, 1 }, name, NormalisableRange<float>(0.0f, 1.0f), def));
    };
    const auto& g0 = genres()[0];
    choice(id::style, "Style", styleNames, 0);
    l.add(std::make_unique<AudioParameterInt>(ParameterID { id::arrange, 1 }, "Arranjo", 0, 5, 0));   // 0 = PADRAO
    choice(id::key, "Tom", keys, 0);
    choice(id::scale, "Escala", scaleNames, 0);
    l.add(std::make_unique<AudioParameterInt>(ParameterID { id::sound, 1 }, "Sound", 0, (int) sounds().size() - 1, 0));
    choice(id::bars, "Bars", { "2", "4", "8" }, 1);
    choice(id::density, "Density", { "LOW", "MID", "HIGH" }, 1);
    l.add(std::make_unique<AudioParameterInt>(ParameterID { id::bpm, 1 }, "BPM", 60, 200, g0.bpm));
    l.add(std::make_unique<AudioParameterBool>(ParameterID { id::syncBpm, 1 }, "Sync BPM", false));
    flt(id::sub, "Sub", 0.6f);
    flt(id::drive, "Drive", 0.45f);
    flt(id::movement, "Movimento", 0.55f);
    flt(id::punch, "Punch", 0.7f);
    choice(id::automation, "Automacao", autos, g0.automation);
    flt(id::autoAmt, "Automacao Intensidade", 0.8f);
    flt(id::rev, "Reverb", g0.reverb);
    flt(id::dly, "Delay", g0.delay);
    flt(id::sc, "Sidechain", g0.sidechain);
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
    liveRate = sampleRate > 0 ? sampleRate : 48000.0;
    internalPpq = 0.0;
}

static int intParam(const juce::AudioProcessorValueTreeState& s, const char* pid) {
    return (int) std::lround(s.getRawParameterValue(pid)->load());
}
static float fParam(const juce::AudioProcessorValueTreeState& s, const char* pid) {
    return s.getRawParameterValue(pid)->load();
}
static const Genre& genreOf(const juce::AudioProcessorValueTreeState& s) {
    return genres()[(size_t) juce::jlimit(0, (int) genres().size() - 1, intParam(s, id::style))];
}

int Processor::currentArrangement() const {
    const auto& g = genreOf(state);
    const int a = intParam(state, id::arrange);
    return juce::jlimit(0, (int) g.arrangements.size() - 1, a == 0 ? lastArrangement : a - 1);
}

GenSettings Processor::currentGenSettings() const {
    GenSettings g;
    const auto& arr = genreOf(state).arrangements[(size_t) currentArrangement()];
    g.style = arr.engine;
    g.groove = arr.groove;
    g.key = intParam(state, id::key) - 1;
    g.scale = intParam(state, id::scale) - 1;
    g.bars = std::array<int, 3> { 2, 4, 8 }[(size_t) juce::jlimit(0, 2, intParam(state, id::bars))];
    g.density = intParam(state, id::density);
    g.seed = seed;
    return g;
}

int Processor::currentPresetIndex() const {
    return juce::jlimit(0, (int) sounds().size() - 1, intParam(state, id::sound));
}

int Processor::currentBpm() const { return intParam(state, id::bpm); }

FxSettings Processor::currentFx() const {
    FxSettings f;
    f.automation = intParam(state, id::automation);
    f.automationAmount = fParam(state, id::autoAmt);
    f.reverb = fParam(state, id::rev);
    f.delay = fParam(state, id::dly);
    f.sidechain = fParam(state, id::sc);
    f.volume = fParam(state, id::volume);
    return f;
}

static int nearestShift(int fromPc, int toPitch) {
    return ((toPitch % 12 - fromPc) % 12 + 12 + 6) % 12 - 6;
}

int Processor::playInKey(int pitch) {
    const int t = nearestShift(pattern.key, pitch);
    transposeUi = t;
    setPreviewing(true);
    return t;
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
    if (!variation && intParam(state, id::arrange) == 0) {   // PADRAO: any arrangement of the style
        const int n = (int) genreOf(state).arrangements.size();
        int next = juce::Random::getSystemRandom().nextInt(n);
        if (n > 1 && next == lastArrangement) next = (next + 1 + juce::Random::getSystemRandom().nextInt(n - 1)) % n;
        lastArrangement = next;
    }
    transposeUi = 0;
    seed = seed * 1103515245u + 12345u + (uint32_t) juce::Time::getMillisecondCounter();
    pattern = variation ? bass::generate(currentGenSettings(), &pattern) : bass::generate(currentGenSettings());
    publishPattern();
}

void Processor::regenerateKeepSeed() {
    pattern = bass::generate(currentGenSettings());
    publishPattern();
}

void Processor::applyStyleDefaults() {
    const auto& g = genreOf(state);
    auto set = [&](const char* pid, float plain) {
        if (auto* p = state.getParameter(pid)) p->setValueNotifyingHost(p->convertTo0to1(plain));
    };
    lastArrangement = 0;
    transposeUi = 0;
    set(id::arrange, 0.0f);
    set(id::sound, (float) firstSoundOfGenre(intParam(state, id::style)));
    set(id::bpm, (float) g.bpm);
    set(id::automation, (float) g.automation);
    set(id::rev, g.reverb);
    set(id::dly, g.delay);
    set(id::sc, g.sidechain);
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

    // MIDI in: holding a note plays the loop in that note's key
    bool retrigger = false;
    for (const auto meta : midi) {
        const auto m = meta.getMessage();
        if (m.isNoteOn()) { midiHeld = m.getNoteNumber(); transposeUi = nearestShift(audioPattern.key, midiHeld); retrigger = true; }
        else if (m.isNoteOff() && m.getNoteNumber() == midiHeld) midiHeld = -1;
        else if (m.isAllNotesOff() || m.isAllSoundOff()) midiHeld = -1;
    }
    engine.setTranspose(transposeUi.load());

    const bool sync = fParam(state, id::syncBpm) > 0.5f;
    double bpm = currentBpm(), ppq = internalPpq;
    bool hostRunning = false;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            hostRunning = pos->getIsPlaying();
            if (sync) {
                if (auto b = pos->getBpm()) bpm = *b;
                if (hostRunning) if (auto q = pos->getPpqPosition()) ppq = *q;
            }
        }
    }
    hostPlaying = hostRunning;
    const bool useHostClock = sync && hostRunning;

    const bool midiPlay = midiHeld >= 0;
    const bool play = previewing.load() || midiPlay;
    if (play && (restartPreview.exchange(false) || retrigger)) {
        internalPpq = 0.0;
        if (!useHostClock) ppq = 0.0;
        engine.synthRef().allNotesOff();
        engine.reset();
        engine.setTranspose(transposeUi.load());
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
    if (!useHostClock) internalPpq = ppq + n * bpm / 60.0 / liveRate;
    else internalPpq = ppq;
}

// ===========================================================================
// Export
// ===========================================================================
juce::String Processor::exportName() const {
    juce::String s = juce::String("Bass Code - ") + genreOf(state).name + " " + noteName(pattern.key) + " "
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
