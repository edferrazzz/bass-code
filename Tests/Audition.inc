// Audition helpers (BASS_AUDITION_DIR / BASS_DEMO_DIR): simple drums so the bass is heard in context.
enum Drums { dPsy, dHouse, dMinimal, dTechno, dAfro, dBr };
static int drumsForGenre(int g) { static const int d[] { dPsy, dHouse, dHouse, dMinimal, dTechno, dAfro, dBr, dHouse }; return d[juce::jlimit(0, 7, g)]; }

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
