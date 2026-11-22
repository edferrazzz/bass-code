// Bass Code checks: every style x groove x density generates a sane line, every sound renders
// without clipping or silence. Optional: BASS_SNAPSHOT=<file.png>, BASS_DEMO_DIR=<dir>.
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Editor.h"
#include "Processor.h"

#include <cstdio>
#include <set>

using namespace bass;

#include "Audition.inc"

static int failures = 0, checks = 0;
static void expect(bool ok, const juce::String& what) {
    ++checks;
    if (!ok) { ++failures; std::printf("FAIL: %s\n", what.toRawUTF8()); }
}

static void setParam(Processor& p, const char* id, float plain) {
    auto* prm = p.state.getParameter(id);
    prm->setValueNotifyingHost(prm->convertTo0to1(plain));
}

int main() {
    juce::ScopedJuceInitialiser_GUI gui;

    // ---- generator: every arrangement of every genre
    for (int gi = 0; gi < (int) genres().size(); ++gi)
        for (auto& arr : genres()[(size_t) gi].arrangements)
            for (int d = 0; d < 3; ++d)
                for (int bars : { 2, 4, 8 })
                    for (uint32_t seed = 1; seed < 5; ++seed) {
                        GenSettings g { arr.engine, -1, -1, arr.groove, bars, d, seed };
                        auto p = generate(g);
                        const auto tag = juce::String(genres()[(size_t) gi].name) + " " + arr.name + " d" + juce::String(d) + " b" + juce::String(bars);
                        expect(!p.notes.empty(), tag + " has notes");
                        expect((int) p.barRoots.size() == bars, tag + " bar roots");
                        bool ok = true;
                        for (auto& n : p.notes)
                            ok = ok && n.start >= 0 && n.length >= 1 && n.start + n.length <= p.steps() && n.pitch >= 24 && n.pitch <= 60;
                        expect(ok, tag + " notes in range");
                        auto v = generate(g, &p);
                        expect(v.barRoots == p.barRoots && v.key == p.key, tag + " variation keeps chords");
                    }

    // ---- audio: every sound through the full chain, every automation
    for (int snd = 0; snd < (int) sounds().size(); ++snd) {
        Processor proc;
        proc.prepareToPlay(48000.0, 512);
        setParam(proc, "style", (float) sounds()[(size_t) snd].genre);
        proc.applyStyleDefaults();
        setParam(proc, "sound", (float) snd);
        setParam(proc, "dly", 0.5f);
        for (float macro : { 0.0f, 1.0f }) {
            for (auto* id : { "sub", "drive", "movement", "punch" }) setParam(proc, id, macro);
            setParam(proc, "automation", (float) (snd % numAutomations));
            proc.generate(false);
            auto buf = proc.renderLoop(48000.0);
            const float peak = std::max(buf.getMagnitude(0, 0, buf.getNumSamples()), buf.getMagnitude(1, 0, buf.getNumSamples()));
            const float rms = buf.getRMSLevel(0, 0, buf.getNumSamples());
            const auto tag = juce::String(sounds()[(size_t) snd].preset.name) + " macros " + juce::String(macro);
            expect(peak < 0.95f, tag + " peak " + juce::String(peak));
            expect(rms > 0.015f, tag + " not silent (rms " + juce::String(rms) + ")");
            bool finite = true;
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < buf.getNumSamples(); ++i) finite = finite && std::isfinite(buf.getSample(c, i));
            expect(finite, tag + " finite");
        }
    }

    // ---- PADRAO picks different arrangements
    {
        Processor proc;
        std::set<int> seen;
        for (int i = 0; i < 30; ++i) { proc.generate(false); seen.insert(proc.currentArrangement()); }
        expect(seen.size() >= 3, "PADRAO varies the arrangement");
    }

    // ---- live MIDI: a held note plays the loop transposed
    {
        Processor proc;
        proc.prepareToPlay(48000.0, 512);
        juce::AudioBuffer<float> b(2, 512);
        juce::MidiBuffer m;
        m.addEvent(juce::MidiMessage::noteOn(1, 36 + ((proc.getPattern().key + 5) % 12), 0.9f), 0);
        float peak = 0.0f;
        for (int i = 0; i < 80; ++i) { proc.processBlock(b, m); m.clear(); peak = std::max(peak, b.getMagnitude(0, 0, 512)); }
        expect(peak > 0.05f, "MIDI note plays the loop (peak " + juce::String(peak) + ", notes " + juce::String((int) proc.getPattern().notes.size()) + ")");
        expect(proc.getTranspose() == 5, "MIDI note transposes (" + juce::String(proc.getTranspose()) + ")");
        expect(proc.playInKey(proc.getPattern().key + 24) == 0, "piano key of the root = no shift");
    }

    // ---- MIDI export + state round trip
    {
        Processor proc;
        auto f = proc.writeMidiFile();
        expect(f.existsAsFile() && f.getSize() > 40, "MIDI file written");
        juce::MemoryBlock mb;
        proc.getStateInformation(mb);
        Processor other;
        other.setStateInformation(mb.getData(), (int) mb.getSize());
        expect(other.getPattern().notes.size() == proc.getPattern().notes.size(), "state keeps the pattern");
    }

    // ---- audition/demo: each sound on its genre's first arrangement, with drums, and its default automation
    juce::File aud(juce::SystemStats::getEnvironmentVariable("BASS_AUDITION_DIR", {}));
    if (aud != juce::File()) {
        aud.createDirectory();
        int lastGenre = -1, num = 0;
        for (int si = 0; si < (int) sounds().size(); ++si) {
            const auto& snd = sounds()[(size_t) si];
            const auto& gen = genres()[(size_t) snd.genre];
            if (lastGenre != snd.genre) { lastGenre = snd.genre; num = 0; }
            ++num;
            Engine e; e.prepare(48000.0, 512);
            e.synthRef().setPresetDirect(snd.preset);
            e.setMacros({});
            const auto& arr = gen.arrangements[(size_t) ((num - 1) % (int) gen.arrangements.size())];
            GenSettings g { arr.engine, 9, 0, arr.groove, 4, 1, 3 };
            auto pat = generate(g);
            FxSettings fx; fx.automation = gen.automation; fx.reverb = gen.reverb; fx.delay = gen.delay; fx.sidechain = gen.sidechain;
            const double bpm = gen.bpm;
            const double beats = pat.steps() / 4.0 * 2.0;
            const int n = (int) (beats * 60.0 / bpm * 48000.0);
            juce::AudioBuffer<float> buf(2, n);
            for (int i = 0; i < n; i += 512) {
                const int len = std::min(512, n - i);
                e.render(buf.getWritePointer(0, i), buf.getWritePointer(1, i), len, i * bpm / 60.0 / 48000.0, bpm, pat, true, fx);
            }
            buf.applyGain(0.8f);
            addDrums(buf, drumsForGenre(snd.genre), bpm, 48000.0);
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < n; ++i) buf.setSample(ch, i, std::tanh(buf.getSample(ch, i) * 1.1f) * 0.9f);
            auto dir = aud.getChildFile(juce::String(gen.name).replace(" ", "-").toLowerCase());
            dir.createDirectory();
            auto f = dir.getChildFile(juce::String(num).paddedLeft('0', 2) + " " + snd.preset.name + ".wav");
            f.deleteFile();
            juce::WavAudioFormat wav;
            auto os = f.createOutputStream();
            std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(os.get(), 48000.0, 2, 16, {}, 0));
            if (w) { os.release(); w->writeFromAudioSampleBuffer(buf, 0, n); }
        }
    }

    // ---- editor snapshot
    auto snap = juce::SystemStats::getEnvironmentVariable("BASS_SNAPSHOT", {});
    if (snap.isNotEmpty()) {
        Processor proc;
        std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
        ed->setSize(Editor::designW, Editor::designH);
        auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 2.0f);
        juce::File out(snap);
        out.deleteFile();
        juce::PNGImageFormat png;
        if (auto os = out.createOutputStream()) png.writeImageToStream(img, *os);
    }

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
