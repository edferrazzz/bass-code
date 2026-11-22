#include "Genres.h"

namespace bass {

const std::vector<Genre>& genres() {
    static const std::vector<Genre> g {
        { "PSY", "PSY", 145, { { "ROLLING", psy, 0 }, { "GALLOP", psy, 1 }, { "OCTAVE ROLL", psy, 2 }, { "OFF ROLL", psy, 15 }, { "TECH ROLL", psy, 10 } }, 0.30f, 0.08f, 0.0f, autoHpEnd },
        { "HOUSE", "HOUSE", 124, { { "OFFBEAT", house, 0 }, { "CLASSIC", house, 1 }, { "WALKING", house, 2 }, { "PUMP 8THS", house, 12 }, { "SYNCOPE", house, 16 } }, 0.45f, 0.12f, 0.0f, autoLpEnd },
        { "DEEP HOUSE", "DEEP HOUSE", 120, { { "LONG DEEP", house, 13 }, { "DEEP OFFBEAT", house, 0 }, { "WARM WALK", house, 2 }, { "SOUL", afroHouse, 2 }, { "SYNCOPE", house, 16 } }, 0.40f, 0.18f, 0.0f, autoReverbBuild },
        { "MINIMAL", "MINIMAL", 126, { { "SKIPPY", electronic, 11 }, { "STAB", electronic, 1 }, { "CLICK 16", electronic, 2 }, { "LOG", afroHouse, 1 }, { "PUMP 8THS", electronic, 12 } }, 0.35f, 0.08f, 0.15f, autoDelayThrow },
        { "TECHNO", "TECHNO", 130, { { "DRIVE 8THS", electronic, 0 }, { "ROLLING 16", electronic, 10 }, { "ACID", electronic, 2 }, { "STAB", electronic, 1 }, { "OFF ROLL", electronic, 15 } }, 0.50f, 0.10f, 0.0f, autoHpReverbBuild },
        { "AFRO HOUSE", "AFRO HOUSE", 122, { { "SYNCOPATED", afroHouse, 0 }, { "LOG DRUM", afroHouse, 1 }, { "DEEP", afroHouse, 2 }, { "AMAPIANO", afroHouse, 14 }, { "SKIPPY", afroHouse, 11 } }, 0.40f, 0.15f, 0.15f, autoReverbBuild },
        { "BRAZILIAN BASS", "BRAZILIAN BASS", 125, { { "BOUNCE", brazilianBass, 0 }, { "WOBBLE", brazilianBass, 1 }, { "DROP", brazilianBass, 2 }, { "PUMP 8THS", brazilianBass, 12 }, { "SYNCOPE", brazilianBass, 16 } }, 0.65f, 0.10f, 0.0f, autoLpEnd },
        { "ELETRONICA", "ELETR\xc3\x94NICA", 128, { { "DRIVE", electronic, 0 }, { "STAB", electronic, 1 }, { "SLIDE", electronic, 2 }, { "PUMP 8THS", electronic, 12 }, { "LONG", electronic, 13 } }, 0.50f, 0.14f, 0.0f, autoLpOpen },
    };
    return g;
}

namespace {
struct B {
    Preset p {};
    explicit B(const char* n) { p.name = n; p.wave = 0; p.unison = 1; p.detune = 0; p.sub = 0.5f; p.cutoff = 400; p.resonance = 0.2f;
        p.envAmount = 2.0f; p.decay = 0.12f; p.attackMs = 1; p.ampDecay = 0.2f; p.sustain = 0.0f; p.releaseMs = 40; p.drive = 0.25f;
        p.glideMs = 0; p.wobble = 0; p.fm = 0; p.pitchEnv = 0; p.pitchDecay = 0; p.keytrack = 0.5f; }
    B& osc(const char* a, const char* b = nullptr, float mix = 0, float semi = 0, float det = 0) {
        p.tableA = waveIndex(a); p.tableB = b ? waveIndex(b) : -1; p.mixB = mix; p.semiB = semi; p.detuneB = det; return *this; }
    B& uni(int n, float cents, float width) { p.unison = n; p.detune = cents; p.width = width; return *this; }
    B& sub(float s) { p.sub = s; return *this; }
    B& filt(float cut, float res, float env, float dec, float key = 0.5f) { p.cutoff = cut; p.resonance = res; p.envAmount = env; p.decay = dec; p.keytrack = key; return *this; }
    B& amp(float att, float dec, float sus, float rel) { p.attackMs = att; p.ampDecay = dec; p.sustain = sus; p.releaseMs = rel; return *this; }
    B& dirt(float drive, float fold = 0, float crush = 0) { p.drive = drive; p.fold = fold; p.crush = crush; return *this; }
    B& fm(float f) { p.fm = f; return *this; }
    B& pitch(float semis, float dec) { p.pitchEnv = semis; p.pitchDecay = dec; return *this; }
    B& lfo(int steps, float toPitch = 0) { p.wobble = steps; p.lfoToPitch = toPitch; return *this; }
    B& glide(float ms) { p.glideMs = ms; return *this; }
    B& noise(float n) { p.noise = n; return *this; }
    operator Preset() const { return p; }
};

} // namespace

const std::vector<Sound>& sounds() {
    static const std::vector<Sound> s {
        { 0, B("FULL ON").osc("SAW").filt(180, 0.3f, 4.2f, 0.075f).amp(0.5f, 0.11f, 0, 18).dirt(0.35f).sub(0.55f) },
        { 0, B("GOA RESO").osc("SAW BRIGHT").filt(220, 0.62f, 3.6f, 0.09f).amp(0.5f, 0.12f, 0, 20).dirt(0.3f).sub(0.5f) },
        { 0, B("DARK FOREST").osc("SQUARE", "FM 3", 0.5f, 12).filt(150, 0.5f, 3.8f, 0.06f).amp(0.5f, 0.1f, 0, 16).dirt(0.5f, 0.3f).fm(0.6f).sub(0.55f) },
        { 0, B("PROG ROUND").osc("TRI", "SAW SOFT", 0.35f).filt(300, 0.15f, 2.6f, 0.12f).amp(1, 0.16f, 0, 30).dirt(0.2f).sub(0.65f) },
        { 0, B("KNOCK").osc("SINE HARM 2").filt(500, 0.1f, 2.0f, 0.05f).amp(0.3f, 0.09f, 0, 14).dirt(0.3f).pitch(5, 0.012f).sub(0.7f) },
        { 0, B("HI TECH ZAP").osc("FM 2").filt(260, 0.4f, 4.6f, 0.05f).amp(0.3f, 0.085f, 0, 12).dirt(0.45f, 0.4f).fm(0.8f).pitch(12, 0.008f).sub(0.5f) },
        { 0, B("TWIN SAW").osc("SAW", "SAW", 0.6f, 0, 14).uni(2, 10, 0.35f).filt(200, 0.35f, 4.0f, 0.08f).amp(0.5f, 0.11f, 0, 18).dirt(0.4f).sub(0.5f) },
        { 0, B("RUBBER ZEN").osc("D BASS 1").filt(350, 0.25f, 2.2f, 0.07f).amp(0.5f, 0.12f, 0, 20).dirt(0.25f).pitch(3, 0.02f).sub(0.65f) },
        { 1, B("M ORGAN").osc("ORGAN 2", "SINE", 0.5f, -12).filt(1400, 0.05f, 1.2f, 0.2f).amp(1, 0.35f, 0.35f, 60).dirt(0.15f).sub(0.45f) },
        { 1, B("PIANO BASS").osc("E BASS 2", "SINE HARM", 0.4f).filt(900, 0.1f, 2.0f, 0.18f).amp(1, 0.45f, 0.15f, 80).dirt(0.15f).sub(0.5f) },
        { 1, B("FUNK PLUCK").osc("SAW").filt(300, 0.45f, 3.4f, 0.13f).amp(1, 0.25f, 0.15f, 40).dirt(0.3f).sub(0.5f) },
        { 1, B("TECH RUBBER").osc("SINE HARM").filt(700, 0.2f, 1.6f, 0.08f).amp(0.5f, 0.18f, 0.1f, 30).dirt(0.35f, 0.25f).pitch(4, 0.02f).sub(0.65f) },
        { 1, B("CHICAGO").osc("SQUARE SOFT", "TRI", 0.5f, 12).filt(600, 0.25f, 1.8f, 0.15f).amp(1, 0.3f, 0.3f, 50).dirt(0.2f).sub(0.5f) },
        { 1, B("DISCO FILTER").osc("SAW", "SQUARE", 0.4f, 0, 8).uni(2, 6, 0.3f).filt(280, 0.6f, 3.0f, 0.2f).amp(1, 0.3f, 0.3f, 50).dirt(0.25f).sub(0.45f) },
        { 1, B("BOUNCE FM").osc("FM 1").filt(1000, 0.15f, 1.6f, 0.1f).amp(0.5f, 0.2f, 0.2f, 40).dirt(0.3f).fm(0.4f).pitch(2, 0.015f).sub(0.55f) },
        { 1, B("GARAGE 2STEP").osc("PULSE", "SAW SOFT", 0.4f).filt(450, 0.3f, 2.4f, 0.12f).amp(1, 0.22f, 0.2f, 45).dirt(0.35f).sub(0.6f) },
        { 2, B("WARM SUB").osc("TRI SOFT").filt(500, 0.05f, 0.8f, 0.3f).amp(4, 0.0f, 0.9f, 80).dirt(0.1f).sub(0.7f) },
        { 2, B("VELVET REESE").osc("SAW SOFT", "SAW SOFT", 0.7f, 0, 18).uni(3, 12, 0.6f).filt(420, 0.15f, 0.8f, 0.3f).amp(6, 0.0f, 0.85f, 100).dirt(0.2f).sub(0.55f) },
        { 2, B("SOFT ORGAN").osc("ORGAN 1").filt(800, 0.05f, 0.8f, 0.3f).amp(3, 0.6f, 0.5f, 90).dirt(0.1f).sub(0.55f) },
        { 2, B("RHODES BASS").osc("E BASS 1", "SINE", 0.5f, 12).filt(700, 0.05f, 1.2f, 0.25f).amp(2, 0.7f, 0.2f, 120).dirt(0.12f).sub(0.55f) },
        { 2, B("DUSTY PLUCK").osc("SQUARE SOFT").filt(380, 0.25f, 2.0f, 0.2f).amp(2, 0.4f, 0.1f, 70).dirt(0.25f, 0, 0.35f).sub(0.55f) },
        { 2, B("MOOG ROUND").osc("SAW").filt(220, 0.35f, 1.8f, 0.25f, 0.8f).amp(3, 0.35f, 0.45f, 70).dirt(0.2f).sub(0.55f) },
        { 2, B("NIGHT FM").osc("FM 1", "TRI SOFT", 0.5f).filt(600, 0.1f, 1.0f, 0.3f).amp(4, 0.0f, 0.8f, 100).dirt(0.15f).fm(0.25f).sub(0.6f) },
        { 2, B("WOOD DEEP").osc("D BASS 2").filt(650, 0.1f, 1.2f, 0.2f).amp(2, 0.45f, 0.2f, 90).dirt(0.12f).sub(0.6f) },
        { 3, B("CLICK SUB").osc("SINE").filt(900, 0.05f, 1.0f, 0.03f).amp(0.3f, 0.1f, 0, 22).dirt(0.15f).pitch(4, 0.008f).sub(0.75f) },
        { 3, B("RUBBER BAND").osc("FM 1").filt(700, 0.25f, 2.0f, 0.06f).amp(0.3f, 0.12f, 0, 28).dirt(0.25f).fm(0.6f).pitch(3, 0.025f).sub(0.6f) },
        { 3, B("BLIP").osc("PULSE").filt(500, 0.6f, 3.0f, 0.04f).amp(0.3f, 0.08f, 0, 18).dirt(0.2f).sub(0.5f) },
        { 3, B("WOODBLOCK").osc("SYM 1").filt(1400, 0.1f, 1.0f, 0.05f).amp(0.3f, 0.09f, 0, 20).dirt(0.2f).pitch(10, 0.01f).sub(0.55f) },
        { 3, B("MICRO FM").osc("FM 4").filt(1100, 0.15f, 1.2f, 0.04f).amp(0.3f, 0.1f, 0, 20).dirt(0.2f).fm(0.5f).sub(0.6f) },
        { 3, B("BUBBLE").osc("SINE HARM").filt(800, 0.3f, 1.4f, 0.06f).amp(0.5f, 0.14f, 0, 30).dirt(0.15f).lfo(1, 0.6f).sub(0.65f) },
        { 3, B("CRUSH TICK").osc("BIT 1").filt(1200, 0.1f, 1.0f, 0.05f).amp(0.3f, 0.1f, 0, 20).dirt(0.3f, 0, 0.5f).sub(0.6f) },
        { 4, B("RUMBLE").osc("TRI", "DIST 1", 0.3f).filt(240, 0.3f, 1.8f, 0.3f).amp(2, 0.5f, 0.2f, 120).dirt(0.6f).sub(0.65f) },
        { 4, B("DRIVE SAW").osc("SAW", "SAW", 0.5f, 0, 10).uni(2, 8, 0.4f).filt(380, 0.3f, 2.4f, 0.15f).amp(1, 0.22f, 0.3f, 40).dirt(0.55f).sub(0.5f) },
        { 4, B("ACID 303").osc("SAW").filt(260, 0.82f, 3.8f, 0.16f, 0.3f).amp(1, 0.22f, 0.3f, 40).dirt(0.45f).glide(70).sub(0.4f) },
        { 4, B("INDUSTRIAL").osc("DIST 3").uni(2, 14, 0.4f).filt(500, 0.35f, 2.2f, 0.1f).amp(0.5f, 0.15f, 0.2f, 30).dirt(0.75f, 0.5f).sub(0.5f) },
        { 4, B("DARK STAB").osc("SAW", "SQUARE", 0.5f, 7).uni(3, 16, 0.5f).filt(320, 0.5f, 3.0f, 0.08f).amp(0.5f, 0.12f, 0, 40).dirt(0.55f).sub(0.45f) },
        { 4, B("HYPNO FM").osc("FM 3").filt(700, 0.3f, 1.6f, 0.12f).amp(1, 0.18f, 0.2f, 30).dirt(0.4f).fm(0.7f).lfo(16).sub(0.6f) },
        { 4, B("ACID SQUARE").osc("SQUARE").filt(250, 0.8f, 3.4f, 0.14f, 0.3f).amp(1, 0.2f, 0.3f, 40).dirt(0.5f).glide(60).sub(0.4f) },
        { 4, B("RAW PULSE").osc("RAW 2").filt(420, 0.25f, 2.0f, 0.1f).amp(0.5f, 0.16f, 0.2f, 30).dirt(0.6f, 0.2f).sub(0.55f) },
        { 5, B("LOG DRUM").osc("SINE HARM").filt(1500, 0.05f, 0.8f, 0.1f).amp(0.5f, 0.24f, 0, 60).dirt(0.35f).pitch(7, 0.045f).sub(0.55f) },
        { 5, B("AMAPIANO LOG").osc("SINE", "TRI", 0.4f, 12).filt(2000, 0.05f, 0.6f, 0.2f).amp(0.5f, 0.45f, 0, 90).dirt(0.5f).pitch(12, 0.07f).glide(50).sub(0.65f) },
        { 5, B("TALKING").osc("VOICE 2").filt(1000, 0.25f, 1.4f, 0.14f).amp(0.5f, 0.25f, 0, 50).dirt(0.25f).pitch(4, 0.06f).sub(0.5f) },
        { 5, B("DEEP AFRO").osc("TRI SOFT", "E BASS 1", 0.4f).filt(800, 0.1f, 1.2f, 0.2f).amp(3, 0.45f, 0.5f, 70).dirt(0.15f).sub(0.6f) },
        { 5, B("MARIMBA SUB").osc("PLUCK").filt(1600, 0.1f, 1.0f, 0.1f).amp(0.5f, 0.3f, 0, 60).dirt(0.15f).pitch(12, 0.02f).sub(0.55f) },
        { 5, B("DRUM BASS").osc("FM 2").filt(900, 0.15f, 1.2f, 0.08f).amp(0.5f, 0.2f, 0, 50).dirt(0.35f).fm(0.5f).pitch(5, 0.03f).sub(0.6f) },
        { 5, B("WOOD").osc("SYM 2").filt(700, 0.35f, 2.2f, 0.08f).amp(0.5f, 0.16f, 0, 40).dirt(0.2f).pitch(3, 0.03f).sub(0.5f) },
        { 5, B("TRIBAL WARM").osc("SAW SOFT").filt(380, 0.15f, 1.4f, 0.25f).amp(3, 0.5f, 0.5f, 80).dirt(0.2f).sub(0.6f) },
        { 6, B("BR BOUNCE").osc("FM 3", "SQUARE", 0.3f).filt(700, 0.3f, 2.0f, 0.12f).amp(1, 0.0f, 0.85f, 40).dirt(0.55f, 0.3f).fm(0.7f).lfo(2).sub(0.55f) },
        { 6, B("WOBBLE 8TH").osc("VOICE 1", "SAW", 0.4f).filt(420, 0.45f, 1.0f, 0.2f).amp(2, 0.0f, 0.9f, 50).dirt(0.6f, 0.2f).fm(0.6f).lfo(2).glide(40).sub(0.55f) },
        { 6, B("SLAP").osc("E BASS 3").uni(2, 8, 0.3f).filt(900, 0.3f, 2.4f, 0.08f).amp(0.5f, 0.2f, 0.3f, 40).dirt(0.5f).pitch(5, 0.03f).sub(0.5f) },
        { 6, B("GROWL").osc("VOICE 3", "FM 4", 0.4f, 0, 9).uni(2, 9, 0.4f).filt(600, 0.45f, 1.6f, 0.15f).amp(2, 0.0f, 0.9f, 50).dirt(0.7f, 0.5f).fm(0.9f).lfo(4).sub(0.5f) },
        { 6, B("DIRTY SAW").osc("DIST 2").uni(3, 16, 0.5f).filt(650, 0.35f, 2.0f, 0.18f).amp(2, 0.0f, 0.85f, 50).dirt(0.65f).lfo(4).sub(0.55f) },
        { 6, B("METAL BOUNCE").osc("FM 4").uni(2, 12, 0.4f).filt(900, 0.35f, 2.0f, 0.1f).amp(1, 0.0f, 0.8f, 40).dirt(0.7f, 0.6f).fm(1.2f).pitch(3, 0.02f).sub(0.5f) },
        { 6, B("LASER").osc("SAW BRIGHT").uni(2, 10, 0.3f).filt(800, 0.55f, 3.0f, 0.06f).amp(0.5f, 0.15f, 0.2f, 30).dirt(0.6f).pitch(12, 0.04f).sub(0.5f) },
        { 6, B("YOYO").osc("OVERTONE 2").filt(500, 0.45f, 1.0f, 0.2f).amp(2, 0.0f, 0.9f, 50).dirt(0.6f, 0.3f).fm(0.8f).lfo(3, 0.3f).sub(0.55f) },
        { 7, B("STAB").osc("SAW", "PULSE", 0.5f, 12).uni(2, 10, 0.4f).filt(700, 0.4f, 2.8f, 0.1f).amp(0.5f, 0.14f, 0, 40).dirt(0.35f).sub(0.45f) },
        { 7, B("CLEAN SUB").osc("SINE").filt(600, 0.05f, 0.8f, 0.2f).amp(4, 0.0f, 0.9f, 60).dirt(0.05f).sub(0.75f) },
        { 7, B("MELODIC PULSE").osc("SAW SOFT", "SAW SOFT", 0.6f, 12, 10).uni(3, 10, 0.6f).filt(380, 0.25f, 2.2f, 0.18f).amp(2, 0.3f, 0.3f, 60).dirt(0.3f).sub(0.55f) },
        { 7, B("SUPER SAW").osc("SAW").uni(3, 20, 0.8f).filt(900, 0.2f, 1.6f, 0.2f).amp(2, 0.0f, 0.8f, 60).dirt(0.4f).glide(30).sub(0.5f) },
        { 7, B("OFFBEAT PLUCK").osc("SAW", "SQUARE", 0.3f, 12).uni(2, 7, 0.4f).filt(320, 0.35f, 3.4f, 0.1f).amp(0.5f, 0.16f, 0, 40).dirt(0.3f).sub(0.5f) },
        { 7, B("GLIDE SINE").osc("SINE HARM").filt(800, 0.05f, 0.8f, 0.2f).amp(2, 0.0f, 0.9f, 60).dirt(0.25f).glide(90).sub(0.7f) },
        { 7, B("FUTURE FOLD").osc("TRI", "SINE", 0.4f, 12).uni(2, 8, 0.4f).filt(1200, 0.2f, 1.2f, 0.15f).amp(1, 0.0f, 0.8f, 50).dirt(0.6f, 0.7f).lfo(4).sub(0.55f) },
        { 7, B("BRIGHT FM").osc("FM 2", "SAW SOFT", 0.3f).filt(1200, 0.2f, 1.6f, 0.12f).amp(1, 0.2f, 0.3f, 40).dirt(0.3f).fm(0.6f).sub(0.5f) },
    };
    return s;
}

const std::vector<Preset>& presets() {
    static const std::vector<Preset> p = [] {
        std::vector<Preset> v;
        for (auto& s : sounds()) v.push_back(s.preset);
        return v;
    }();
    return p;
}

int firstSoundOfGenre(int genre) {
    const auto& s = sounds();
    for (int i = 0; i < (int) s.size(); ++i) if (s[(size_t) i].genre == genre) return i;
    return 0;
}

const char* automationName(int a) {
    static const char* n[] { "NENHUMA", "HIGH PASS NO FIM", "LOW PASS NO FIM", "LOW PASS ABRINDO", "REVERB CRESCENDO",
                             "DELAY R\xc3\x81PIDO NO FIM", "STUTTER NO FIM", "HIGH PASS + REVERB", "FILTRO WOBBLE", "LOW PASS NO MEIO" };
    return n[a >= 0 && a < numAutomations ? a : 0];
}

} // namespace bass
