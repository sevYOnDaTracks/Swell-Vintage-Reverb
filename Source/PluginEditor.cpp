#include "PluginEditor.h"
#include "BinaryData.h"
#include "VintageUI.h"

namespace {
const juce::Colour ink { 0xfff4f4f2 }, dim { 0xffa6a6a6 }, muted { 0xff6a6a6a }, well { 0xff0f0f10 }, lineDark { 0xff0b0b0c };
const juce::Image& faceplate() {
    static const auto face = juce::ImageFileFormat::loadFrom(BinaryData::vintagereverb_png, BinaryData::vintagereverb_pngSize);
    return face;
}
juce::Rectangle<int> panelRect(int x, int y, int w, int h) {
    return juce::Rectangle<float>(x, y, w, h).transformedBy(
        juce::AffineTransform::scale(Panel::W / 1774.f, Panel::H / 887.f)).toNearestInt();
}
// Recessed readouts mask the printed example values beneath the controls.
void readoutWell(juce::Graphics& g, juce::Rectangle<float> dest) {
    juce::ColourGradient well(juce::Colour(0xff161411), 0, dest.getY(),
                              juce::Colour(0xff28231d), 0, dest.getBottom(), false);
    g.setGradientFill(well); g.fillRoundedRectangle(dest, 2);
}
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
    const auto bounds = juce::Rectangle<float>(x, y, w, h);
    juce::Path clip;
    clip.addEllipse(bounds);
    g.reduceClipRegion(clip);
    g.addTransform(juce::AffineTransform::rotation(start + pos * (end - start) - .05f,
                                                   bounds.getCentreX(), bounds.getCentreY()));
    static const auto knob = faceplate().getClippedImage({127, 168, 199, 199});
    g.drawImage(knob, bounds);
}

void ReverbLook::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) {
    float cy = y + h * .5f;
    auto track = juce::Rectangle<float>((float)x, cy - 4, (float)w, 8);
    g.setColour(juce::Colour(0xff161410)); g.fillRoundedRectangle(track, 4);
    g.setColour(vui::cream); g.fillRoundedRectangle(track.reduced(2).withRight(std::max(pos, track.getX()+2)), 2);
    static const auto cap = faceplate().getClippedImage({685,408,44,41});
    g.drawImage(cap, juce::Rectangle<float>(pos-13, cy-12, 26, 24));
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
void Knob::place(juce::Rectangle<int> dial, juce::Rectangle<int> value) {
    auto bounds = dial.getUnion(value);
    setBounds(bounds);
    slider.setBounds(dial - bounds.getPosition());
    readout = value - bounds.getPosition();
}
void Knob::paint(juce::Graphics& g) {
    auto r = readout.toFloat();
    readoutWell(g, r);
    drawLabel(g, format((float)slider.getValue()), r, small ? 12.8f : 14.f, vui::cream);
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
    readoutWell(g, getLocalBounds().toFloat());
    auto r = getLocalBounds().toFloat().reduced(5, 1).removeFromTop(16);
    drawLabel(g, label, r, 12.f, vui::cream, juce::Justification::centredLeft, .06f);
    drawLabel(g, format((float) slider.getValue()), r, 12.f, vui::cream, juce::Justification::centredRight);
}

//============================================================================== PresetBox
void PresetBox::paint(juce::Graphics& g) { auto r = getLocalBounds().toFloat().reduced(49, 8);
    g.setColour(juce::Colour(0xff2c1b0b)); g.fillRect(r);
    drawLabel(g, label, r, 16, juce::Colour(0xffffcb76)); }
void PresetBox::mouseDown(const juce::MouseEvent& e) {
    if (e.x < 34) { if (onPrev) onPrev(); }
    else if (e.x > getWidth() - 34) { if (onNext) onNext(); }
    else if (onMenu) onMenu();
}

//============================================================================== Meters
void Meters::paint(juce::Graphics& g) {
    // Each row covers the printed LED cells. Labels and dB ticks belong to the faceplate.
    constexpr int rowY[] {0, 34, 97, 135, 172};
    const float scale = getWidth() / 417.f;
    for (int row = 0; row < 5; ++row) {
        auto strip = juce::Rectangle<float>(0, rowY[row]*scale, (float)getWidth(), 19*scale);
        g.setColour(juce::Colour(0xff141210)); g.fillRect(strip);
        for (int led = 0; led < 20; ++led) {
            auto cell = juce::Rectangle<float>((5+led*20.4f)*scale, strip.getY()+4*scale, 15*scale, 10*scale);
            vui::led(g, cell, led/19.f, level[(size_t)row] >= -60 + led*(60.f/19));
        }
    }
}

//============================================================================== Icons and tabs
void IconButton::paint(juce::Graphics& g) {
    if (isMouseOver()) {
        g.setColour(vui::cream.withAlpha(.12f));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 3);
    }
}

void TabLabel::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(1, 0).withTrimmedTop(3);
    juce::Path tab; tab.addRoundedRectangle(r.getX(), r.getY(), r.getWidth(), r.getHeight() + 4, 5, 5, true, true, false, false);
    g.setColour(juce::Colours::black.withAlpha(.6f)); g.fillPath(tab, juce::AffineTransform::translation(0, -1));
    juce::ColourGradient grad(juce::Colour(active ? 0xff3a3833 : 0xff1e1d20), 0, r.getY(), juce::Colour(active ? 0xff232220 : 0xff141316), 0, r.getBottom(), false);
    g.setGradientFill(grad); g.fillPath(tab);
    g.setColour(active ? vui::cream.withAlpha(.35f) : juce::Colours::white.withAlpha(.05f)); g.strokePath(tab, juce::PathStrokeType(1));
    drawLabel(g, name, r, 12.5f, active ? ink : muted, juce::Justification::centred, .16f, active);
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
    g.setColour(juce::Colour(0xff1a1918)); g.fillRoundedRectangle(r, 5);
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
            "Vintage Reverb - version 0.2.1\nSwell Audio, by @sevyondatracks\n\nDouble-clic sur un bouton : valeur par defaut.\nShift + glisser : reglage fin.\nLes presets ne touchent jamais au Mix ni au Gain.");
    };
    showTab(false);
    refreshNames();
    setSize(W, H);
    startTimerHz(30);
}
Panel::~Panel() { setLookAndFeel(nullptr); }

void Panel::resized() {
    // Positions follow the original photographed dial and readout centres.
    mainKnobs[0]->place(panelRect(126,166,202,202), panelRect(156,379,160,39));
    mainKnobs[1]->place(panelRect(387,180,109,109), panelRect(365,311,155,33));
    mainKnobs[2]->place(panelRect(533,178,101,101), panelRect(530,295,105,34));
    constexpr int centres[]{186,379,551,694,861,1010,1174,1315,1464,1610};
    for (size_t i=0; i<advKnobs.size(); ++i) {
        int x = centres[i];
        advKnobs[i]->place(panelRect(x-56,637,112,112), panelRect(x-60,766,120,36));
    }
    categoryBox.setBounds(panelRect(681,161,430,77));
    programBox.setBounds(panelRect(681,252,430,77));
    meters.setBounds(panelRect(1255,144,417,193));
    vlf->setBounds(panelRect(402,377,467,79));
    earlyLate->setBounds(panelRect(971,377,491,79));
    advTab.setBounds(panelRect(105,479,338,54));
    eqTab.setBounds(panelRect(443,479,323,54));
    eq.setBounds(panelRect(77,531,1623,299));
    int x=1406;
    for (auto* button : { &save, &folder, &settings, &help }) {
        button->setBounds(panelRect(x,23,60,61));
        x+=65;
    }
}

void Panel::paint(juce::Graphics& g) {
    g.drawImage(faceplate(), getLocalBounds().toFloat());
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
    juce::PropertiesFile::Options o;
    o.applicationName = "Swell Reverb"; o.filenameSuffix = ".settings"; o.folderName = "Swell Audio"; o.osxLibrarySubFolder = "Application Support";
    props = std::make_unique<juce::PropertiesFile>(o);
    scale = juce::jlimit(.6f, 2.f, (float) props->getDoubleValue("scale", 1.0));
    panel.onScale = [this](float s) { setSize(juce::roundToInt(Panel::W * s), juce::roundToInt(Panel::H * s)); };
    setResizable(true, true);
    setResizeLimits(juce::roundToInt(Panel::W * .6f), juce::roundToInt(Panel::H * .6f), Panel::W * 2, Panel::H * 2);
    getConstrainer()->setFixedAspectRatio((double) Panel::W / Panel::H);
    setSize(juce::roundToInt(Panel::W * scale), juce::roundToInt(Panel::H * scale));
    ready = true;
}
void ReverbEditor::resized() {
    scale = (float) getWidth() / (float) Panel::W;
    panel.setBounds(0, 0, Panel::W, Panel::H);
    panel.setTransform(juce::AffineTransform::scale(scale));
    if (ready) { props->setValue("scale", (double) scale); props->saveIfNeeded(); }
}
