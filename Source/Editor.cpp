#include "Editor.h"
#include "BinaryData.h"

#include <cmath>

namespace bass {

static juce::String T(const char* utf8) { return juce::String::fromUTF8(utf8); }

// ===========================================================================
// Colours + fonts
// ===========================================================================
namespace col {
const juce::Colour bg { 0xff0d0f17 }, panel { 0xff151827 }, panel2 { 0xff1b1f31 }, line { 0xff262b42 };
const juce::Colour text { 0xffe9ecf8 }, dim { 0xff7d83a3 }, soft { 0xffaab0cc }, dark { 0xff0b1206 };
const juce::Colour limeA { 0xffc8ff2e }, limeB { 0xff4dff7a }, well { 0xff0f1220 };
}

static juce::Typeface::Ptr face(const char* d, int n) { return juce::Typeface::createSystemTypefaceFor(d, (size_t) n); }
static juce::Font interBold(float h) {
    static auto tf = face(BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize);
    return juce::Font(juce::FontOptions(tf).withHeight(h));
}
static juce::Font interRegular(float h) {
    static auto tf = face(BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize);
    return juce::Font(juce::FontOptions(tf).withHeight(h));
}
static juce::Font spaceBold(float h) {
    static auto tf = face(BinaryData::SpaceGroteskBold_ttf, BinaryData::SpaceGroteskBold_ttfSize);
    return juce::Font(juce::FontOptions(tf).withHeight(h));
}

static juce::ColourGradient lime(juce::Rectangle<float> r) {
    return juce::ColourGradient(col::limeA, r.getX(), r.getY(), col::limeB, r.getRight(), r.getY(), false);
}

static void label(juce::Graphics& g, const juce::String& s, float x, float y, float w = 300.0f) {
    g.setColour(col::dim);
    g.setFont(interBold(10.5f).withExtraKerningFactor(0.18f));
    g.drawText(s, juce::Rectangle<float>(x, y, w, 14.0f), juce::Justification::centredLeft, false);
}

static void card(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour fill = col::panel) {
    g.setColour(fill);
    g.fillRoundedRectangle(r, 12.0f);
    g.setColour(col::line);
    g.drawRoundedRectangle(r.reduced(0.5f), 12.0f, 1.0f);
}

static juce::String styleDisplay(int s) {
    return s == electronic ? T("ELETRÔNICA") : juce::String(style(s).name);
}
static juce::String scaleDisplay(int i) {
    static const char* n[] { "MENOR", "MAIOR", "FRÍGIO", "DÓRICO", "MENOR HARMÔNICA" };
    return T(n[juce::jlimit(0, 4, i)]);
}

// ===========================================================================
// LookAndFeel
// ===========================================================================
class Look final : public juce::LookAndFeel_V4 {
public:
    Look() {
        setColour(juce::PopupMenu::backgroundColourId, col::panel2);
        setColour(juce::PopupMenu::textColourId, col::text);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, col::limeA);
        setColour(juce::PopupMenu::highlightedTextColourId, col::dark);
        setColour(juce::ComboBox::textColourId, col::text);
        setColour(juce::ComboBox::backgroundColourId, col::panel2);
        setColour(juce::ComboBox::outlineColourId, col::line);
        setColour(juce::ComboBox::arrowColourId, col::dim);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override { return interBold(14.0f); }
    juce::Font getPopupMenuFont() override { return interBold(14.0f); }

    void drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override {
        auto r = juce::Rectangle<float>(0, 0, (float) w, (float) h).reduced(0.5f);
        g.setColour(col::panel2);
        g.fillRoundedRectangle(r, 9.0f);
        g.setColour(box.isMouseOver(true) ? juce::Colour(0xff363c5c) : col::line);
        g.drawRoundedRectangle(r, 9.0f, 1.0f);
        juce::Path p;
        const float cx = (float) w - 22.0f, cy = (float) h * 0.5f;
        p.startNewSubPath(cx - 5.0f, cy - 2.5f);
        p.lineTo(cx, cy + 2.5f);
        p.lineTo(cx + 5.0f, cy - 2.5f);
        g.setColour(col::dim);
        g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& l) override {
        l.setBounds(14, 0, box.getWidth() - 40, box.getHeight());
        l.setFont(getComboBoxFont(box));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1,
                          juce::Slider&) override {
        auto r = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h);
        const float d = std::min(r.getWidth(), r.getHeight()) - 4.0f;
        auto c = r.withSizeKeepingCentre(d, d);
        const float rad = d * 0.5f - 4.0f;
        const float ang = a0 + pos * (a1 - a0);
        juce::Path track, val;
        track.addCentredArc(c.getCentreX(), c.getCentreY(), rad, rad, 0, a0, a1, true);
        val.addCentredArc(c.getCentreX(), c.getCentreY(), rad, rad, 0, a0, ang, true);
        g.setColour(col::line);
        g.strokePath(track, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setGradientFill(juce::ColourGradient(col::limeA, c.getX(), c.getBottom(), col::limeB, c.getRight(), c.getY(), false));
        g.strokePath(val, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        auto inner = c.reduced(12.0f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2a3050), inner.getX(), inner.getY(),
                                               juce::Colour(0xff141727), inner.getRight(), inner.getBottom(), false));
        g.fillEllipse(inner);
        const float len = inner.getWidth() * 0.36f;
        const auto ctr = inner.getCentre();
        const juce::Point<float> tip(ctr.x + len * std::sin(ang), ctr.y - len * std::cos(ang));
        const juce::Point<float> base(ctr.x + len * 0.25f * std::sin(ang), ctr.y - len * 0.25f * std::cos(ang));
        g.setColour(juce::Colours::white);
        g.drawLine({ base, tip }, 3.0f);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                          juce::Slider::SliderStyle, juce::Slider& s) override {
        const float cy = (float) y + (float) h * 0.5f;
        auto track = juce::Rectangle<float>((float) x, cy - 3.0f, (float) w, 6.0f);
        g.setColour(col::line);
        g.fillRoundedRectangle(track, 3.0f);
        auto fill = track.withRight(pos);
        const bool on = s.isEnabled();
        if (on) g.setGradientFill(lime(track)); else g.setColour(juce::Colour(0xff3a3f58));
        g.fillRoundedRectangle(fill, 3.0f);
        auto thumb = juce::Rectangle<float>(18.0f, 18.0f).withCentre({ pos, cy });
        g.setColour(juce::Colours::white);
        g.fillEllipse(thumb);
        g.setColour(on ? col::limeB : juce::Colour(0xff555b78));
        g.drawEllipse(thumb.reduced(1.5f), 3.0f);
    }

    // the only TextButton is the SWEEP chip, whose label Root paints itself
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override {}
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override {}

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool) override {
        auto r = b.getLocalBounds().toFloat().withSizeKeepingCentre(34.0f, 18.0f);
        const bool on = b.getToggleState();
        if (on) g.setGradientFill(lime(r)); else g.setColour(juce::Colour(0xff2a2f48));
        g.fillRoundedRectangle(r, 9.0f);
        auto knob = juce::Rectangle<float>(14.0f, 14.0f).withCentre({ on ? r.getRight() - 9.0f : r.getX() + 9.0f, r.getCentreY() });
        g.setColour(on ? juce::Colours::white : col::dim);
        g.fillEllipse(knob);
    }
};

// ===========================================================================
// Widgets
// ===========================================================================
// Row of chips / segments.
class Segments final : public juce::Component {
public:
    std::function<int()> get;
    std::function<void(int)> set;
    juce::StringArray items;
    bool big = false;   // STYLE chips (separate buttons) vs. segmented control

    void paint(juce::Graphics& g) override {
        const int sel = get ? get() : 0;
        auto r = getLocalBounds().toFloat();
        if (!big) {
            g.setColour(col::well);
            g.fillRoundedRectangle(r, 8.0f);
            r = r.reduced(4.0f);
        }
        const float gap = big ? 8.0f : 4.0f;
        const float w = (r.getWidth() - gap * (float) (items.size() - 1)) / (float) items.size();
        for (int i = 0; i < items.size(); ++i) {
            auto c = juce::Rectangle<float>(r.getX() + (float) i * (w + gap), r.getY(), w, r.getHeight());
            if (i == sel) {
                g.setGradientFill(lime(c));
                g.fillRoundedRectangle(c, big ? 9.0f : 6.0f);
            } else if (big) {
                g.setColour(i == hover ? juce::Colour(0xff222740) : col::panel2);
                g.fillRoundedRectangle(c, 9.0f);
                g.setColour(col::line);
                g.drawRoundedRectangle(c.reduced(0.5f), 9.0f, 1.0f);
            }
            g.setColour(i == sel ? col::dark : (big ? col::soft : col::dim));
            g.setFont(interBold(big ? 12.5f : 11.5f).withExtraKerningFactor(big ? 0.1f : 0.04f));
            g.drawText(items[i], c, juce::Justification::centred, false);
        }
    }
    int indexAt(juce::Point<int> p) const {
        if (items.isEmpty()) return -1;
        return juce::jlimit(0, items.size() - 1, (int) ((float) p.x / (float) getWidth() * (float) items.size()));
    }
    void mouseDown(const juce::MouseEvent& e) override { if (set) set(indexAt(e.getPosition())); repaint(); }
    void mouseMove(const juce::MouseEvent& e) override { hover = indexAt(e.getPosition()); repaint(); }
    void mouseExit(const juce::MouseEvent&) override { hover = -1; repaint(); }

private:
    int hover = -1;
};

class Btn final : public juce::Button {
public:
    enum Kind { play, variation, generate, dragMidi, dragWav };
    Btn(Kind k) : juce::Button("btn"), kind(k) { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    std::function<void()> onDragOut;
    bool active = false;

    void paintButton(juce::Graphics& g, bool over, bool down) override {
        auto r = getLocalBounds().toFloat().reduced(0.5f);
        if (kind == generate) {
            g.setGradientFill(lime(r));
            g.fillRoundedRectangle(r, 10.0f);
            if (over) { g.setColour(juce::Colours::white.withAlpha(down ? 0.18f : 0.1f)); g.fillRoundedRectangle(r, 10.0f); }
        } else {
            g.setColour(down ? juce::Colour(0xff252a44) : over ? juce::Colour(0xff20243a) : col::panel2);
            g.fillRoundedRectangle(r, 10.0f);
            g.setColour(active ? col::limeA : col::line);
            g.drawRoundedRectangle(r, 10.0f, active ? 1.5f : 1.0f);
        }
        const auto c = kind == generate ? col::dark : col::text;
        juce::String t;
        switch (kind) {
            case play: t = active ? "PARAR" : "OUVIR"; break;
            case variation: t = T("VARIAÇÃO"); break;
            case generate: t = "GERAR"; break;
            case dragMidi: t = "ARRASTAR MIDI"; break;
            case dragWav: t = "ARRASTAR WAV"; break;
        }
        auto f = kind == generate ? spaceBold(21.0f).withExtraKerningFactor(0.32f) : interBold(12.5f).withExtraKerningFactor(0.12f);
        const float tw = juce::GlyphArrangement::getStringWidth(f, t);
        const float iconW = 16.0f, gap = 9.0f;
        const float x0 = r.getCentreX() - (tw + iconW + gap) * 0.5f;
        auto icon = juce::Rectangle<float>(x0, r.getCentreY() - 8.0f, iconW, 16.0f);
        drawIcon(g, icon, c);
        g.setColour(c);
        g.setFont(f);
        g.drawText(t, juce::Rectangle<float>(x0 + iconW + gap, r.getY(), tw + 4.0f, r.getHeight()), juce::Justification::centredLeft, false);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if ((kind == dragMidi || kind == dragWav) && e.getDistanceFromDragStart() > 6 && !dragging && onDragOut) {
            dragging = true;
            onDragOut();
        }
        juce::Button::mouseDrag(e);
    }
    void mouseUp(const juce::MouseEvent& e) override {
        const bool wasDrag = dragging;
        dragging = false;
        if (wasDrag) { setState(juce::Button::buttonNormal); return; }
        juce::Button::mouseUp(e);
    }

private:
    void drawIcon(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c) {
        g.setColour(c);
        juce::Path p;
        const auto ctr = r.getCentre();
        const juce::PathStrokeType st(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        switch (kind) {
            case play:
                if (active) { g.fillRoundedRectangle(r.withSizeKeepingCentre(11.0f, 11.0f), 2.0f); return; }
                p.addTriangle(ctr.x - 5.0f, ctr.y - 6.0f, ctr.x - 5.0f, ctr.y + 6.0f, ctr.x + 6.0f, ctr.y);
                g.fillPath(p);
                return;
            case variation:
                p.addCentredArc(ctr.x, ctr.y, 6.0f, 6.0f, 0, 0.6f, 5.6f, true);
                g.strokePath(p, st);
                p.clear();
                p.addTriangle(ctr.x + 2.0f, ctr.y - 9.0f, ctr.x + 7.0f, ctr.y - 5.0f, ctr.x + 1.0f, ctr.y - 3.0f);
                g.fillPath(p);
                return;
            case generate: {   // sparkle
                for (int i = 0; i < 4; ++i) {
                    const float a = (float) i * juce::MathConstants<float>::halfPi;
                    p.startNewSubPath(ctr);
                    p.lineTo(ctr.x + 8.0f * std::sin(a), ctr.y - 8.0f * std::cos(a));
                }
                g.strokePath(p, juce::PathStrokeType(2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                return;
            }
            case dragMidi:
                p.startNewSubPath(ctr.x, ctr.y - 7.0f); p.lineTo(ctr.x, ctr.y + 5.0f);
                p.startNewSubPath(ctr.x - 4.5f, ctr.y + 0.5f); p.lineTo(ctr.x, ctr.y + 5.0f); p.lineTo(ctr.x + 4.5f, ctr.y + 0.5f);
                g.strokePath(p, st);
                return;
            case dragWav:
                p.startNewSubPath(ctr.x - 8.0f, ctr.y);
                for (int i = 0; i <= 16; ++i) p.lineTo(ctr.x - 8.0f + (float) i, ctr.y - 5.0f * std::sin((float) i * 0.8f) * (1.0f - std::abs((float) i - 8.0f) / 10.0f));
                g.strokePath(p, st);
                return;
        }
    }
    Kind kind;
    bool dragging = false;
};

// ===========================================================================
// Piano roll
// ===========================================================================
class PianoRoll final : public juce::Component {
public:
    explicit PianoRoll(Processor& p) : proc(p) { setWantsKeyboardFocus(true); refreshFromProcessor(); }

    void refreshFromProcessor() {
        pat = proc.getPattern();
        if (selected >= (int) pat.notes.size()) selected = -1;
        fitRange();
        repaint();
    }

    void setPlayhead(double steps) {
        if (std::abs(steps - playhead) > 0.01) { playhead = steps; repaint(); }
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.setColour(col::well);
        g.fillRoundedRectangle(r, 12.0f);
        g.setColour(col::line);
        g.drawRoundedRectangle(r.reduced(0.5f), 12.0f, 1.0f);

        const auto grid = gridArea();
        const int steps = pat.steps();
        const float cw = grid.getWidth() / (float) steps, rh = rowH();

        // rows + keys
        for (int i = 0; i < rows; ++i) {
            const int pitch = top - i;
            const bool black = isBlack(pitch);
            const float y = grid.getY() + (float) i * rh;
            if (black) { g.setColour(juce::Colour(0xff0c0e1a)); g.fillRect(grid.getX(), y, grid.getWidth(), rh); }
            if (isInScale(pitch)) { g.setColour(col::limeA.withAlpha(0.025f)); g.fillRect(grid.getX(), y, grid.getWidth(), rh); }
            auto key = juce::Rectangle<float>(r.getX() + 12.0f, y, 44.0f, rh);
            g.setColour(juce::Colour(0xffd9dce8));
            g.fillRect(key);
            if (black) { g.setColour(juce::Colour(0xff11131d)); g.fillRect(key.withWidth(28.0f).reduced(0.0f, 1.0f)); }
            else if ((pitch % 12) == 4 || (pitch % 12) == 11) { g.setColour(juce::Colour(0xff9aa0b8)); g.fillRect(key.getX(), y, key.getWidth(), 1.0f); }
            if (pitch % 12 == 0) {
                g.setColour(juce::Colour(0xff4a4f68));
                g.setFont(interBold(8.5f));
                g.drawText(noteLabel(pitch), key.withTrimmedRight(3.0f), juce::Justification::centredRight, false);
            }
            g.setColour(juce::Colours::white.withAlpha(0.025f));
            g.drawHorizontalLine((int) y, grid.getX(), grid.getRight());
        }
        // vertical grid
        for (int st = 0; st <= steps; ++st) {
            const float x = grid.getX() + (float) st * cw;
            g.setColour(juce::Colours::white.withAlpha(st % 16 == 0 ? 0.16f : st % 4 == 0 ? 0.07f : 0.025f));
            g.drawVerticalLine((int) x, grid.getY() - (st % 16 == 0 ? 24.0f : 0.0f), grid.getBottom());
        }
        // bar labels
        for (int b = 0; b < pat.bars; ++b) {
            const float x = grid.getX() + (float) (b * 16) * cw;
            juce::String chord;
            if (b < (int) pat.barRoots.size()) chord = juce::String(noteName(pat.barRoots[(size_t) b])) + (b < (int) pat.barMinor.size() && pat.barMinor[(size_t) b] ? "m" : "");
            g.setColour(col::dim);
            g.setFont(interBold(11.0f));
            g.drawText(juce::String(b + 1), juce::Rectangle<float>(x + 8.0f, r.getY() + 10.0f, 20.0f, 18.0f), juce::Justification::centredLeft, false);
            g.setColour(juce::Colour(0xffd6f7b0));
            g.drawText(chord, juce::Rectangle<float>(x + 8.0f + (b + 1 >= 10 ? 18.0f : 12.0f), r.getY() + 10.0f, 60.0f, 18.0f), juce::Justification::centredLeft, false);
        }
        // notes
        for (int i = 0; i < (int) pat.notes.size(); ++i) {
            const auto nr = noteRect(pat.notes[(size_t) i]);
            if (nr.isEmpty()) continue;
            g.setGradientFill(lime(nr));
            g.setOpacity(i == selected ? 1.0f : 0.55f + 0.45f * pat.notes[(size_t) i].velocity * pat.notes[(size_t) i].velocity);
            g.fillRoundedRectangle(nr, 4.0f);
            g.setOpacity(1.0f);
            if (pat.notes[(size_t) i].slide) {
                g.setColour(col::dark.withAlpha(0.6f));
                g.fillRoundedRectangle(nr.withWidth(3.0f).translated(2.0f, 0).reduced(0, 3.0f), 1.0f);
            }
            if (i == selected) {
                g.setColour(juce::Colours::white);
                g.drawRoundedRectangle(nr.expanded(1.5f), 5.0f, 2.0f);
            }
        }
        // tooltip on the selected note
        if (selected >= 0 && selected < (int) pat.notes.size()) {
            const auto nr = noteRect(pat.notes[(size_t) selected]);
            auto text = juce::String(noteLabel(pat.notes[(size_t) selected].pitch)) + T("   ↑↓");
            auto f = interBold(11.0f);
            const float w = juce::GlyphArrangement::getStringWidth(f, text) + 14.0f;
            auto tip = juce::Rectangle<float>(w, 20.0f).withCentre({ nr.getCentreX(), nr.getY() - 15.0f });
            tip = tip.constrainedWithin(grid.withTop(grid.getY() - 26.0f));
            g.setColour(juce::Colours::white);
            g.fillRoundedRectangle(tip, 5.0f);
            g.setColour(col::dark);
            g.setFont(f);
            g.drawText(text, tip, juce::Justification::centred, false);
        }
        // kick guide lane
        const auto kl = kickLane();
        g.setColour(col::dim);
        g.setFont(interBold(9.5f).withExtraKerningFactor(0.15f));
        g.drawText("KICK", juce::Rectangle<float>(r.getX() + 12.0f, kl.getY(), 44.0f, kl.getHeight()), juce::Justification::centredLeft, false);
        for (int b = 0; b < steps / 4; ++b) {
            auto k = juce::Rectangle<float>(grid.getX() + (float) (b * 4) * cw + 1.0f, kl.getCentreY() - 6.0f, std::max(4.0f, cw - 3.0f), 12.0f);
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.fillRoundedRectangle(k, 3.0f);
            g.setColour(juce::Colours::white.withAlpha(0.25f));
            g.drawRoundedRectangle(k, 3.0f, 1.0f);
        }
        // playhead
        if (playhead >= 0.0) {
            const float x = grid.getX() + (float) playhead * cw;
            g.setColour(col::limeA.withAlpha(0.85f));
            g.fillRect(x - 1.0f, grid.getY(), 2.0f, kl.getBottom() - grid.getY());
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        grabKeyboardFocus();
        const auto pos = e.position;
        const int hit = noteAt(pos);
        mode = none;
        if (hit >= 0) {
            selected = hit;
            const auto nr = noteRect(pat.notes[(size_t) hit]);
            mode = pos.x > nr.getRight() - 6.0f ? resize : move;
        } else if (gridArea().contains(pos)) {
            Note n;
            n.start = stepAt(pos.x);
            n.pitch = pitchAt(pos.y);
            n.length = defaultLength();
            n.velocity = 0.9f;
            pat.notes.push_back(n);
            selected = (int) pat.notes.size() - 1;
            mode = move;
            commit();
        } else {
            selected = -1;
        }
        if (selected >= 0) dragOrig = pat.notes[(size_t) selected];
        dragStartStep = stepAt(pos.x);
        dragStartPitch = pitchAt(pos.y);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (selected < 0 || mode == none) return;
        auto& n = pat.notes[(size_t) selected];
        const int ds = stepAt(e.position.x) - dragStartStep;
        if (mode == move) {
            n.start = juce::jlimit(0, pat.steps() - n.length, dragOrig.start + ds);
            n.pitch = juce::jlimit(20, 72, dragOrig.pitch + (pitchAt(e.position.y) - dragStartPitch));
        } else {
            n.length = juce::jlimit(1, pat.steps() - n.start, dragOrig.length + ds);
        }
        commit();
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (mode != none) { sortKeepSelection(); commit(); }
        mode = none;
    }

    void mouseDoubleClick(const juce::MouseEvent& e) override {
        const int hit = noteAt(e.position);
        if (hit >= 0) { pat.notes.erase(pat.notes.begin() + hit); selected = -1; commit(); }
    }

    void mouseMove(const juce::MouseEvent& e) override {
        const int hit = noteAt(e.position);
        if (hit >= 0 && e.position.x > noteRect(pat.notes[(size_t) hit]).getRight() - 6.0f) setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        else if (hit >= 0) setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        else setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    bool keyPressed(const juce::KeyPress& k) override {
        if (selected < 0 || selected >= (int) pat.notes.size()) return false;
        auto& n = pat.notes[(size_t) selected];
        const int code = k.getKeyCode();
        const int big = k.getModifiers().isShiftDown() ? 12 : 1;
        if (code == juce::KeyPress::upKey) n.pitch = juce::jlimit(20, 72, n.pitch + big);
        else if (code == juce::KeyPress::downKey) n.pitch = juce::jlimit(20, 72, n.pitch - big);
        else if (code == juce::KeyPress::leftKey) n.start = juce::jmax(0, n.start - 1);
        else if (code == juce::KeyPress::rightKey) n.start = juce::jmin(pat.steps() - n.length, n.start + 1);
        else if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey) { pat.notes.erase(pat.notes.begin() + selected); selected = -1; }
        else return false;
        if (n.pitch < bottom() || n.pitch > top) fitRange();
        sortKeepSelection();
        commit();
        return true;
    }

private:
    enum Mode { none, move, resize };

    juce::Rectangle<float> gridArea() const {
        auto r = getLocalBounds().toFloat();
        return { r.getX() + 64.0f, r.getY() + 36.0f, r.getWidth() - 76.0f, r.getHeight() - 36.0f - 40.0f };
    }
    juce::Rectangle<float> kickLane() const {
        auto g = gridArea();
        return { g.getX(), g.getBottom() + 6.0f, g.getWidth(), 28.0f };
    }
    float rowH() const { return gridArea().getHeight() / (float) rows; }
    int bottom() const { return top - rows + 1; }
    static bool isBlack(int p) { const int pc = p % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }
    bool isInScale(int p) const {
        const auto& sc = scales()[(size_t) juce::jlimit(0, (int) scales().size() - 1, pat.scale)].steps;
        const int pc = ((p - pat.key) % 12 + 12) % 12;
        for (auto s : sc) if (s == pc) return true;
        return false;
    }
    int defaultLength() const { return proc.currentGenSettings().style == psy ? 1 : 2; }

    juce::Rectangle<float> noteRect(const Note& n) const {
        const auto grid = gridArea();
        if (n.pitch > top || n.pitch < bottom()) return {};
        const float cw = grid.getWidth() / (float) pat.steps(), rh = rowH();
        return { grid.getX() + (float) n.start * cw + 1.0f, grid.getY() + (float) (top - n.pitch) * rh + 2.0f,
                 std::max(4.0f, (float) n.length * cw - 3.0f), rh - 4.0f };
    }
    int noteAt(juce::Point<float> p) const {
        for (int i = (int) pat.notes.size() - 1; i >= 0; --i)
            if (noteRect(pat.notes[(size_t) i]).expanded(1.0f, 0.0f).contains(p)) return i;
        return -1;
    }
    int stepAt(float x) const {
        const auto grid = gridArea();
        return juce::jlimit(0, pat.steps() - 1, (int) std::floor((x - grid.getX()) / grid.getWidth() * (float) pat.steps()));
    }
    int pitchAt(float y) const {
        const auto grid = gridArea();
        return juce::jlimit(bottom(), top, top - (int) std::floor((y - grid.getY()) / rowH()));
    }
    void fitRange() {
        int lo = 127, hi = 0;
        for (auto& n : pat.notes) { lo = std::min(lo, n.pitch); hi = std::max(hi, n.pitch); }
        if (lo > hi) { lo = 28; hi = 52; }
        const int mid = (lo + hi) / 2;
        top = juce::jlimit(20 + rows - 1, 72, std::max(hi + 2, mid + rows / 2));
        if (lo < bottom()) top = lo + rows - 1 - 2 > hi ? lo + rows - 3 : top;
    }
    void sortKeepSelection() {
        Note keep;
        const bool has = selected >= 0 && selected < (int) pat.notes.size();
        if (has) keep = pat.notes[(size_t) selected];
        std::stable_sort(pat.notes.begin(), pat.notes.end(), [](auto& a, auto& b) { return a.start < b.start; });
        if (has)
            for (int i = 0; i < (int) pat.notes.size(); ++i) {
                auto& n = pat.notes[(size_t) i];
                if (n.start == keep.start && n.pitch == keep.pitch && n.length == keep.length) { selected = i; break; }
            }
    }
    void commit() {
        proc.setPattern(pat);
        // keep our order/selection (setPattern sorts its own copy)
        repaint();
    }

    Processor& proc;
    Pattern pat;
    int rows = 19, top = 48;
    int selected = -1;
    Mode mode = none;
    Note dragOrig;
    int dragStartStep = 0, dragStartPitch = 0;
    double playhead = -1.0;
};

// ===========================================================================
// Root: the whole UI at design size (scaled by the editor)
// ===========================================================================
class Root final : public juce::Component, private juce::Timer {
public:
    explicit Root(Processor& p) : proc(p), roll(p),
        playBtn(Btn::play), varBtn(Btn::variation), genBtn(Btn::generate), midiBtn(Btn::dragMidi), wavBtn(Btn::dragWav) {
        auto& st = proc.state;

        styles.big = true;
        for (int s = 0; s < numStyles; ++s) styles.items.add(styleDisplay(s));
        styles.get = [this] { return ip("style"); };
        styles.set = [this](int i) {
            setParam("style", (float) i);
            proc.applyStyleDefaults();
            proc.regenerateKeepSeed();
            refreshCombos();
            roll.refreshFromProcessor();
            repaint();
        };
        addAndMakeVisible(styles);

        for (auto* c : { &keyBox, &scaleBox, &grooveBox, &soundBox }) addAndMakeVisible(*c);
        keyBox.addItem(T("PADRÃO"), 1);
        for (int k = 0; k < 12; ++k) keyBox.addItem(noteName(k), k + 2);
        scaleBox.addItem(T("PADRÃO"), 1);
        for (int i = 0; i < (int) scales().size(); ++i) scaleBox.addItem(scaleDisplay(i), i + 2);
        keyBox.onChange = [this] { setParam("key", (float) (keyBox.getSelectedId() - 1)); regen(); };
        scaleBox.onChange = [this] { setParam("scale", (float) (scaleBox.getSelectedId() - 1)); regen(); };
        grooveBox.onChange = [this] { setParam("groove", (float) (grooveBox.getSelectedId() - 1)); regen(); };
        soundBox.onChange = [this] { setParam("sound", (float) (soundBox.getSelectedId() - 1)); repaint(); };
        refreshCombos();

        addAndMakeVisible(roll);

        const char* knobIds[] { "sub", "drive", "movement", "punch" };
        for (int i = 0; i < 4; ++i) {
            auto& k = knobs[(size_t) i];
            k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            k.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
            k.setDoubleClickReturnValue(true, 0.5);
            addAndMakeVisible(k);
            knobAtt[(size_t) i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(st, knobIds[i], k);
        }

        const char* fxIds[] { "lp", "rev", "dly", "sc" };
        const char* fxOn[] { "lpOn", "revOn", "dlyOn", "scOn" };
        for (int i = 0; i < 4; ++i) {
            auto& s = fxSliders[(size_t) i];
            s.setSliderStyle(juce::Slider::LinearHorizontal);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            addAndMakeVisible(s);
            fxAtt[(size_t) i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(st, fxIds[i], s);
            addAndMakeVisible(fxToggles[(size_t) i]);
            fxTogAtt[(size_t) i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(st, fxOn[i], fxToggles[(size_t) i]);
            fxToggles[(size_t) i].onStateChange = [this] { updateEnabled(); };
        }
        sweepBtn.setClickingTogglesState(true);
        sweepBtn.setButtonText("SWEEP");
        sweepBtn.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible(sweepBtn);
        sweepAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(st, "sweep", sweepBtn);
        sweepBtn.onStateChange = [this] { repaint(); };

        bars.items = { "2", "4", "8" };
        bars.get = [this] { return ip("bars"); };
        bars.set = [this](int i) { setParam("bars", (float) i); regen(); };
        density.items = { "LOW", "MID", "HIGH" };
        density.get = [this] { return ip("density"); };
        density.set = [this](int i) { setParam("density", (float) i); regen(); };
        addAndMakeVisible(bars);
        addAndMakeVisible(density);
        bpm.setSliderStyle(juce::Slider::LinearHorizontal);
        bpm.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(bpm);
        bpmAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(st, "bpm", bpm);
        bpm.onValueChange = [this] { repaint(); };

        for (auto* b : { &playBtn, &varBtn, &genBtn, &midiBtn, &wavBtn }) addAndMakeVisible(*b);
        playBtn.onClick = [this] { proc.setPreviewing(!proc.isPreviewing()); playBtn.active = proc.isPreviewing(); playBtn.repaint(); };
        varBtn.onClick = [this] { proc.generate(true); roll.refreshFromProcessor(); repaint(); };
        genBtn.onClick = [this] { proc.generate(false); roll.refreshFromProcessor(); repaint(); };
        midiBtn.onDragOut = [this] { dragFile(proc.writeMidiFile()); };
        wavBtn.onDragOut = [this] { dragFile(proc.writeWavFile()); };
        midiBtn.onClick = [this] { saveAs(proc.writeMidiFile(), "*.mid"); };
        wavBtn.onClick = [this] { saveAs(proc.writeWavFile(), "*.wav"); };

        updateEnabled();
        setSize(Editor::designW, Editor::designH);
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override {
        auto r = getLocalBounds().toFloat();
        g.fillAll(col::bg);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0x331f3a12), r.getRight() - 120.0f, -60.0f,
                                               juce::Colour(0x001f3a12), r.getRight() - 700.0f, 360.0f, true));
        g.fillRect(r);

        // header
        auto logo = juce::Rectangle<float>(24.0f, 20.0f, 42.0f, 42.0f);
        g.setGradientFill(juce::ColourGradient(col::limeA, logo.getX(), logo.getY(), col::limeB, logo.getRight(), logo.getBottom(), false));
        g.fillRoundedRectangle(logo, 10.0f);
        juce::Path wave;
        const float lx = logo.getX(), ly = logo.getCentreY();
        wave.startNewSubPath(lx + 7.0f, ly);
        wave.lineTo(lx + 13.0f, ly); wave.lineTo(lx + 16.5f, ly - 10.0f); wave.lineTo(lx + 22.0f, ly + 11.0f);
        wave.lineTo(lx + 27.0f, ly - 5.0f); wave.lineTo(lx + 30.0f, ly); wave.lineTo(lx + 35.0f, ly);
        g.setColour(col::dark);
        g.strokePath(wave, juce::PathStrokeType(2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setFont(spaceBold(26.0f).withExtraKerningFactor(0.02f));
        g.setColour(col::text);
        g.drawText("BASS", juce::Rectangle<float>(80.0f, 20.0f, 80.0f, 42.0f), juce::Justification::centredLeft, false);
        g.setColour(col::limeA);
        g.drawText("CODE", juce::Rectangle<float>(80.0f + juce::GlyphArrangement::getStringWidth(spaceBold(26.0f).withExtraKerningFactor(0.02f), "BASS "), 20.0f, 100.0f, 42.0f), juce::Justification::centredLeft, false);

        // status pill
        const auto pill = pillText();
        auto pf = interBold(11.5f).withExtraKerningFactor(0.12f);
        const float pw = juce::GlyphArrangement::getStringWidth(pf, pill) + 40.0f;
        auto pr = juce::Rectangle<float>((float) getWidth() - 24.0f - pw, 23.0f, pw, 36.0f);
        g.setColour(col::limeA.withAlpha(0.65f));
        g.drawRoundedRectangle(pr, 18.0f, 1.0f);
        g.setColour(col::limeA);
        g.setFont(pf);
        g.drawText(pill, pr, juce::Justification::centred, false);
        g.setColour(col::line);
        g.fillRect(24.0f, 78.0f, (float) getWidth() - 48.0f, 1.0f);

        label(g, "STYLE", 24.0f, 90.0f);
        const float colW = ((float) getWidth() - 48.0f - 36.0f) / 4.0f;
        const char* dl[] { "TOM", "ESCALA", "GROOVE", "SOUND" };
        for (int i = 0; i < 4; ++i) label(g, dl[i], 24.0f + (float) i * (colW + 12.0f), 162.0f);

        g.setColour(col::dim);
        g.setFont(interRegular(12.0f));
        g.drawText(T("Clique numa nota e use ↑↓ (Shift = oitava) ou arraste pra mudar a nota do bass  ·  clique no grid cria nota  ·  puxe a ponta pra alongar  ·  duplo clique apaga  ·  KICK é só guia do ducking"),
                   juce::Rectangle<float>(24.0f, 558.0f, (float) getWidth() - 48.0f, 18.0f), juce::Justification::centredLeft, true);

        // SOM card
        card(g, somCard);
        label(g, "SOM", somCard.getX() + 18.0f, somCard.getY() + 14.0f);
        const char* kn[] { "SUB", "DRIVE", "MOVIMENTO", "PUNCH" };
        const char* ks[] { "peso", "sujeira", "filtro", "ataque" };
        for (int i = 0; i < 4; ++i) {
            auto b = knobs[(size_t) i].getBounds().toFloat();
            g.setColour(col::text);
            g.setFont(interBold(12.0f).withExtraKerningFactor(0.12f));
            g.drawText(kn[i], juce::Rectangle<float>(b.getX() - 20.0f, b.getBottom() + 4.0f, b.getWidth() + 40.0f, 16.0f), juce::Justification::centred, false);
            g.setColour(col::dim);
            g.setFont(interRegular(10.5f));
            g.drawText(ks[i], juce::Rectangle<float>(b.getX() - 20.0f, b.getBottom() + 20.0f, b.getWidth() + 40.0f, 14.0f), juce::Justification::centred, false);
        }

        // FX card
        card(g, fxCard);
        label(g, T("FX  ·  VÃO JUNTO NO WAV"), fxCard.getX() + 18.0f, fxCard.getY() + 14.0f);
        const char* fn[] { "LOWPASS", "REVERB", "DELAY", "SIDECHAIN" };
        for (int i = 0; i < 4; ++i) {
            auto s = fxSliders[(size_t) i].getBounds().toFloat();
            g.setColour(fxToggles[(size_t) i].getToggleState() ? col::text : col::dim);
            g.setFont(interBold(12.0f).withExtraKerningFactor(0.1f));
            g.drawText(fn[i], juce::Rectangle<float>(fxCard.getX() + 18.0f, s.getY(), 90.0f, s.getHeight()), juce::Justification::centredLeft, false);
            if (i == 0) continue;
            juce::String v = i == 2 ? "1/8 D" : juce::String((int) std::lround(fxSliders[(size_t) i].getValue() * 100.0)) + "%";
            g.setColour(juce::Colour(0xffd6f7b0));
            g.setFont(interBold(11.5f));
            g.drawText(v, juce::Rectangle<float>(fxCard.getRight() - 128.0f, s.getY(), 68.0f, s.getHeight()), juce::Justification::centredRight, false);
        }
        g.setColour(col::dim);
        g.setFont(interRegular(10.5f));
        g.drawText(T("SWEEP = o lowpass abre ao longo do loop, pronto pra build ou drop"),
                   juce::Rectangle<float>(fxCard.getX() + 18.0f, fxCard.getBottom() - 26.0f, fxCard.getWidth() - 30.0f, 14.0f), juce::Justification::centredLeft, false);

        // side cards
        for (int i = 0; i < 3; ++i) card(g, sideCards[(size_t) i]);
        label(g, "BARS", sideCards[0].getX() + 14.0f, sideCards[0].getY() + 9.0f);
        label(g, "DENSITY", sideCards[1].getX() + 14.0f, sideCards[1].getY() + 9.0f);
        label(g, "BPM", sideCards[2].getX() + 14.0f, sideCards[2].getY() + 9.0f);
        g.setColour(juce::Colour(0xffd6f7b0));
        g.setFont(interBold(11.5f));
        g.drawText(juce::String((int) bpm.getValue()), sideCards[2].withTrimmedRight(14.0f).withHeight(32.0f), juce::Justification::centredRight, false);
    }

    void paintOverChildren(juce::Graphics& g) override {
        // SWEEP chip label next to the lowpass slider
        auto b = sweepBtn.getBounds().toFloat();
        const bool on = sweepBtn.getToggleState();
        g.setColour(on ? col::limeA : col::dim);
        g.setFont(interBold(11.5f).withExtraKerningFactor(0.08f));
        g.drawText(on ? T("SWEEP ↗") : T("FIXO"), b, juce::Justification::centredRight, false);
    }

    void resized() override {
        const int W = getWidth();
        styles.setBounds(24, 106, W - 48, 42);
        const int colW = (W - 48 - 36) / 4;
        juce::ComboBox* boxes[] { &keyBox, &scaleBox, &grooveBox, &soundBox };
        for (int i = 0; i < 4; ++i) boxes[i]->setBounds(24 + i * (colW + 12), 178, colW, 42);
        roll.setBounds(24, 234, W - 48, 318);

        const float midY = 586.0f, midH = 176.0f;
        somCard = { 24.0f, midY, 500.0f, midH };
        fxCard = { 536.0f, midY, 400.0f, midH };
        const float sx = 948.0f, sw = (float) W - 24.0f - sx;
        const float sh = (midH - 16.0f) / 3.0f;
        for (int i = 0; i < 3; ++i) sideCards[(size_t) i] = { sx, midY + (float) i * (sh + 8.0f), sw, sh };

        for (int i = 0; i < 4; ++i) {
            const float cx = somCard.getX() + 30.0f + (somCard.getWidth() - 60.0f) * ((float) i + 0.5f) / 4.0f;
            knobs[(size_t) i].setBounds(juce::Rectangle<int>(84, 84).withCentre({ (int) cx, (int) midY + 82 }));
        }
        for (int i = 0; i < 4; ++i) {
            const int y = (int) midY + 40 + i * 30;
            fxSliders[(size_t) i].setBounds((int) fxCard.getX() + 112, y, 168, 24);
            fxToggles[(size_t) i].setBounds((int) fxCard.getRight() - 54, y, 40, 24);
        }
        sweepBtn.setBounds((int) fxCard.getRight() - 128, (int) midY + 40, 68, 24);
        for (int i = 0; i < 2; ++i) {
            auto c = sideCards[(size_t) i];
            (i == 0 ? bars : density).setBounds((int) c.getX() + 92, (int) c.getY() + 8, (int) c.getWidth() - 104, (int) c.getHeight() - 16);
        }
        bpm.setBounds((int) sideCards[2].getX() + 92, (int) sideCards[2].getY() + 8, (int) sideCards[2].getWidth() - 150, (int) sideCards[2].getHeight() - 16);

        const int by = 776, bh = 56, gap = 10;
        const int unit = (W - 48 - gap * 4) * 10 / 61;   // 4 x 1.0 + 2.1
        int x = 24;
        playBtn.setBounds(x, by, unit, bh); x += unit + gap;
        varBtn.setBounds(x, by, unit, bh); x += unit + gap;
        const int big = W - 48 - gap * 4 - unit * 4;
        genBtn.setBounds(x, by, big, bh); x += big + gap;
        midiBtn.setBounds(x, by, unit, bh); x += unit + gap;
        wavBtn.setBounds(x, by, W - 24 - x, bh);
    }

private:
    void timerCallback() override {
        roll.setPlayhead(proc.getPlayPositionSteps());
        if (playBtn.active != proc.isPreviewing()) { playBtn.active = proc.isPreviewing(); playBtn.repaint(); }
    }

    int ip(const char* pid) const { return (int) std::lround(proc.state.getRawParameterValue(pid)->load()); }
    void setParam(const char* pid, float plain) {
        if (auto* p = proc.state.getParameter(pid)) p->setValueNotifyingHost(p->convertTo0to1(plain));
    }
    void regen() { proc.regenerateKeepSeed(); roll.refreshFromProcessor(); repaint(); }

    void refreshCombos() {
        const auto& sd = style(ip("style"));
        keyBox.setSelectedId(ip("key") + 1, juce::dontSendNotification);
        scaleBox.setSelectedId(ip("scale") + 1, juce::dontSendNotification);
        grooveBox.clear(juce::dontSendNotification);
        for (int i = 0; i < 3; ++i) grooveBox.addItem(sd.grooves[(size_t) i], i + 1);
        grooveBox.setSelectedId(ip("groove") + 1, juce::dontSendNotification);
        soundBox.clear(juce::dontSendNotification);
        for (int i = 0; i < (int) sd.sounds.size(); ++i) soundBox.addItem(presets()[(size_t) sd.sounds[(size_t) i]].name, i + 1);
        soundBox.setSelectedId(ip("sound") + 1, juce::dontSendNotification);
    }

    void updateEnabled() {
        for (int i = 0; i < 4; ++i) fxSliders[(size_t) i].setEnabled(fxToggles[(size_t) i].getToggleState());
        sweepBtn.setEnabled(fxToggles[0].getToggleState());
        repaint();
    }

    juce::String pillText() const {
        const auto& pat = proc.getPattern();
        juce::String key = ip("key") == 0 ? T("PADRÃO → ") + noteName(pat.key) : juce::String(noteName(pat.key));
        return styleDisplay(ip("style")) + T("  ·  ") + key + " " + scaleDisplay(pat.scale) + T("  ·  ")
               + style(ip("style")).grooves[(size_t) juce::jlimit(0, 2, ip("groove"))] + T("  ·  ")
               + juce::String(pat.bars) + " BARS" + T("  ·  ") + juce::String(proc.currentBpm()) + " BPM";
    }

    void dragFile(const juce::File& f) {
        if (f.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles({ f.getFullPathName() }, false, this);
    }

    void saveAs(const juce::File& f, const juce::String& pattern) {
        if (!f.existsAsFile()) return;
        chooser = std::make_unique<juce::FileChooser>("Salvar", juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile(f.getFileName()), pattern);
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                 | juce::FileBrowserComponent::warnAboutOverwriting,
                             [f](const juce::FileChooser& fc) {
                                 auto dest = fc.getResult();
                                 if (dest != juce::File()) f.copyFileTo(dest);
                             });
    }

    Processor& proc;
    Segments styles, bars, density;
    juce::ComboBox keyBox, scaleBox, grooveBox, soundBox;
    PianoRoll roll;
    std::array<juce::Slider, 4> knobs, fxSliders;
    std::array<juce::ToggleButton, 4> fxToggles;
    juce::TextButton sweepBtn;
    juce::Slider bpm;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> knobAtt, fxAtt;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 4> fxTogAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sweepAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAtt;
    Btn playBtn, varBtn, genBtn, midiBtn, wavBtn;
    juce::Rectangle<float> somCard, fxCard;
    std::array<juce::Rectangle<float>, 3> sideCards;
    std::unique_ptr<juce::FileChooser> chooser;
};


// ===========================================================================
// Editor
// ===========================================================================
Editor::Editor(Processor& p) : juce::AudioProcessorEditor(p) {
    look = std::make_unique<Look>();
    setLookAndFeel(look.get());
    root = std::make_unique<Root>(p);
    addAndMakeVisible(*root);
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio((double) designW / designH);
    setResizeLimits(designW * 6 / 10, designH * 6 / 10, designW * 2, designH * 2);
    setSize(designW, designH);
}

Editor::~Editor() {
    root.reset();
    setLookAndFeel(nullptr);
}

void Editor::paint(juce::Graphics& g) { g.fillAll(col::bg); }

void Editor::resized() {
    const float s = (float) getWidth() / (float) designW;
    root->setTransform(juce::AffineTransform::scale(s));
    root->setBounds(0, 0, designW, designH);
}

} // namespace bass
