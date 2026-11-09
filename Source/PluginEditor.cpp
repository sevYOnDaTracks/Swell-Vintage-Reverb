#include "PluginEditor.h"
#include "BinaryData.h"

namespace {
const juce::Colour ink { 0xfff2f2f2 }, dim { 0xff9c9c9c }, muted { 0xff6a6a6a }, well { 0xff0f0f10 }, lineDark { 0xff0b0b0c };
constexpr float X0 = 34; // left edge of the metal face (after the wooden cheek)

juce::Font uiFont(float size, bool bold = false, float kerning = 0) {
#if JUCE_WINDOWS
    const char* face = "Segoe UI";
#else
    const char* face = "Helvetica Neue";
#endif
    return juce::Font(juce::FontOptions(face, size, bold ? juce::Font::bold : juce::Font::plain)).withExtraKerningFactor(kerning);
}
void drawLabel(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size, juce::Colour c, juce::Justification j = juce::Justification::centred, float kerning = 0, bool bold = false) {
    g.setColour(c); g.setFont(uiFont(size, bold, kerning)); g.drawText(s, r, j, false);
}
juce::String fixed(float v, int decimals) { return juce::String(v, decimals); }
juce::String hz(float v) { return v >= 1000 ? fixed(v / 1000, 1) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz"; }
juce::String db(float v) { return v <= -59.9f ? juce::String("-inf") : fixed(v, 1); }
juce::String mixText(float m) {
    m /= 100;
    float d = juce::Decibels::gainToDecibels(std::min(1.f, 2 * (1 - m)), -60.f), w = juce::Decibels::gainToDecibels(std::min(1.f, 2 * m), -60.f);
    return db(d) + " / " + db(w) + " dB";
}
void screw(juce::Graphics& g, float cx, float cy) {
    juce::ColourGradient grad(juce::Colour(0xff8d8d8d), cx - 2, cy - 2, juce::Colour(0xff3a3a3a), cx + 4, cy + 4, true);
    g.setColour(juce::Colours::black.withAlpha(.6f)); g.fillEllipse(cx - 5.5f, cy - 4.5f, 11, 11);
    g.setGradientFill(grad); g.fillEllipse(cx - 5.5f, cy - 5.5f, 11, 11);
    g.setColour(juce::Colour(0xff222222));
    juce::Path slot; slot.addRectangle(-4.f, -1.f, 8.f, 2.f);
    g.fillPath(slot, juce::AffineTransform::rotation(.6f).translated(cx, cy));
}
}

//============================================================================== Look
ReverbLook::ReverbLook() {
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1a1a1b));
    setColour(juce::PopupMenu::textColourId, ink);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ink);
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xff111111));
    setColour(juce::PopupMenu::headerTextColourId, dim);
}

void ReverbLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s) {
    const bool small = s.getProperties()["small"];
    const bool bipolar = s.getProperties()["bipolar"];
    auto r = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h);
    float size = std::min(r.getWidth(), r.getHeight());
    auto c = r.getCentre();
    float arcR = size / 2 - 3, bodyR = arcR - (small ? 5.f : 7.f), thick = small ? 3.f : 4.f;
    float angle = start + pos * (end - start), from = bipolar ? (start + end) / 2 : start;

    juce::Path track; track.addCentredArc(c.x, c.y, arcR, arcR, 0, start, end, true);
    g.setColour(well); g.strokePath(track, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (std::abs(angle - from) > .01f) {
        juce::Path arc; arc.addCentredArc(c.x, c.y, arcR, arcR, 0, std::min(from, angle), std::max(from, angle), true);
        g.setColour(ink.withAlpha(.13f)); g.strokePath(arc, juce::PathStrokeType(thick + 5, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(ink); g.strokePath(arc, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour(juce::Colours::black.withAlpha(.6f)); g.fillEllipse(c.x - bodyR, c.y - bodyR + 2.5f, bodyR * 2, bodyR * 2);
    juce::ColourGradient body(juce::Colour(0xff4c4c4d), c.x - bodyR * .3f, c.y - bodyR * .55f, juce::Colour(0xff0c0c0c), c.x + bodyR * .6f, c.y + bodyR, true);
    body.addColour(.55, juce::Colour(0xff212122));
    g.setGradientFill(body); g.fillEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2);
    g.setColour(juce::Colour(0xff050505)); g.drawEllipse(c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2, 1);
    auto at = [&](float rr) { return juce::Point<float>(c.x + rr * std::sin(angle), c.y - rr * std::cos(angle)); };
    g.setColour(ink); g.drawLine(juce::Line<float>(at(bodyR * .35f), at(bodyR - 3)), small ? 2.2f : 3.f);
}

void ReverbLook::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) {
    float cy = (float) y + (float) h / 2;
    auto track = juce::Rectangle<float>((float) x, cy - 2, (float) w, 4);
    g.setColour(well); g.fillRoundedRectangle(track, 2);
    g.setColour(ink); g.fillRoundedRectangle(track.withRight(pos), 2);
    g.setColour(juce::Colours::black.withAlpha(.6f)); g.fillEllipse(pos - 7, cy - 6, 14, 14);
    juce::ColourGradient thumb(juce::Colours::white, pos - 3, cy - 4, juce::Colour(0xff9a9a9a), pos + 5, cy + 6, true);
    g.setGradientFill(thumb); g.fillEllipse(pos - 7, cy - 7, 14, 14);
}

//============================================================================== Knob
Knob::Knob(ReverbProcessor& p, const char* id, juce::String l, std::function<juce::String(float)> f, bool sm) : label(std::move(l)), format(std::move(f)), small(sm) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity(220);
    slider.setVelocityModeParameters(.6, 1, .05, true, juce::ModifierKeys::shiftModifier);
    slider.getProperties().set("small", small);
    slider.getProperties().set("bipolar", juce::String(id) == "gain");
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible(slider);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state, id, slider);
    if (auto* param = p.state.getParameter(id)) slider.setDoubleClickReturnValue(true, param->convertFrom0to1(param->getDefaultValue()));
}
void Knob::resized() {
    auto r = getLocalBounds().reduced(0, 0);
    r.removeFromTop(small ? 18 : 22); r.removeFromBottom(small ? 18 : 20);
    int s = std::min(r.getWidth(), r.getHeight());
    slider.setBounds(r.withSizeKeepingCentre(s, s));
}
void Knob::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    drawLabel(g, label, r.removeFromTop(small ? 18.f : 22.f), small ? 12.5f : 15.f, ink);
    drawLabel(g, format((float) slider.getValue()), r.removeFromBottom(small ? 18.f : 20.f), small ? 12 : 13, ink.withAlpha(.92f));
}

//============================================================================== HSlider
HSlider::HSlider(ReverbProcessor& p, const char* id, juce::String l, std::function<juce::String(float)> f) : label(std::move(l)), format(std::move(f)) {
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible(slider);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state, id, slider);
    if (auto* param = p.state.getParameter(id)) slider.setDoubleClickReturnValue(true, param->convertFrom0to1(param->getDefaultValue()));
}
void HSlider::resized() { slider.setBounds(getLocalBounds().withTrimmedTop(18).reduced(0, 2)); }
void HSlider::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().removeFromTop(16);
    drawLabel(g, label, r, 11.5f, dim, juce::Justification::centredLeft, .12f);
    drawLabel(g, format((float) slider.getValue()), r, 11.5f, ink, juce::Justification::centredRight);
}

//============================================================================== PresetBox
void PresetBox::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(.5f);
    g.setColour(juce::Colours::white.withAlpha(.05f)); g.drawRoundedRectangle(r.translated(0, 1), 4, 1);
    g.setColour(juce::Colour(0xff0c0c0d)); g.fillRoundedRectangle(r, 4);
    g.setColour(juce::Colours::black); g.drawRoundedRectangle(r, 4, 1);
    juce::ColourGradient inner(juce::Colours::black.withAlpha(.7f), 0, r.getY(), juce::Colours::transparentBlack, 0, r.getY() + 7, false);
    g.setGradientFill(inner); g.fillRoundedRectangle(r.reduced(1), 4);
    drawLabel(g, juce::CharPointer_UTF8("\xe2\x80\xb9"), r.withWidth(34), 20, ink.withAlpha(.8f));
    drawLabel(g, juce::CharPointer_UTF8("\xe2\x80\xba"), r.withTrimmedLeft(r.getWidth() - 34), 20, ink.withAlpha(.8f));
    drawLabel(g, label, r.reduced(34, 0), 15.5f, ink);
}
void PresetBox::mouseDown(const juce::MouseEvent& e) {
    if (e.x < 34) { if (onPrev) onPrev(); }
    else if (e.x > getWidth() - 34) { if (onNext) onNext(); }
    else if (onMenu) onMenu();
}

//============================================================================== Meters
void Meters::paint(juce::Graphics& g) {
    static const char* names[5] { "INPUT", "OUTPUT", "EARLY", "LATE", "VLF" };
    constexpr int leds = 20;
    const float ledW = 9, gap = 4, x0 = 74, rowH = 22;
    auto ledDb = [](int k) { return -60.f + (float) k * 3.15f; };
    float y = 4;
    for (int row = 0; row < 5; ++row) {
        if (row == 2) {
            const float marks[7] { -60, -48, -36, -24, -12, -6, 0 };
            for (float m : marks) {
                float k = (m + 60) / 3.15f;
                drawLabel(g, juce::String((int) m), { x0 + k * (ledW + gap) - 10, y, ledW + 20, 14 }, 10, muted);
            }
            y += 16;
        }
        drawLabel(g, names[row], { 0, y, x0 - 10, rowH }, 11, dim, juce::Justification::centredRight, .12f);
        for (int k = 0; k < leds; ++k) {
            auto led = juce::Rectangle<float>(x0 + (float) k * (ledW + gap), y + 6, ledW, 10);
            bool on = level[(size_t) row] >= ledDb(k);
            if (on) {
                g.setColour(ink.withAlpha(.18f)); g.fillRoundedRectangle(led.expanded(2), 3);
                g.setColour(ink); g.fillRoundedRectangle(led, 1.5f);
            } else {
                g.setColour(juce::Colour(0xff1d1d1e)); g.fillRoundedRectangle(led, 1.5f);
                g.setColour(juce::Colours::black.withAlpha(.6f)); g.drawRoundedRectangle(led, 1.5f, 1);
            }
        }
        y += rowH;
    }
}

//============================================================================== Icons and tabs
void IconButton::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(2);
    float s = r.getWidth() / 24.f;
    juce::Path p;
    auto P = [&](float x, float y) { return juce::Point<float>(r.getX() + x * s, r.getY() + y * s); };
    switch (kind) {
        case Save:
            p.startNewSubPath(P(5, 4)); p.lineTo(P(16, 4)); p.lineTo(P(19, 7)); p.lineTo(P(19, 20)); p.lineTo(P(5, 20)); p.closeSubPath();
            p.startNewSubPath(P(8, 4)); p.lineTo(P(8, 9)); p.lineTo(P(15, 9)); p.lineTo(P(15, 4));
            p.startNewSubPath(P(8, 20)); p.lineTo(P(8, 14)); p.lineTo(P(16, 14)); p.lineTo(P(16, 20));
            break;
        case Folder:
            p.startNewSubPath(P(3, 7)); p.lineTo(P(9, 7)); p.lineTo(P(11, 9)); p.lineTo(P(21, 9)); p.lineTo(P(21, 19)); p.lineTo(P(3, 19)); p.closeSubPath();
            break;
        case Settings:
            p.addEllipse(r.getCentreX() - 3 * s, r.getCentreY() - 3 * s, 6 * s, 6 * s);
            for (int i = 0; i < 8; ++i) {
                float a = (float) i * juce::MathConstants<float>::pi / 4, c = std::cos(a), sn = std::sin(a);
                p.startNewSubPath(P(12 + 6 * c, 12 + 6 * sn)); p.lineTo(P(12 + 9.5f * c, 12 + 9.5f * sn));
            }
            break;
        case Help:
            p.addEllipse(P(3, 3).x, P(3, 3).y, 18 * s, 18 * s);
            p.startNewSubPath(P(9.5f, 9.5f)); p.cubicTo(P(9.5f, 6.5f), P(14.5f, 6.5f), P(14.5f, 9.5f)); p.cubicTo(P(14.5f, 11.5f), P(12, 11.5f), P(12, 14));
            p.addEllipse(P(11.3f, 16.6f).x, P(11.3f, 16.6f).y, 1.4f * s, 1.4f * s);
            break;
    }
    g.setColour(isMouseOver() ? ink : dim);
    g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void TabLabel::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    drawLabel(g, name, r, 12.5f, active ? ink : muted, juce::Justification::centredLeft, .2f);
    if (active) {
        float w = uiFont(12.5f, false, .2f).getStringWidthFloat(name);
        g.setColour(ink); g.fillRect(0.f, r.getBottom() - 2, w, 2.f);
    }
}

//============================================================================== EQ
namespace { const char* eqF[4] { "eqLowFreq", "eqMid1Freq", "eqMid2Freq", "eqHighFreq" }; const char* eqG[4] { "eqLowGain", "eqMid1Gain", "eqMid2Gain", "eqHighGain" }; }
EqDisplay::EqDisplay(ReverbProcessor& p) : proc(p) { for (int b = 0; b < 4; ++b) { p.state.addParameterListener(eqF[b], this); p.state.addParameterListener(eqG[b], this); } }
EqDisplay::~EqDisplay() { for (int b = 0; b < 4; ++b) { proc.state.removeParameterListener(eqF[b], this); proc.state.removeParameterListener(eqG[b], this); } }
float EqDisplay::xOf(float f) const { return 40 + std::log10(f / 20) / 3 * ((float) getWidth() - 60); }
float EqDisplay::hzOf(float x) const { return 20 * std::pow(10.f, (x - 40) / ((float) getWidth() - 60) * 3); }
float EqDisplay::yOf(float d) const { return (float) getHeight() / 2 - d * ((float) getHeight() / 2 - 14) / 15; }
float EqDisplay::dbOf(float y) const { return ((float) getHeight() / 2 - y) / ((float) getHeight() / 2 - 14) * 15; }
int EqDisplay::bandAt(juce::Point<float> p) const {
    for (int b = 0; b < 4; ++b)
        if (p.getDistanceFrom({ xOf(proc.value(eqF[b])), yOf(proc.value(eqG[b])) }) < 12) return b;
    return -1;
}
void EqDisplay::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(juce::Colours::black.withAlpha(.25f)); g.fillRoundedRectangle(r, 5);
    g.setColour(juce::Colours::black); g.drawRoundedRectangle(r.reduced(.5f), 5, 1);
    g.setColour(juce::Colour(0xff262627));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f }) g.drawVerticalLine((int) xOf(f), 6, r.getBottom() - 6);
    for (float d : { -12.f, -6.f, 0.f, 6.f, 12.f }) g.drawHorizontalLine((int) yOf(d), 40, r.getRight() - 20);
    for (float d : { -12.f, -6.f, 0.f, 6.f, 12.f }) drawLabel(g, juce::String((int) d), { 4, yOf(d) - 7, 30, 14 }, 10, muted, juce::Justification::centredRight);
    for (auto [l, f] : { std::pair<const char*, float> { "100", 100.f }, { "1k", 1000.f }, { "10k", 10000.f } }) drawLabel(g, l, { xOf(f) + 3, r.getBottom() - 18, 30, 14 }, 10, muted, juce::Justification::centredLeft);

    std::array<swell::Biquad, 4> eq;
    auto params = proc.currentParams();
    swell::designEq(eq, params, 48000);
    juce::Path curve;
    for (float x = 40; x <= r.getRight() - 20; x += 2) {
        float f = hzOf(x), d = 0;
        for (auto& b : eq) d += (float) b.magnitudeDb(f, 48000);
        float y = juce::jlimit(4.f, r.getBottom() - 4, yOf(d));
        if (x == 40) curve.startNewSubPath(x, y); else curve.lineTo(x, y);
    }
    juce::Path fill(curve); fill.lineTo(r.getRight() - 20, yOf(0)); fill.lineTo(40, yOf(0)); fill.closeSubPath();
    g.setColour(ink.withAlpha(.07f)); g.fillPath(fill);
    g.setColour(ink.withAlpha(.15f)); g.strokePath(curve, juce::PathStrokeType(6));
    g.setColour(ink); g.strokePath(curve, juce::PathStrokeType(2));
    static const char* names[4] { "LOW", "MID 1", "MID 2", "HIGH" };
    for (int b = 0; b < 4; ++b) {
        float x = xOf(params.eqFreq[(size_t) b]), y = yOf(params.eqGain[(size_t) b]);
        g.setColour(juce::Colours::black.withAlpha(.6f)); g.fillEllipse(x - 7, y - 6, 14, 14);
        g.setColour(b == dragging ? juce::Colours::white : ink); g.fillEllipse(x - 6, y - 6, 12, 12);
        float ty = y < r.getCentreY() ? y + 10 : y - 38;
        drawLabel(g, names[b], { x - 40, ty, 80, 14 }, 10.5f, dim, juce::Justification::centred, .12f);
        drawLabel(g, hz(params.eqFreq[(size_t) b]) + "  " + fixed(params.eqGain[(size_t) b], 1) + " dB", { x - 60, ty + 13, 120, 14 }, 10.5f, ink);
    }
}
void EqDisplay::mouseDown(const juce::MouseEvent& e) {
    dragging = bandAt(e.position);
    if (dragging >= 0) {
        proc.state.getParameter(eqF[dragging])->beginChangeGesture();
        proc.state.getParameter(eqG[dragging])->beginChangeGesture();
    }
    repaint();
}
void EqDisplay::mouseDrag(const juce::MouseEvent& e) {
    if (dragging < 0) return;
    auto* f = proc.state.getParameter(eqF[dragging]);
    auto* gn = proc.state.getParameter(eqG[dragging]);
    f->setValueNotifyingHost(f->convertTo0to1(f->getNormalisableRange().snapToLegalValue(hzOf(e.position.x))));
    gn->setValueNotifyingHost(gn->convertTo0to1(juce::jlimit(-15.f, 15.f, dbOf(e.position.y))));
    repaint();
}
void EqDisplay::mouseUp(const juce::MouseEvent&) {
    if (dragging >= 0) {
        proc.state.getParameter(eqF[dragging])->endChangeGesture();
        proc.state.getParameter(eqG[dragging])->endChangeGesture();
    }
    dragging = -1; repaint();
}
void EqDisplay::mouseDoubleClick(const juce::MouseEvent& e) { int b = bandAt(e.position); if (b >= 0) proc.setValue(eqG[b], 0); }

//============================================================================== Panel
Panel::Panel(ReverbProcessor& p) : proc(p), eq(p) {
    setLookAndFeel(&look);
    logo = juce::ImageCache::getFromMemory(BinaryData::swellmark_png, BinaryData::swellmark_pngSize);
    renderTextures();
    auto knob = [&](std::vector<std::unique_ptr<Knob>>& list, const char* id, const char* name, std::function<juce::String(float)> f, bool small) {
        list.push_back(std::make_unique<Knob>(p, id, name, std::move(f), small));
        addAndMakeVisible(*list.back());
    };
    knob(mainKnobs, "decay", "Decay Time", [](float v) { return fixed(v, 2) + " sec"; }, false);
    knob(mainKnobs, "mix", "Mix", mixText, false);
    knob(mainKnobs, "gain", "Gain", [](float v) { return fixed(v, 1) + " dB"; }, true);
    knob(advKnobs, "pattern", "Pattern", [](float v) { return "Set " + juce::String(juce::roundToInt(v)); }, true);
    knob(advKnobs, "predelay", "Time", [](float v) { return juce::String(juce::roundToInt(v)) + " ms"; }, true);
    knob(advKnobs, "delayTime", "Time", [](float v) { return juce::String(juce::roundToInt(v)) + " ms"; }, true);
    knob(advKnobs, "delayLevel", "Level", [](float v) { return v < .05f ? juce::String("Off") : juce::String(juce::roundToInt(v)) + " %"; }, true);
    knob(advKnobs, "rollRef", "Reflections", hz, true);
    knob(advKnobs, "rollRev", "Reverb", hz, true);
    knob(advKnobs, "lowFreq", "Low Freq", hz, true);
    knob(advKnobs, "lowMul", "Multiplier", [](float v) { return fixed(v, 2) + "x"; }, true);
    knob(advKnobs, "highFreq", "High Freq", hz, true);
    knob(advKnobs, "highMul", "Multiplier", [](float v) { return fixed(v, 2) + "x"; }, true);
    vlf = std::make_unique<HSlider>(p, "vlf", "VERY LOW FREQ REVERB", [](float v) { return fixed(v, 1) + " dB"; });
    earlyLate = std::make_unique<HSlider>(p, "earlyLate", "EARLY / LATE", [](float v) {
        if (v > .985f) return juce::String("Max Late");
        if (v < .015f) return juce::String("Max Early");
        return v < .5f ? "Early " + juce::String(juce::roundToInt((1 - 2 * v) * 100)) + "%" : "Late " + juce::String(juce::roundToInt((2 * v - 1) * 100)) + "%";
    });
    addAndMakeVisible(*vlf); addAndMakeVisible(*earlyLate);
    for (auto* c : std::initializer_list<juce::Component*> { &categoryBox, &programBox, &meters, &save, &folder, &settings, &help, &advTab, &eqTab }) addAndMakeVisible(c);
    addChildComponent(eq);
    categoryBox.onPrev = [this] { stepCategory(-1); }; categoryBox.onNext = [this] { stepCategory(1); }; categoryBox.onMenu = [this] { categoryMenu(); };
    programBox.onPrev = [this] { stepProgram(-1); }; programBox.onNext = [this] { stepProgram(1); }; programBox.onMenu = [this] { programMenu(); };
    advTab.name = "ADVANCED CONTROLS"; eqTab.name = "MASTER EQUALISER";
    advTab.onClick = [this] { showTab(false); }; eqTab.onClick = [this] { showTab(true); };
    save.onClick = [this] { savePreset(); }; folder.onClick = [this] { folderMenu(); }; settings.onClick = [this] { settingsMenu(); };
    help.onClick = [] {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::NoIcon, "Swell Reverb",
            "Vintage Reverb - version 0.1\nSwell Audio, by @sevyondatracks\n\nDouble-clic sur un bouton : valeur par defaut.\nShift + glisser : reglage fin.\nLes presets ne touchent jamais au Mix ni au Gain.");
    };
    showTab(false);
    refreshNames();
    setSize(W, H);
    startTimerHz(30);
}
Panel::~Panel() { setLookAndFeel(nullptr); }

void Panel::renderTextures() {
    auto makeWood = [](int seed) {
        juce::Image img(juce::Image::ARGB, 34, H, true);
        juce::Graphics g(img);
        juce::ColourGradient base(juce::Colour(0xff7a4524), 0, 0, juce::Colour(0xff683820), 0, (float) H, false);
        base.addColour(.3, juce::Colour(0xff5e3219)); base.addColour(.55, juce::Colour(0xff6f3d1f)); base.addColour(.8, juce::Colour(0xff4f2913));
        g.setGradientFill(base); g.fillAll();
        juce::Random r(seed);
        for (int i = 0; i < 70; ++i) {
            float x = r.nextFloat() * 34;
            juce::Path p; p.startNewSubPath(x, 0);
            for (int y = 16; y <= H + 16; y += 16) { x += (r.nextFloat() - .5f) * 1.3f; p.lineTo(x, (float) y); }
            bool light = r.nextFloat() < .25f;
            g.setColour((light ? juce::Colour(0xffffc890) : juce::Colour(0xff1e0c04)).withAlpha(light ? .05f + r.nextFloat() * .06f : .08f + r.nextFloat() * .22f));
            g.strokePath(p, juce::PathStrokeType(.4f + r.nextFloat() * 1.6f));
        }
        for (int k = 0; k < 2; ++k) {
            float cy = (float) H * (.25f + .45f * (float) k) + r.nextFloat() * 40;
            juce::ColourGradient knot(juce::Colour(0xff1e0c04).withAlpha(.35f), 16, cy, juce::Colours::transparentBlack, 34, cy, true);
            g.setGradientFill(knot); g.fillEllipse(-4, cy - 40, 42, 80);
        }
        juce::ColourGradient edge(juce::Colours::black.withAlpha(.45f), 0, 0, juce::Colours::black.withAlpha(.5f), 34, 0, false);
        edge.addColour(.18, juce::Colours::transparentBlack); edge.addColour(.5, juce::Colours::white.withAlpha(.06f)); edge.addColour(.82, juce::Colours::transparentBlack);
        g.setGradientFill(edge); g.fillAll();
        return img;
    };
    woodL = makeWood(7); woodR = makeWood(19);
    metal = juce::Image(juce::Image::RGB, 1000, H, true);
    juce::Graphics g(metal);
    juce::ColourGradient grad(juce::Colour(0xff2a2a2b), 0, 0, juce::Colour(0xff171718), 0, (float) H, false);
    grad.addColour(.6, juce::Colour(0xff202021));
    g.setGradientFill(grad); g.fillAll();
    juce::Random r(3);
    for (int x = 0; x < 1000; ++x) {
        g.setColour((r.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha(r.nextFloat() * .035f));
        g.drawVerticalLine(x, 0, (float) H);
    }
}

void Panel::resized() {
    mainKnobs[0]->setBounds((int) X0 + 30, 66, 132, 182);
    mainKnobs[1]->setBounds((int) X0 + 168, 78, 88, 136);
    mainKnobs[2]->setBounds((int) X0 + 258, 78, 66, 114);
    categoryBox.setBounds((int) X0 + 356, 100, 250, 40);
    programBox.setBounds((int) X0 + 356, 152, 250, 40);
    meters.setBounds((int) X0 + 636, 76, 340, 150);
    vlf->setBounds((int) X0 + 185, 244, 260, 40);
    earlyLate->setBounds((int) X0 + 515, 244, 260, 40);
    advTab.setBounds((int) X0 + 36, 296, 200, 34);
    eqTab.setBounds((int) X0 + 1000 - 36 - 150, 296, 150, 34);
    int x = (int) X0 + 1000 - 32;
    for (auto* b : { &help, &settings, &folder, &save }) { x -= 30; b->setBounds(x, 18, 22, 22); }
    // Advanced groups: weights 1,1,2,2,4 across the face.
    const int weights[5] { 1, 1, 2, 2, 4 }, counts[5] { 1, 1, 2, 2, 4 };
    float unit = (948 - 40) / 10.f, gx = X0 + 26;
    int k = 0;
    for (int gi = 0; gi < 5; ++gi) {
        float gw = unit * (float) weights[gi];
        float kw = (gw - 12) / (float) counts[gi];
        for (int j = 0; j < counts[gi]; ++j) advKnobs[(size_t) k++]->setBounds(juce::roundToInt(gx + 6 + kw * (float) j), 372, juce::roundToInt(kw), 118);
        gx += gw + 10;
    }
    eq.setBounds((int) X0 + 26, 342, 948, 154);
}

void Panel::paint(juce::Graphics& g) {
    g.drawImageAt(woodL, 0, 0); g.drawImageAt(woodR, (int) X0 + 1000, 0);
    g.drawImageAt(metal, (int) X0, 0);
    auto face = juce::Rectangle<float>(X0, 0, 1000, (float) H);
    // Top bar.
    g.setColour(juce::Colours::black.withAlpha(.18f)); g.fillRect(face.withHeight(58));
    g.setColour(lineDark); g.drawHorizontalLine(58, X0, X0 + 1000);
    g.setColour(juce::Colours::white.withAlpha(.04f)); g.drawHorizontalLine(59, X0, X0 + 1000);
    g.setColour(ink);
    g.drawImageWithin(logo, (int) X0 + 26, 13, 34, 32, juce::RectanglePlacement::centred, true);
    drawLabel(g, "swell", { X0 + 66, 9, 120, 28 }, 25, ink, juce::Justification::centredLeft);
    drawLabel(g, "AUDIO", { X0 + 68, 35, 120, 12 }, 8.5f, dim, juce::Justification::centredLeft, .55f);
    drawLabel(g, "VINTAGE REVERB", { X0 + 300, 18, 400, 22 }, 13, dim, juce::Justification::centred, .5f);
    // Tabs and advanced area.
    g.setColour(juce::Colours::black.withAlpha(.25f)); g.fillRect(X0, 296.f, 1000.f, 34.f);
    g.setColour(lineDark); g.drawHorizontalLine(296, X0, X0 + 1000);
    g.setColour(juce::Colours::white.withAlpha(.04f)); g.drawHorizontalLine(297, X0, X0 + 1000);
    g.setColour(juce::Colours::black.withAlpha(.28f)); g.fillRect(X0, 330.f, 1000.f, (float) H - 330);
    if (!showEq) {
        static const char* heads[5] { "REFLECTIONS", "PRE-DELAY", "DELAY", "ROLL-OFF", "FREQUENCY DEPENDENT DECAY TIME" };
        const int weights[5] { 1, 1, 2, 2, 4 };
        float unit = (948 - 40) / 10.f, gx = X0 + 26;
        for (int gi = 0; gi < 5; ++gi) {
            auto r = juce::Rectangle<float>(gx, 342, unit * (float) weights[gi], 154);
            g.setColour(juce::Colours::black.withAlpha(.18f)); g.fillRoundedRectangle(r, 5);
            g.setColour(juce::Colours::white.withAlpha(.04f)); g.drawRoundedRectangle(r.translated(0, 1), 5, 1);
            g.setColour(juce::Colours::black); g.drawRoundedRectangle(r.reduced(.5f), 5, 1);
            drawLabel(g, heads[gi], r.withHeight(26).reduced(4, 0), 11.5f, dim, juce::Justification::centred, .16f);
            g.setColour(juce::Colours::black); g.drawHorizontalLine((int) r.getY() + 26, r.getX() + 8, r.getRight() - 8);
            g.setColour(juce::Colours::white.withAlpha(.04f)); g.drawHorizontalLine((int) r.getY() + 27, r.getX() + 8, r.getRight() - 8);
            gx += r.getWidth() + 10;
        }
    }
    g.setColour(juce::Colours::black); g.drawRect(face, 1);
    for (auto pt : { juce::Point<float>(X0 + 12, 12), { X0 + 988, 12 }, { X0 + 12, (float) H - 12 }, { X0 + 988, (float) H - 12 } }) screw(g, pt.x, pt.y);
}

void Panel::timerCallback() {
    std::atomic<float>* src[5] { &proc.meterIn, &proc.meterOut, &proc.meterEarly, &proc.meterLate, &proc.meterVlf };
    bool changed = false;
    for (int i = 0; i < 5; ++i) {
        float d = juce::Decibels::gainToDecibels(src[i]->exchange(0.f), -100.f);
        float next = std::max(d, meters.level[(size_t) i] - 2.5f);
        if (std::abs(next - meters.level[(size_t) i]) > .01f) { meters.level[(size_t) i] = next; changed = true; }
    }
    if (changed) meters.repaint();
    refreshNames();
}

void Panel::refreshNames() {
    int prog = (int) proc.value("program");
    juce::String cat = categories[(size_t) categoryOf(prog)];
    juce::String name = proc.userPresetName.isNotEmpty() ? proc.userPresetName
                                                          : juce::String(positionInCategory(prog) + 1).paddedLeft('0', 2) + " " + programs[(size_t) prog].name;
    if (proc.userPresetName.isNotEmpty()) cat = "User";
    if (cat != categoryBox.label) { categoryBox.label = cat; categoryBox.repaint(); }
    if (name != programBox.label) { programBox.label = name; programBox.repaint(); }
}

void Panel::showTab(bool e) {
    showEq = e; advTab.active = !e; eqTab.active = e;
    for (auto& k : advKnobs) k->setVisible(!e);
    eq.setVisible(e);
    advTab.repaint(); eqTab.repaint(); repaint();
}

void Panel::stepCategory(int d) {
    int c = (categoryOf((int) proc.value("program")) + d + (int) categories.size()) % (int) categories.size();
    proc.loadProgram(programIndex(c, 0)); refreshNames();
}
void Panel::stepProgram(int d) {
    int prog = (int) proc.value("program"), c = categoryOf(prog), n = programsIn(c);
    proc.loadProgram(programIndex(c, (positionInCategory(prog) + d + n) % n)); refreshNames();
}
void Panel::categoryMenu() {
    juce::PopupMenu m; m.setLookAndFeel(&look);
    int current = categoryOf((int) proc.value("program"));
    for (int c = 0; c < (int) categories.size(); ++c) m.addItem(c + 1, categories[(size_t) c], true, c == current && proc.userPresetName.isEmpty());
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&categoryBox), [this](int r) { if (r > 0) { proc.loadProgram(programIndex(r - 1, 0)); refreshNames(); } });
}
void Panel::programMenu() {
    juce::PopupMenu m; m.setLookAndFeel(&look);
    int current = (int) proc.value("program");
    for (int c = 0; c < (int) categories.size(); ++c) {
        juce::PopupMenu sub;
        for (int n = 0; n < programsIn(c); ++n) { int i = programIndex(c, n); sub.addItem(i + 1, juce::String(n + 1).paddedLeft('0', 2) + " " + programs[(size_t) i].name, true, i == current && proc.userPresetName.isEmpty()); }
        m.addSubMenu(categories[(size_t) c], sub, true, nullptr, c == categoryOf(current));
    }
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&programBox), [this](int r) { if (r > 0) { proc.loadProgram(r - 1); refreshNames(); } });
}
void Panel::savePreset() {
    auto folderPath = ReverbProcessor::userPresetFolder(); folderPath.createDirectory();
    chooser = std::make_unique<juce::FileChooser>("Enregistrer le preset", folderPath.getChildFile("Mon preset.swellverb"), "*.swellverb");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting, [this](const juce::FileChooser& fc) {
        auto f = fc.getResult();
        if (f == juce::File()) return;
        proc.saveUserPreset(f.withFileExtension("swellverb")); refreshNames();
    });
}
void Panel::folderMenu() {
    auto dir = ReverbProcessor::userPresetFolder();
    auto files = dir.findChildFiles(juce::File::findFiles, false, "*.swellverb");
    files.sort();
    juce::PopupMenu m; m.setLookAndFeel(&look);
    m.addSectionHeader("Mes presets");
    if (files.isEmpty()) m.addItem(-1, "(aucun preset enregistre)", false);
    for (int i = 0; i < files.size(); ++i) m.addItem(i + 1, files[i].getFileNameWithoutExtension(), true, files[i].getFileNameWithoutExtension() == proc.userPresetName);
    m.addSeparator();
    m.addItem(1000, "Charger un fichier...");
    m.addItem(1001, "Ouvrir le dossier des presets");
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&folder), [this, files, dir](int r) {
        if (r > 0 && r <= files.size()) { proc.loadUserPreset(files[r - 1]); refreshNames(); }
        else if (r == 1000) {
            chooser = std::make_unique<juce::FileChooser>("Charger un preset", dir, "*.swellverb");
            chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this](const juce::FileChooser& fc) {
                if (fc.getResult().existsAsFile()) { proc.loadUserPreset(fc.getResult()); refreshNames(); }
            });
        } else if (r == 1001) { dir.createDirectory(); dir.startAsProcess(); }
    });
}
void Panel::settingsMenu() {
    juce::PopupMenu m; m.setLookAndFeel(&look);
    m.addSectionHeader("Taille de l'interface");
    const float sizes[4] { .75f, 1.f, 1.25f, 1.5f };
    for (int i = 0; i < 4; ++i) m.addItem(i + 1, juce::String(juce::roundToInt(sizes[i] * 100)) + " %");
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settings), [this, sizes](int r) { if (r > 0 && onScale) onScale(sizes[r - 1]); });
}

//============================================================================== Editor
ReverbEditor::ReverbEditor(ReverbProcessor& p) : AudioProcessorEditor(p), panel(p) {
    addAndMakeVisible(panel);
    panel.onScale = [this](float s) { scale = s; setSize(juce::roundToInt(Panel::W * s), juce::roundToInt(Panel::H * s)); };
    setSize(Panel::W, Panel::H);
}
void ReverbEditor::resized() {
    panel.setBounds(0, 0, Panel::W, Panel::H);
    panel.setTransform(juce::AffineTransform::scale(scale));
}
