#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Processor.h"

namespace bass {

class Look;
class Root;

class Editor final : public juce::AudioProcessorEditor {
public:
    explicit Editor(Processor&);
    ~Editor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

    static constexpr int designW = 1200, designH = 856;

private:
    std::unique_ptr<Look> look;
    std::unique_ptr<Root> root;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};

} // namespace bass
