#include "PluginEditor.h"

using namespace juce;

//==============================================================================
//  Static artwork
//==============================================================================
void BackgroundLayer::paint (Graphics& g)
{
    const auto full = getLocalBounds().toFloat();

    g.fillAll (Theme::bg);
    Theme::drawNoise (g, getLocalBounds(), 0.6f);
    Theme::drawScratches (g, full, 260, 7, 0.06f);

    g.setColour (Colours::black);
    g.drawRoundedRectangle (full.reduced (2), 10, 3);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (full.reduced (5), 9, 1);
    for (auto p : { Point<float> (16, 16), { full.getRight() - 16, 16 }, { 16, full.getBottom() - 16 },
                    { full.getRight() - 16, full.getBottom() - 16 } })
        Theme::drawScrew (g, p, 5);

    // ---------------- header ----------------
    Theme::drawPanel (g, { 10, 10, 1516, 100 }, 6);
    {
        const Point<float> c (98, 60);
        Path orbitPath;
        orbitPath.addEllipse (-62, -18, 124, 36);
        orbitPath.applyTransform (AffineTransform::rotation (-0.35f).translated (c.x, c.y));
        g.setColour (Theme::textDim.withAlpha (0.45f));
        g.strokePath (orbitPath, PathStrokeType (0.8f));
        Theme::drawPlanet (g, c, 26, Theme::text, -0.35f);
        g.setColour (Theme::text.withAlpha (0.6f));
        g.fillEllipse (c.x + 40, c.y - 26, 4, 4);
        g.fillEllipse (c.x - 52, c.y + 12, 3, 3);
    }
    {
        const String name = String (GRAINLAB_PLUGIN_NAME).toUpperCase();
        g.setFont (Theme::mono (50.0f, true).withExtraKerningFactor (0.10f));
        g.setColour (Colours::black.withAlpha (0.7f));
        g.drawText (name, Rectangle<float> (178, 20, 460, 54), Justification::centredLeft);
        g.setGradientFill (ColourGradient (Colour (0xffd7dedc), 0, 22, Colour (0xff7b8583), 0, 70, false));
        g.drawText (name, Rectangle<float> (176, 18, 460, 54), Justification::centredLeft);
        Theme::drawText (g, "GRANULAR AUDIO EXPERIMENTATION SYSTEM", { 178, 72, 470, 20 }, 15.0f, Theme::text,
                         Justification::centredLeft, false, 0.14f);
    }
    Theme::drawPanel (g, { 648, 22, 388, 80 }, 4, true);
    g.setColour (Theme::border);
    g.drawVerticalLine (893, 32, 92);

    Theme::drawPanel (g, { 1052, 20, 296, 86 }, 4, true);
    {
        const Point<float> c (1180, 62);
        g.setColour (Theme::textFaint.withAlpha (0.5f));
        for (int i = 0; i < 9; ++i) g.drawVerticalLine (1066 + i * 18, 30, 96);
        for (int i = 0; i < 4; ++i) g.drawHorizontalLine (34 + i * 20, 1062, 1240);
        g.setColour (Theme::text.withAlpha (0.5f));
        g.drawEllipse (c.x - 30, c.y - 30, 60, 60, 1.0f);
        Path o; o.addEllipse (-58, -14, 116, 28);
        o.applyTransform (AffineTransform::rotation (-0.5f).translated (c.x, c.y));
        g.strokePath (o, PathStrokeType (0.8f));
        g.drawLine (1070, 92, 1236, 30, 0.6f);
        g.fillEllipse (1232, 26, 6, 6);
        g.fillEllipse (1066, 88, 5, 5);
        for (int i = 0; i < 3; ++i)
            Theme::drawText (g, StringArray { "EXPLORE", "MANIPULATE", "CREATE" }[i], { 1254, 32.0f + i * 18, 92, 18 }, 12.0f,
                             Theme::text, Justification::centredLeft, false, 0.12f);
    }

    Theme::drawPanel (g, { 1362, 22, 80, 80 }, 4, true);
    {
        const Point<float> c (1402, 62);
        g.setGradientFill (ColourGradient (Colour (0xfffff1c9).withAlpha (0.9f), c.x, c.y, Colour (0x00ffb060), c.x + 30, c.y, true));
        g.fillEllipse (c.x - 30, c.y - 30, 60, 60);
        g.setColour (Colours::black);
        g.fillEllipse (c.x - 18, c.y - 18, 36, 36);
        g.setColour (Colour (0xfffff3d6));
        g.drawEllipse (c.x - 18, c.y - 18, 36, 36, 1.2f);
    }
    for (int i = 0; i < 3; ++i)
        Theme::drawPanel (g, { 1458.0f + i * 15, 24, 10, 76 }, 2, true);

    // ---------------- left column ----------------
    Theme::drawPanel (g, { 18, 124, 212, 136 }, 5);
    Theme::drawPanel (g, { 18, 332, 212, 270 }, 5);
    Theme::drawPanel (g, { 18, 610, 212, 284 }, 5);
    for (auto p : { Point<float> (30, 622), { 218, 622 }, { 30, 882 }, { 218, 882 } }) Theme::drawScrew (g, p, 4);

    // ---------------- screen bezel ----------------
    Theme::drawPanel (g, { 236, 122, 958, 368 }, 8);
    g.setColour (Colours::black);
    g.fillRoundedRectangle (252, 132, 926, 346, 10);
    g.setColour (Theme::tealDim.withAlpha (0.5f));
    g.drawRoundedRectangle (259, 137, 910, 332, 9, 2.0f);
    for (auto p : { Point<float> (246, 132), { 1184, 132 }, { 246, 480 }, { 1184, 480 } }) Theme::drawScrew (g, p, 3.5f);

    // ---------------- control sections ----------------
    struct Section { Rectangle<float> r; const char* title; Theme::Icon icon; };
    const Section sections[] = {
        { { 242, 502, 212, 390 }, "GRAIN",    Theme::Icon::Grain },
        { { 462, 502, 204, 390 }, "MOTION",   Theme::Icon::Motion },
        { { 674, 502, 200, 390 }, "RANDOM",   Theme::Icon::Random },
        { { 882, 502, 144, 390 }, "ENVELOPE", Theme::Icon::Envelope },
        { { 1034, 502, 152, 390 }, "TIME",    Theme::Icon::Time },
    };
    for (const auto& s : sections)
    {
        Theme::drawPanel (g, s.r, 5);
        Theme::drawSectionHeader (g, s.r, s.title, s.icon);
    }
    auto divider = [&g] (float x0, float x1, float y)
    {
        g.setColour (Colours::black.withAlpha (0.6f)); g.drawHorizontalLine ((int) y, x0, x1);
        g.setColour (Colours::white.withAlpha (0.04f)); g.drawHorizontalLine ((int) y + 1, x0, x1);
    };
    divider (252, 444, 702);
    divider (472, 656, 702);
    divider (684, 864, 702);
    Theme::drawText (g, "DIRECTION", { 462, 716, 204, 20 }, 13.0f, Theme::text, Justification::centred, false, 0.1f);
    Theme::drawPanel (g, { 1042, 668, 136, 112 }, 4, true);
    Theme::drawPanel (g, { 1042, 786, 136, 100 }, 4, true);

    // ---------------- right column ----------------
    Theme::drawPanel (g, { 1200, 126, 318, 152 }, 5);
    Theme::drawPanel (g, { 1204, 284, 310, 218 }, 5);
    Theme::drawSectionHeader (g, { 1204, 284, 310, 218 }, "MASTER", Theme::Icon::None);
    Theme::drawScrew (g, { 1496, 300 }, 4);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawVerticalLine (1330, 330, 436);

    for (auto [y, title] : { std::pair<float, const char*> { 508.0f, "LOOP MODE" }, { 602.0f, "MIDI MODE" } })
    {
        Theme::drawPanel (g, { 1204, y, 310, 88 }, 5);
        Theme::drawText (g, title, { 1216, y + 6, 200, 20 }, 12.5f, Theme::textDim, Justification::centredLeft, false, 0.1f);
        Theme::drawText (g, ">", { 1490, y + 6, 14, 20 }, 12.0f, Theme::textFaint, Justification::centred);
    }
    Theme::drawPanel (g, { 1200, 698, 318, 196 }, 5);

    // ---------------- bottom bar ----------------
    Theme::drawPanel (g, { 18, 900, 1500, 108 }, 6);
    Theme::drawPanel (g, { 466, 914, 456, 76 }, 4, true);
    Theme::drawPanel (g, { 932, 914, 252, 76 }, 4, true);
    Theme::drawPanel (g, { 1194, 910, 316, 88 }, 4, true);
    Theme::drawText (g, String (GRAINLAB_PLUGIN_NAME).toUpperCase(), { 1222, 928, 200, 18 }, 12.5f, Theme::textDim,
                     Justification::centredLeft, false, 0.14f);
    Theme::drawText (g, "v" + String (JucePlugin_VersionString), { 1222, 948, 200, 18 }, 12.0f, Theme::textFaint,
                     Justification::centredLeft, false, 0.1f);
}

//==============================================================================
//  Editor
//==============================================================================
GrainLabEditor::GrainLabEditor (GrainLabProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      statusView (p), sampleCard (p), presetList (p), radar (p), waveform (p),
      directionButtons (p.apvts, PID::direction, ParamChoices::direction(), ChoiceButtons::Style::Row, 5.0f),
      envelopeButtons (p.apvts, PID::envelope, ParamChoices::envelope(), ChoiceButtons::Style::EnvelopeColumn, 9.0f),
      loopButtons (p.apvts, PID::loopMode, ParamChoices::loopMode(), ChoiceButtons::Style::Row, 7.0f),
      midiButtons (p.apvts, PID::midiMode, ParamChoices::midiMode(), ChoiceButtons::Style::Row, 7.0f),
      reverseToggle (p.apvts, PID::scanReverse, "REVERSE", ParamToggle::Look::LedText),
      freezeToggle (p.apvts, PID::freeze, "FREEZE", ParamToggle::Look::Freeze),
      playToggle (p.apvts, PID::play, "", ParamToggle::Look::Play),
      vuIn (p, false, "INPUT"), vuOut (p, true, "OUTPUT"), ledMeter (p), orbit (p), transport (p), grainCounter (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);
    content.setBounds (0, 0, designW, designH);

    background.setBounds (0, 0, designW, designH);
    content.addAndMakeVisible (background);

    auto place = [this] (Component& c, int x, int y, int w, int h) { c.setBounds (x, y, w, h); content.addAndMakeVisible (c); };

    place (statusView, 648, 22, 388, 80);
    place (sampleCard, 26, 132, 196, 120);
    place (loadButton, 24, 268, 200, 54);
    place (presetList, 22, 336, 204, 262);
    place (radar, 18, 610, 212, 284);
    place (waveform, 262, 140, 904, 326);

    using L = KnobControl::Layout;
    // GRAIN
    addKnob (PID::size,     "SIZE",     "5 ms", "1000 ms", 36, 296, 614, L::TitleTop, "Grain length");
    addKnob (PID::density,  "DENSITY",  "0.5",  "200",     36, 400, 614, L::TitleTop, "Grains per second");
    addKnob (PID::position, "POSITION", "0%",   "100%",    36, 296, 780, L::TitleTop, "Where in the sample grains come from (or click/drag the waveform)");
    addKnob (PID::spread,   "SPREAD",   "0%",   "100%",    36, 400, 780, L::TitleTop, "Width of the region grains are taken from");
    // MOTION
    addKnob (PID::speed,      "SPEED", "0.25x", "4x",  36, 514, 614, L::TitleTop, "How fast the playhead scans through the sample");
    addKnob (PID::grainPitch, "PITCH", "-24",   "+24", 36, 614, 614, L::TitleTop, "Grain pitch in semitones (timing unchanged)");
    // RANDOM
    addKnob (PID::posRand,   "POSITION", "0%", "100%", 34, 726, 614, L::TitleTop, "Random position jitter per grain (up to +/-0.5 s)");
    addKnob (PID::pitchRand, "PITCH",    "0%", "100%", 34, 822, 614, L::TitleTop, "Random pitch per grain (up to +/-12 st)");
    addKnob (PID::speedRand, "SPEED",    "0%", "100%", 34, 726, 780, L::TitleTop, "Random wobble of the scan speed");
    addKnob (PID::panRand,   "PAN",      "0%", "100%", 34, 822, 780, L::TitleTop, "Random stereo position per grain");
    // TIME
    addKnob (PID::stretch,     "STRETCH",      "25%",  "400%", 26, 1110, 598, L::TitleTop, "Time stretch: 400% = 4x longer, pitch unchanged");
    addKnob (PID::globalPitch, "GLOBAL PITCH", "-24",  "+24",  24, 1110, 722, L::TitleTop, "Master transpose in semitones");
    addKnob (PID::fineTune,    "FINE TUNE",    "-100", "+100", 20, 1110, 834, L::TitleTop, "Fine tune in cents");
    // MASTER
    addKnob (PID::gain,  "GAIN",  String (CharPointer_UTF8 ("-\xe2\x88\x9e")), "+12 dB", 40, 1270, 364, L::TitleBelow, "Output gain");
    addKnob (PID::pan,   "PAN",   "L",  "R",    26, 1376, 360, L::TitleBelow, "Output pan");
    addKnob (PID::width, "WIDTH", "0%", "200%", 26, 1462, 360, L::TitleBelow, "Stereo width");

    place (directionButtons, 470, 742, 188, 48);
    place (reverseToggle, 504, 808, 120, 44);
    place (envelopeButtons, 894, 552, 120, 300);
    place (vuIn, 1208, 136, 148, 132);
    place (vuOut, 1362, 136, 148, 132);
    place (ledMeter, 1216, 452, 292, 30);
    place (loopButtons, 1212, 536, 296, 50);
    place (midiButtons, 1212, 630, 296, 50);
    place (orbit, 1200, 698, 318, 196);
    place (randomizeButton, 38, 914, 200, 76);
    place (freezeToggle, 260, 914, 182, 76);
    place (transport, 466, 914, 456, 76);
    place (playToggle, 545, 929, 46, 46);
    place (grainCounter, 932, 914, 252, 76);

    playToggle.activeOverride = [this] { return (int) proc.apvts.getRawParameterValue (PID::midiMode)->load() == 0; };
    playToggle.setTooltip ("Play / stop (when MIDI MODE is OFF)");
    freezeToggle.setTooltip ("Freeze: hold the playhead and sustain the texture indefinitely");
    reverseToggle.setTooltip ("Scan through the sample backwards");

    loadButton.onClick = [this] { openFileChooser(); };
    randomizeButton.onClick = [this] { proc.randomize(); };

    proc.sampleBroadcaster.addChangeListener (this);

    setResizable (true, true);
    setResizeLimits (designW / 2, designH / 2, designW * 3 / 2, designH * 3 / 2);
    getConstrainer()->setFixedAspectRatio ((double) designW / designH);
    const int w = jlimit (designW / 2, designW * 3 / 2, proc.editorWidth.load());
    setSize (w, roundToInt (w * (double) designH / designW));

    startTimerHz (30);
}

GrainLabEditor::~GrainLabEditor()
{
    stopTimer();
    proc.sampleBroadcaster.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

KnobControl& GrainLabEditor::addKnob (const String& id, const String& title, const String& lo, const String& hi,
                                      float radius, float cx, float cy, KnobControl::Layout layout, const String& tip)
{
    auto* k = knobs.add (new KnobControl (proc.apvts, id, title, lo, hi, radius, layout, tip));
    k->placeAt (cx, cy);
    content.addAndMakeVisible (k);
    return *k;
}

void GrainLabEditor::resized()
{
    const float scale = (float) getWidth() / (float) designW;
    content.setTransform (AffineTransform::scale (scale));
    proc.editorWidth.store (getWidth());
}

void GrainLabEditor::paintOverChildren (Graphics& g)
{
    if (! dragOver) return;
    g.setColour (Theme::orange.withAlpha (0.12f));
    g.fillAll();
    g.setColour (Theme::orange);
    g.drawRect (getLocalBounds(), 3);
}

void GrainLabEditor::timerCallback()
{
    proc.pollMeters();
    waveform.tick();
    vuIn.tick();
    vuOut.tick();
    ledMeter.tick();
    grainCounter.tick();
    radar.tick();
    orbit.tick();
    statusView.tick();
    transport.tick();

    loadButton.ledOn = proc.isLoading() || proc.getUISample() != nullptr;

    if (proc.getPresetIndex() != lastPreset) { lastPreset = proc.getPresetIndex(); presetList.repaint(); }
    const int mm = (int) proc.apvts.getRawParameterValue (PID::midiMode)->load();
    if (mm != lastMidiMode) { lastMidiMode = mm; playToggle.repaint(); }
}

void GrainLabEditor::changeListenerCallback (ChangeBroadcaster*)
{
    statusView.repaint();
    sampleCard.repaint();
    loadButton.repaint();
    waveform.repaint();
}

void GrainLabEditor::openFileChooser()
{
    chooser = std::make_unique<FileChooser> ("Load a sample", File(),
                                             proc.getFormatManager().getWildcardForAllFormats());
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                          [this] (const FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.existsAsFile()) { proc.loadSampleAsync (f); sampleCard.repaint(); waveform.repaint(); }
                          });
}

bool GrainLabEditor::isInterestedInFileDrag (const StringArray& files)
{
    const auto wildcard = proc.getFormatManager().getWildcardForAllFormats();
    for (const auto& f : files)
        if (wildcard.containsIgnoreCase (File (f).getFileExtension()) && File (f).getFileExtension().isNotEmpty())
            return true;
    return false;
}

void GrainLabEditor::filesDropped (const StringArray& files, int, int)
{
    dragOver = false;
    repaint();
    for (const auto& f : files)
    {
        const File file (f);
        if (file.existsAsFile()) { proc.loadSampleAsync (file); break; }
    }
}
