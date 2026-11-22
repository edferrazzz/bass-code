// Candidate sounds for Ed to pick from (BASS_AUDITION_DIR). Not in the product yet.
// Built with a small fluent helper so each sound reads like a patch sheet.
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

struct Cand { const char* genre; int genStyle; int groove; int bpm; int drums; Preset p; };
enum Drums { dPsy, dHouse, dMinimal, dTechno, dAfro, dBr };

static std::vector<Cand> makeCandidates() {
    std::vector<Cand> c;
    auto add = [&](const char* g, int st, int gr, int bpm, int dr, Preset p) { c.push_back({ g, st, gr, bpm, dr, p }); };

    // ---------------- PSY (145): tight, no sustain, filter envelope does the work
    add("PSY", psy, 0, 145, dPsy, B("FULL ON").osc("SAW").filt(180, 0.3f, 4.2f, 0.075f).amp(0.5f, 0.11f, 0, 18).dirt(0.35f).sub(0.55f));
    add("PSY", psy, 0, 145, dPsy, B("GOA RESO").osc("SAW BRIGHT").filt(220, 0.62f, 3.6f, 0.09f).amp(0.5f, 0.12f, 0, 20).dirt(0.3f).sub(0.5f));
    add("PSY", psy, 0, 148, dPsy, B("DARK FOREST").osc("SQUARE", "FM 3", 0.5f, 12).filt(150, 0.5f, 3.8f, 0.06f).amp(0.5f, 0.1f, 0, 16).dirt(0.5f, 0.3f).fm(0.6f).sub(0.55f));
    add("PSY", psy, 1, 138, dPsy, B("PROG ROUND").osc("TRI", "SAW SOFT", 0.35f).filt(300, 0.15f, 2.6f, 0.12f).amp(1, 0.16f, 0, 30).dirt(0.2f).sub(0.65f));
    add("PSY", psy, 0, 145, dPsy, B("KNOCK").osc("SINE HARM 2").filt(500, 0.1f, 2.0f, 0.05f).amp(0.3f, 0.09f, 0, 14).dirt(0.3f).pitch(5, 0.012f).sub(0.7f));
    add("PSY", psy, 0, 146, dPsy, B("HI TECH ZAP").osc("FM 2").filt(260, 0.4f, 4.6f, 0.05f).amp(0.3f, 0.085f, 0, 12).dirt(0.45f, 0.4f).fm(0.8f).pitch(12, 0.008f).sub(0.5f));
    add("PSY", psy, 2, 145, dPsy, B("TWIN SAW").osc("SAW", "SAW", 0.6f, 0, 14).uni(2, 10, 0.35f).filt(200, 0.35f, 4.0f, 0.08f).amp(0.5f, 0.11f, 0, 18).dirt(0.4f).sub(0.5f));
    add("PSY", psy, 0, 143, dPsy, B("RUBBER ZEN").osc("D BASS 1").filt(350, 0.25f, 2.2f, 0.07f).amp(0.5f, 0.12f, 0, 20).dirt(0.25f).pitch(3, 0.02f).sub(0.65f));

    // ---------------- HOUSE (124)
    add("HOUSE", house, 0, 124, dHouse, B("M ORGAN").osc("ORGAN 2", "SINE", 0.5f, -12).filt(1400, 0.05f, 1.2f, 0.2f).amp(1, 0.35f, 0.35f, 60).dirt(0.15f).sub(0.45f));
    add("HOUSE", house, 1, 124, dHouse, B("PIANO BASS").osc("E BASS 2", "SINE HARM", 0.4f).filt(900, 0.1f, 2.0f, 0.18f).amp(1, 0.45f, 0.15f, 80).dirt(0.15f).sub(0.5f));
    add("HOUSE", house, 2, 124, dHouse, B("FUNK PLUCK").osc("SAW").filt(300, 0.45f, 3.4f, 0.13f).amp(1, 0.25f, 0.15f, 40).dirt(0.3f).sub(0.5f));
    add("HOUSE", house, 1, 128, dHouse, B("TECH RUBBER").osc("SINE HARM").filt(700, 0.2f, 1.6f, 0.08f).amp(0.5f, 0.18f, 0.1f, 30).dirt(0.35f, 0.25f).pitch(4, 0.02f).sub(0.65f));
    add("HOUSE", house, 0, 124, dHouse, B("CHICAGO").osc("SQUARE SOFT", "TRI", 0.5f, 12).filt(600, 0.25f, 1.8f, 0.15f).amp(1, 0.3f, 0.3f, 50).dirt(0.2f).sub(0.5f));
    add("HOUSE", house, 1, 126, dHouse, B("DISCO FILTER").osc("SAW", "SQUARE", 0.4f, 0, 8).uni(2, 6, 0.3f).filt(280, 0.6f, 3.0f, 0.2f).amp(1, 0.3f, 0.3f, 50).dirt(0.25f).sub(0.45f));
    add("HOUSE", house, 0, 124, dHouse, B("BOUNCE FM").osc("FM 1").filt(1000, 0.15f, 1.6f, 0.1f).amp(0.5f, 0.2f, 0.2f, 40).dirt(0.3f).fm(0.4f).pitch(2, 0.015f).sub(0.55f));
    add("HOUSE", house, 2, 124, dHouse, B("GARAGE 2STEP").osc("PULSE", "SAW SOFT", 0.4f).filt(450, 0.3f, 2.4f, 0.12f).amp(1, 0.22f, 0.2f, 45).dirt(0.35f).sub(0.6f));

    // ---------------- DEEP HOUSE (120): warm, round, longer notes
    add("DEEP HOUSE", house, 2, 120, dHouse, B("WARM SUB").osc("TRI SOFT").filt(500, 0.05f, 0.8f, 0.3f).amp(4, 0.0f, 0.9f, 80).dirt(0.1f).sub(0.7f));
    add("DEEP HOUSE", house, 1, 120, dHouse, B("VELVET REESE").osc("SAW SOFT", "SAW SOFT", 0.7f, 0, 18).uni(3, 12, 0.6f).filt(420, 0.15f, 0.8f, 0.3f).amp(6, 0.0f, 0.85f, 100).dirt(0.2f).sub(0.55f));
    add("DEEP HOUSE", house, 0, 120, dHouse, B("SOFT ORGAN").osc("ORGAN 1").filt(800, 0.05f, 0.8f, 0.3f).amp(3, 0.6f, 0.5f, 90).dirt(0.1f).sub(0.55f));
    add("DEEP HOUSE", house, 2, 118, dHouse, B("RHODES BASS").osc("E BASS 1", "SINE", 0.5f, 12).filt(700, 0.05f, 1.2f, 0.25f).amp(2, 0.7f, 0.2f, 120).dirt(0.12f).sub(0.55f));
    add("DEEP HOUSE", house, 1, 120, dHouse, B("DUSTY PLUCK").osc("SQUARE SOFT").filt(380, 0.25f, 2.0f, 0.2f).amp(2, 0.4f, 0.1f, 70).dirt(0.25f, 0, 0.35f).sub(0.55f));
    add("DEEP HOUSE", house, 0, 122, dHouse, B("MOOG ROUND").osc("SAW").filt(220, 0.35f, 1.8f, 0.25f, 0.8f).amp(3, 0.35f, 0.45f, 70).dirt(0.2f).sub(0.55f));
    add("DEEP HOUSE", house, 2, 120, dHouse, B("NIGHT FM").osc("FM 1", "TRI SOFT", 0.5f).filt(600, 0.1f, 1.0f, 0.3f).amp(4, 0.0f, 0.8f, 100).dirt(0.15f).fm(0.25f).sub(0.6f));
    add("DEEP HOUSE", house, 1, 121, dHouse, B("WOOD DEEP").osc("D BASS 2").filt(650, 0.1f, 1.2f, 0.2f).amp(2, 0.45f, 0.2f, 90).dirt(0.12f).sub(0.6f));

    // ---------------- MINIMAL (126): dry, short, clicky
    add("MINIMAL", electronic, 1, 126, dMinimal, B("CLICK SUB").osc("SINE").filt(900, 0.05f, 1.0f, 0.03f).amp(0.3f, 0.1f, 0, 22).dirt(0.15f).pitch(4, 0.008f).sub(0.75f));
    add("MINIMAL", electronic, 1, 126, dMinimal, B("RUBBER BAND").osc("FM 1").filt(700, 0.25f, 2.0f, 0.06f).amp(0.3f, 0.12f, 0, 28).dirt(0.25f).fm(0.6f).pitch(3, 0.025f).sub(0.6f));
    add("MINIMAL", electronic, 2, 125, dMinimal, B("BLIP").osc("PULSE").filt(500, 0.6f, 3.0f, 0.04f).amp(0.3f, 0.08f, 0, 18).dirt(0.2f).sub(0.5f));
    add("MINIMAL", electronic, 1, 126, dMinimal, B("WOODBLOCK").osc("SYM 1").filt(1400, 0.1f, 1.0f, 0.05f).amp(0.3f, 0.09f, 0, 20).dirt(0.2f).pitch(10, 0.01f).sub(0.55f));
    add("MINIMAL", electronic, 0, 124, dMinimal, B("MICRO FM").osc("FM 4").filt(1100, 0.15f, 1.2f, 0.04f).amp(0.3f, 0.1f, 0, 20).dirt(0.2f).fm(0.5f).sub(0.6f));
    add("MINIMAL", electronic, 2, 126, dMinimal, B("BUBBLE").osc("SINE HARM").filt(800, 0.3f, 1.4f, 0.06f).amp(0.5f, 0.14f, 0, 30).dirt(0.15f).lfo(1, 0.6f).sub(0.65f));
    add("MINIMAL", electronic, 1, 127, dMinimal, B("CRUSH TICK").osc("BIT 1").filt(1200, 0.1f, 1.0f, 0.05f).amp(0.3f, 0.1f, 0, 20).dirt(0.3f, 0, 0.5f).sub(0.6f));

    // ---------------- TECHNO (130)
    add("TECHNO", electronic, 0, 132, dTechno, B("RUMBLE").osc("TRI", "DIST 1", 0.3f).filt(240, 0.3f, 1.8f, 0.3f).amp(2, 0.5f, 0.2f, 120).dirt(0.6f).sub(0.65f));
    add("TECHNO", electronic, 0, 130, dTechno, B("DRIVE SAW").osc("SAW", "SAW", 0.5f, 0, 10).uni(2, 8, 0.4f).filt(380, 0.3f, 2.4f, 0.15f).amp(1, 0.22f, 0.3f, 40).dirt(0.55f).sub(0.5f));
    add("TECHNO", electronic, 2, 132, dTechno, B("ACID 303").osc("SAW").filt(260, 0.82f, 3.8f, 0.16f, 0.3f).amp(1, 0.22f, 0.3f, 40).dirt(0.45f).glide(70).sub(0.4f));
    add("TECHNO", electronic, 0, 134, dTechno, B("INDUSTRIAL").osc("DIST 3").uni(2, 14, 0.4f).filt(500, 0.35f, 2.2f, 0.1f).amp(0.5f, 0.15f, 0.2f, 30).dirt(0.75f, 0.5f).sub(0.5f));
    add("TECHNO", electronic, 1, 130, dTechno, B("DARK STAB").osc("SAW", "SQUARE", 0.5f, 7).uni(3, 16, 0.5f).filt(320, 0.5f, 3.0f, 0.08f).amp(0.5f, 0.12f, 0, 40).dirt(0.55f).sub(0.45f));
    add("TECHNO", electronic, 0, 128, dTechno, B("HYPNO FM").osc("FM 3").filt(700, 0.3f, 1.6f, 0.12f).amp(1, 0.18f, 0.2f, 30).dirt(0.4f).fm(0.7f).lfo(16).sub(0.6f));
    add("TECHNO", electronic, 2, 130, dTechno, B("ACID SQUARE").osc("SQUARE").filt(250, 0.8f, 3.4f, 0.14f, 0.3f).amp(1, 0.2f, 0.3f, 40).dirt(0.5f).glide(60).sub(0.4f));
    add("TECHNO", electronic, 0, 131, dTechno, B("RAW PULSE").osc("RAW 2").filt(420, 0.25f, 2.0f, 0.1f).amp(0.5f, 0.16f, 0.2f, 30).dirt(0.6f, 0.2f).sub(0.55f));

    // ---------------- AFRO HOUSE (122): percussive, pitched, log drums
    add("AFRO HOUSE", afroHouse, 1, 122, dAfro, B("LOG DRUM").osc("SINE HARM").filt(1500, 0.05f, 0.8f, 0.1f).amp(0.5f, 0.24f, 0, 60).dirt(0.35f).pitch(7, 0.045f).sub(0.55f));
    add("AFRO HOUSE", afroHouse, 1, 120, dAfro, B("AMAPIANO LOG").osc("SINE", "TRI", 0.4f, 12).filt(2000, 0.05f, 0.6f, 0.2f).amp(0.5f, 0.45f, 0, 90).dirt(0.5f).pitch(12, 0.07f).glide(50).sub(0.65f));
    add("AFRO HOUSE", afroHouse, 1, 122, dAfro, B("TALKING").osc("VOICE 2").filt(1000, 0.25f, 1.4f, 0.14f).amp(0.5f, 0.25f, 0, 50).dirt(0.25f).pitch(4, 0.06f).sub(0.5f));
    add("AFRO HOUSE", afroHouse, 0, 122, dAfro, B("DEEP AFRO").osc("TRI SOFT", "E BASS 1", 0.4f).filt(800, 0.1f, 1.2f, 0.2f).amp(3, 0.45f, 0.5f, 70).dirt(0.15f).sub(0.6f));
    add("AFRO HOUSE", afroHouse, 1, 122, dAfro, B("MARIMBA SUB").osc("PLUCK").filt(1600, 0.1f, 1.0f, 0.1f).amp(0.5f, 0.3f, 0, 60).dirt(0.15f).pitch(12, 0.02f).sub(0.55f));
    add("AFRO HOUSE", afroHouse, 0, 122, dAfro, B("DRUM BASS").osc("FM 2").filt(900, 0.15f, 1.2f, 0.08f).amp(0.5f, 0.2f, 0, 50).dirt(0.35f).fm(0.5f).pitch(5, 0.03f).sub(0.6f));
    add("AFRO HOUSE", afroHouse, 1, 122, dAfro, B("WOOD").osc("SYM 2").filt(700, 0.35f, 2.2f, 0.08f).amp(0.5f, 0.16f, 0, 40).dirt(0.2f).pitch(3, 0.03f).sub(0.5f));
    add("AFRO HOUSE", afroHouse, 2, 120, dAfro, B("TRIBAL WARM").osc("SAW SOFT").filt(380, 0.15f, 1.4f, 0.25f).amp(3, 0.5f, 0.5f, 80).dirt(0.2f).sub(0.6f));

    // ---------------- BRAZILIAN BASS (125): bouncy, metallic, moving
    add("BRAZILIAN BASS", brazilianBass, 0, 125, dBr, B("BR BOUNCE").osc("FM 3", "SQUARE", 0.3f).filt(700, 0.3f, 2.0f, 0.12f).amp(1, 0.0f, 0.85f, 40).dirt(0.55f, 0.3f).fm(0.7f).lfo(2).sub(0.55f));
    add("BRAZILIAN BASS", brazilianBass, 1, 125, dBr, B("WOBBLE 8TH").osc("VOICE 1", "SAW", 0.4f).filt(420, 0.45f, 1.0f, 0.2f).amp(2, 0.0f, 0.9f, 50).dirt(0.6f, 0.2f).fm(0.6f).lfo(2).glide(40).sub(0.55f));
    add("BRAZILIAN BASS", brazilianBass, 2, 125, dBr, B("SLAP").osc("E BASS 3").uni(2, 8, 0.3f).filt(900, 0.3f, 2.4f, 0.08f).amp(0.5f, 0.2f, 0.3f, 40).dirt(0.5f).pitch(5, 0.03f).sub(0.5f));
    add("BRAZILIAN BASS", brazilianBass, 1, 125, dBr, B("GROWL").osc("VOICE 3", "FM 4", 0.4f, 0, 9).uni(2, 9, 0.4f).filt(600, 0.45f, 1.6f, 0.15f).amp(2, 0.0f, 0.9f, 50).dirt(0.7f, 0.5f).fm(0.9f).lfo(4).sub(0.5f));
    add("BRAZILIAN BASS", brazilianBass, 0, 125, dBr, B("DIRTY SAW").osc("DIST 2").uni(3, 16, 0.5f).filt(650, 0.35f, 2.0f, 0.18f).amp(2, 0.0f, 0.85f, 50).dirt(0.65f).lfo(4).sub(0.55f));
    add("BRAZILIAN BASS", brazilianBass, 0, 126, dBr, B("METAL BOUNCE").osc("FM 4").uni(2, 12, 0.4f).filt(900, 0.35f, 2.0f, 0.1f).amp(1, 0.0f, 0.8f, 40).dirt(0.7f, 0.6f).fm(1.2f).pitch(3, 0.02f).sub(0.5f));
    add("BRAZILIAN BASS", brazilianBass, 2, 125, dBr, B("LASER").osc("SAW BRIGHT").uni(2, 10, 0.3f).filt(800, 0.55f, 3.0f, 0.06f).amp(0.5f, 0.15f, 0.2f, 30).dirt(0.6f).pitch(12, 0.04f).sub(0.5f));
    add("BRAZILIAN BASS", brazilianBass, 1, 125, dBr, B("YOYO").osc("OVERTONE 2").filt(500, 0.45f, 1.0f, 0.2f).amp(2, 0.0f, 0.9f, 50).dirt(0.6f, 0.3f).fm(0.8f).lfo(3, 0.3f).sub(0.55f));

    // ---------------- ELETRONICA (128): melodic/big room/future
    add("ELETRONICA", electronic, 1, 128, dHouse, B("STAB").osc("SAW", "PULSE", 0.5f, 12).uni(2, 10, 0.4f).filt(700, 0.4f, 2.8f, 0.1f).amp(0.5f, 0.14f, 0, 40).dirt(0.35f).sub(0.45f));
    add("ELETRONICA", electronic, 0, 126, dHouse, B("CLEAN SUB").osc("SINE").filt(600, 0.05f, 0.8f, 0.2f).amp(4, 0.0f, 0.9f, 60).dirt(0.05f).sub(0.75f));
    add("ELETRONICA", electronic, 0, 124, dHouse, B("MELODIC PULSE").osc("SAW SOFT", "SAW SOFT", 0.6f, 12, 10).uni(3, 10, 0.6f).filt(380, 0.25f, 2.2f, 0.18f).amp(2, 0.3f, 0.3f, 60).dirt(0.3f).sub(0.55f));
    add("ELETRONICA", electronic, 2, 128, dHouse, B("SUPER SAW").osc("SAW").uni(3, 20, 0.8f).filt(900, 0.2f, 1.6f, 0.2f).amp(2, 0.0f, 0.8f, 60).dirt(0.4f).glide(30).sub(0.5f));
    add("ELETRONICA", electronic, 0, 128, dHouse, B("OFFBEAT PLUCK").osc("SAW", "SQUARE", 0.3f, 12).uni(2, 7, 0.4f).filt(320, 0.35f, 3.4f, 0.1f).amp(0.5f, 0.16f, 0, 40).dirt(0.3f).sub(0.5f));
    add("ELETRONICA", electronic, 2, 126, dHouse, B("GLIDE SINE").osc("SINE HARM").filt(800, 0.05f, 0.8f, 0.2f).amp(2, 0.0f, 0.9f, 60).dirt(0.25f).glide(90).sub(0.7f));
    add("ELETRONICA", electronic, 0, 128, dHouse, B("FUTURE FOLD").osc("TRI", "SINE", 0.4f, 12).uni(2, 8, 0.4f).filt(1200, 0.2f, 1.2f, 0.15f).amp(1, 0.0f, 0.8f, 50).dirt(0.6f, 0.7f).lfo(4).sub(0.55f));
    add("ELETRONICA", electronic, 1, 128, dHouse, B("BRIGHT FM").osc("FM 2", "SAW SOFT", 0.3f).filt(1200, 0.2f, 1.6f, 0.12f).amp(1, 0.2f, 0.3f, 40).dirt(0.3f).fm(0.6f).sub(0.5f));
    return c;
}
static const std::vector<Cand> candidates = makeCandidates();

// ---- simple drums so the bass is heard in context (audition only)
static void addDrums(juce::AudioBuffer<float>& buf, int kind, double bpm, double sr) {
    const int n = buf.getNumSamples();
    const double spb = 60.0 / bpm * sr;
    uint32_t rnd = 1234567u;
    auto noise = [&] { rnd = rnd * 1664525u + 1013904223u; return (float) (rnd >> 9) / 4194304.0f - 1.0f; };
    const int beats = (int) (n / spb);
    for (int b = 0; b < beats; ++b) {
        const int s0 = (int) (b * spb);
        // kick on every beat (afro/br skip a few for groove)
        const bool kick = !(kind == dAfro && b % 4 == 3);
        if (kick) {
            const float k0 = kind == dPsy ? 160.0f : kind == dTechno ? 120.0f : 140.0f;
            const float len = kind == dPsy ? 0.16f : kind == dTechno ? 0.35f : 0.28f;
            double ph = 0;
            for (int i = 0; i < (int) (len * sr) && s0 + i < n; ++i) {
                const float t = (float) (i / sr);
                const float f = 45.0f + k0 * std::exp(-t * 30.0f);
                ph += f / sr;
                const float env = std::exp(-t * (kind == dPsy ? 22.0f : 9.0f));
                const float v = (float) std::sin(6.2831853 * ph) * env * 0.85f + (i < 60 ? noise() * 0.3f * (1.0f - i / 60.0f) : 0.0f);
                buf.addSample(0, s0 + i, v); buf.addSample(1, s0 + i, v);
            }
        }
        // offbeat hat
        const int h0 = (int) (s0 + spb * 0.5);
        for (int i = 0; i < (int) (0.05 * sr) && h0 + i < n; ++i) {
            static float hp = 0, last = 0;
            const float x = noise();
            hp = 0.6f * (hp + x - last); last = x;
            const float v = hp * std::exp(-(float) i / (float) (0.012 * sr)) * 0.16f;
            buf.addSample(0, h0 + i, v * 0.8f); buf.addSample(1, h0 + i, v);
        }
        // clap on 2 and 4 (not psy, which keeps it driving)
        if (kind != dPsy && b % 2 == 1) {
            for (int i = 0; i < (int) (0.18 * sr) && s0 + i < n; ++i) {
                const float t = (float) (i / sr);
                const float env = std::exp(-t * 25.0f) * (t < 0.03f ? (std::fmod(t, 0.01f) < 0.005f ? 1.0f : 0.5f) : 1.0f);
                const float v = noise() * env * 0.22f;
                buf.addSample(0, s0 + i, v); buf.addSample(1, s0 + i, v * 0.9f);
            }
        }
        // afro shaker 16ths
        if (kind == dAfro || kind == dMinimal)
            for (int q = 1; q < 4; q += 2) {
                const int q0 = (int) (s0 + spb * 0.25 * q);
                for (int i = 0; i < (int) (0.03 * sr) && q0 + i < n; ++i) {
                    const float v = noise() * std::exp(-(float) i / (float) (0.006 * sr)) * 0.07f;
                    buf.addSample(0, q0 + i, v); buf.addSample(1, q0 + i, v * 1.1f);
                }
            }
    }
}
