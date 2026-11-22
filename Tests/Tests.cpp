// Bass Code checks: every style x groove x density generates a sane line, every sound renders
// without clipping or silence. Optional: BASS_SNAPSHOT=<file.png>, BASS_DEMO_DIR=<dir>.
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Editor.h"
#include "Processor.h"

#include <cstdio>

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

    // ---- generator
    for (int s = 0; s < numStyles; ++s)
        for (int gr = 0; gr < 3; ++gr)
            for (int d = 0; d < 3; ++d)
                for (int bars : { 2, 4, 8 })
                    for (uint32_t seed = 1; seed < 6; ++seed) {
                        GenSettings g { s, -1, -1, gr, bars, d, seed };
                        auto p = generate(g);
                        const auto tag = juce::String(style(s).name) + " g" + juce::String(gr) + " d" + juce::String(d) + " b" + juce::String(bars);
                        expect(!p.notes.empty(), tag + " has notes");
                        expect((int) p.barRoots.size() == bars, tag + " bar roots");
                        bool ok = true;
                        for (auto& n : p.notes)
                            ok = ok && n.start >= 0 && n.length >= 1 && n.start + n.length <= p.steps() && n.pitch >= 24 && n.pitch <= 60;
                        expect(ok, tag + " notes in range");
                        auto v = generate(g, &p);
                        expect(v.barRoots == p.barRoots && v.key == p.key, tag + " variation keeps chords");
                    }

    // ---- audio: every sound of every style through the full chain
    juce::File demoDir(juce::SystemStats::getEnvironmentVariable("BASS_DEMO_DIR", {}));
    for (int s = 0; s < numStyles; ++s) {
        for (int snd = 0; snd < 6; ++snd) {
            Processor proc;
            proc.prepareToPlay(48000.0, 512);
            setParam(proc, "style", (float) s);
            proc.applyStyleDefaults();
            setParam(proc, "sound", (float) snd);
            for (float macro : { 0.0f, 0.5f, 1.0f }) {
                for (auto* id : { "sub", "drive", "movement", "punch" }) setParam(proc, id, macro);
                setParam(proc, "dlyOn", 1.0f);
                proc.generate(false);
                auto buf = proc.renderLoop(48000.0);
                const float peak = std::max(buf.getMagnitude(0, 0, buf.getNumSamples()), buf.getMagnitude(1, 0, buf.getNumSamples()));
                const float rms = buf.getRMSLevel(0, 0, buf.getNumSamples());
                const auto tag = juce::String(style(s).name) + " / " + presets()[(size_t) style(s).sounds[(size_t) snd]].name + " macros " + juce::String(macro);
                expect(peak < 0.95f, tag + " peak " + juce::String(peak));
                expect(rms > 0.02f, tag + " not silent (rms " + juce::String(rms) + ")");
                bool finite = true;
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < buf.getNumSamples(); ++i) finite = finite && std::isfinite(buf.getSample(c, i));
                expect(finite, tag + " finite");
            }
            if (demoDir != juce::File() && snd == 0) {
                for (auto* id : { "sub", "drive", "movement", "punch" }) setParam(proc, id, 0.6f);
                setParam(proc, "dlyOn", style(s).delayOn ? 1.0f : 0.0f);
                setParam(proc, "bars", 2.0f);   // 8 bars
                proc.generate(false);
                demoDir.createDirectory();
                auto buf = proc.renderLoop(48000.0);
                auto f = demoDir.getChildFile(juce::String(style(s).name).replace(" ", "-").toLowerCase() + ".wav");
                f.deleteFile();
                juce::WavAudioFormat wav;
                auto os = f.createOutputStream();
                std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(os.get(), 48000.0, 2, 24, {}, 0));
                if (w) { os.release(); w->writeFromAudioSampleBuffer(buf, 0, buf.getNumSamples()); }
            }
        }
    }

    // ---- live MIDI plays the synth
    {
        Processor proc;
        proc.prepareToPlay(48000.0, 512);
        juce::AudioBuffer<float> b(2, 512);
        juce::MidiBuffer m;
        m.addEvent(juce::MidiMessage::noteOn(1, 36, 0.9f), 0);
        float peak = 0.0f;
        for (int i = 0; i < 40; ++i) { proc.processBlock(b, m); m.clear(); peak = std::max(peak, b.getMagnitude(0, 0, 512)); }
        expect(peak > 0.05f, "MIDI note plays");
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

    // ---- audition: one MP3-ready WAV per candidate sound
    juce::File aud(juce::SystemStats::getEnvironmentVariable("BASS_AUDITION_DIR", {}));
    if (aud != juce::File()) {
        aud.createDirectory();
        juce::String lastGenre; int num = 0;
        for (auto& c : candidates) {
            if (lastGenre != c.genre) { lastGenre = c.genre; num = 0; }
            ++num;
            Engine e; e.prepare(48000.0, 512);
            e.synthRef().setPresetDirect(c.p);
            e.setMacros({});
            GenSettings g { c.genStyle, 9, 0, c.groove, 4, 1, 3 };   // A minor, same line for every sound of a genre
            auto pat = generate(g);
            FxSettings fx; fx.lowpass = 0.9f; fx.reverb = 0.08f; fx.sidechain = 0.3f;
            const double beats = pat.steps() / 4.0 * 2.0;   // the 4-bar line twice
            const int n = (int) (beats * 60.0 / c.bpm * 48000.0);
            juce::AudioBuffer<float> buf(2, n);
            for (int i = 0; i < n; i += 512) {
                const int len = std::min(512, n - i);
                e.render(buf.getWritePointer(0, i), buf.getWritePointer(1, i), len, i * c.bpm / 60.0 / 48000.0, c.bpm, pat, true, fx);
            }
            buf.applyGain(0.8f);
            addDrums(buf, c.drums, c.bpm, 48000.0);
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < n; ++i) buf.setSample(ch, i, std::tanh(buf.getSample(ch, i) * 1.1f) * 0.9f);
            auto dir = aud.getChildFile(juce::String(c.genre).replace(" ", "-").toLowerCase());
            dir.createDirectory();
            auto f = dir.getChildFile(juce::String(num).paddedLeft('0', 2) + " " + c.p.name + ".wav");
            f.deleteFile();
            juce::WavAudioFormat wav;
            auto os = f.createOutputStream();
            std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(os.get(), 48000.0, 2, 16, {}, 0));
            if (w) { os.release(); w->writeFromAudioSampleBuffer(buf, 0, n); }
            expect(c.p.tableA >= 0 && buf.getRMSLevel(0, 0, n) > 0.02f, juce::String("audition ") + c.p.name);
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
