#pragma once

#include "Core/Music.h"
#include "Dsp/Synth.h"

#include <vector>

namespace bass {

// A genre ("STYLE" menu): its tempo, its arrangements (grooves) and FX defaults.
struct Arrangement { const char* name; int engine; int groove; };
struct Genre {
    const char* name;      // ASCII; the UI shows accents via displayName
    const char* displayName;
    int bpm;
    std::vector<Arrangement> arrangements;
    float sidechain, reverb, delay;
    int automation;        // default FX automation
};
const std::vector<Genre>& genres();

// Every sound, tagged with the genre it was designed for (SOUND menu: genre -> sounds).
struct Sound { int genre; Preset preset; };
const std::vector<Sound>& sounds();
int firstSoundOfGenre(int genre);

// FX automations over the loop (AUTOMACAO menu).
enum Automation { autoNone = 0, autoHpEnd, autoLpEnd, autoLpOpen, autoReverbBuild, autoDelayThrow, autoStutterEnd,
                  autoHpReverbBuild, autoFilterWobble, autoLpDipHalf, numAutomations };
const char* automationName(int a);   // UTF-8

} // namespace bass
