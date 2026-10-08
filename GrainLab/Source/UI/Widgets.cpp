#include "Widgets.h"
#include "../PluginProcessor.h"
#include "../Presets.h"

using namespace juce;

//==============================================================================
GrainLabLookAndFeel::GrainLabLookAndFeel()
{
    setColour (TooltipWindow::backgroundColourId, Colour (0xf0101314));
    setColour (TooltipWindow::textColourId, Theme::text);
    setColour (TooltipWindow::outlineColourId, Theme::borderHi);
    setColour (PopupMenu::backgroundColourId, Theme::panel);
    setColour (PopupMenu::textColourId, Theme::text);
    setColour (PopupMenu::highlightedBackgroundColourId, Theme::orangeDim);
}

void GrainLabLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                            float startAngle, float endAngle, Slider&)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c = bounds.getCentre();
    const float outerR = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float bodyR = outerR - 9.0f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // scale ticks
    for (int i = 0; i <= 20; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / 20.0f;
        const bool major = (i % 5) == 0;
        const float r0 = outerR - (major ? 6.0f : 4.0f), r1 = outerR - 1.0f;
        g.setColour ((a <= angle + 0.001f ? Theme::orange.withAlpha (0.55f) : Theme::textFaint.withAlpha (0.7f)));
        g.drawLine (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0, c.x + std::sin (a) * r1, c.y - std::cos (a) * r1,
                    major ? 1.2f : 0.8f);
    }

    // drop shadow
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (c.x - bodyR + 1.5f, c.y - bodyR + 3.0f, bodyR * 2, bodyR * 2);

    // knurled rim
    g.setGradientFill (ColourGradient (Colour (0xff2b3133), c.x - bodyR, c.y - bodyR,
                                       Colour (0xff08090a), c.x + bodyR, c.y + bodyR, false));
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2);
    const int ridges = 56;
    for (int i = 0; i < ridges; ++i)
    {
        const float a = (float) i / ridges * MathConstants<float>::twoPi;
        const float lit = 0.5f + 0.5f * std::cos (a + 2.3f);
        g.setColour (Colours::white.withAlpha (0.03f + 0.07f * lit));
        g.drawLine (c.x + std::sin (a) * bodyR * 0.86f, c.y - std::cos (a) * bodyR * 0.86f,
                    c.x + std::sin (a) * bodyR * 0.99f, c.y - std::cos (a) * bodyR * 0.99f, 1.0f);
    }
    g.setColour (Colours::black);
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2, 1.2f);

    // cap
    const float capR = bodyR * 0.82f;
    g.setGradientFill (ColourGradient (Colour (0xff30373a), c.x - capR * 0.6f, c.y - capR * 0.8f,
                                       Colour (0xff0c0e0f), c.x + capR * 0.5f, c.y + capR, false));
    g.fillEllipse (c.x - capR, c.y - capR, capR * 2, capR * 2);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawEllipse (c.x - capR + 0.5f, c.y - capR + 0.5f, capR * 2 - 1, capR * 2 - 1, 0.8f);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.07f), c.x, c.y - capR,
                                       Colours::transparentWhite, c.x, c.y, false));
    g.fillEllipse (c.x - capR * 0.8f, c.y - capR * 0.95f, capR * 1.6f, capR * 1.1f);

    // pointer with glow
    const float p0 = bodyR * 0.30f, p1 = bodyR * 0.96f;
    const Point<float> a0 (c.x + std::sin (angle) * p0, c.y - std::cos (angle) * p0);
    const Point<float> a1 (c.x + std::sin (angle) * p1, c.y - std::cos (angle) * p1);
    g.setColour (Theme::orange.withAlpha (0.22f));
    g.drawLine ({ a0, a1 }, 6.0f);
    g.setColour (Theme::orange);
    g.drawLine ({ a0, a1 }, 2.2f);
    g.setColour (Colour (0xffffd9a0));
    g.drawLine ({ a0, a1 }, 0.8f);
}

//==============================================================================
KnobControl::KnobControl (AudioProcessorValueTreeState& state, const String& paramID, const String& t,
                          const String& minT, const String& maxT, float r, Layout l, const String& tooltip)
    : title (t), minText (minT), maxText (maxT), radius (r), layout (l)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (220);
    slider.setTooltip (tooltip);
    slider.setMouseCursor (MouseCursor::UpDownResizeCursor);
    addAndMakeVisible (slider);

    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (state, paramID, slider);
    if (auto* p = state.getParameter (paramID))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));

    slider.onValueChange = [this] { repaint(); };
    slider.onDragStart = [this] { repaint(); };
    slider.onDragEnd = [this] { repaint(); };
    width = (int) jmax (110.0f, radius * 2 + 50);
    setInterceptsMouseClicks (false, true);
}

void KnobControl::placeAt (float cx, float cy)
{
    if (layout == Layout::TitleTop)
        setBounds ((int) (cx - width * 0.5f), (int) (cy - radius - 22), width, (int) (radius * 2 + 22 + 36));
    else
        setBounds ((int) (cx - width * 0.5f), (int) (cy - radius - 10), width, (int) (radius * 2 + 10 + 36));
}

void KnobControl::resized()
{
    const float cy = layout == Layout::TitleTop ? radius + 22 : radius + 10;
    const float side = (radius + 10) * 2;
    slider.setBounds (Rectangle<float> (side, side).withCentre ({ width * 0.5f, cy }).toNearestInt());
}

void KnobControl::paint (Graphics& g)
{
    const float w = (float) getWidth();
    const float cy = layout == Layout::TitleTop ? radius + 22 : radius + 10;
    const String valueText = slider.getTextFromValue (slider.getValue());
    const float rangeY = cy + radius + 2;
    const float small = radius >= 30 ? 11.5f : 10.5f;

    Theme::drawText (g, minText, { 0, rangeY, w * 0.5f - radius * 0.35f, 14 }, small, Theme::textDim, Justification::centredRight);
    Theme::drawText (g, maxText, { w * 0.5f + radius * 0.35f, rangeY, w * 0.5f - radius * 0.35f, 14 }, small, Theme::textDim, Justification::centredLeft);

    if (layout == Layout::TitleTop)
    {
        Theme::drawText (g, title, { 0, 0, w, 15 }, 13.0f, Theme::text, Justification::centred, false, 0.08f);
        Theme::drawText (g, valueText, { 0, rangeY + 15, w, 17 }, 14.0f,
                         slider.isMouseButtonDown() ? Theme::orange : Theme::text, Justification::centred);
    }
    else
    {
        const bool showValue = slider.isMouseOverOrDragging();
        Theme::drawText (g, showValue ? valueText : title, { 0, rangeY + 15, w, 17 }, 13.0f,
                         showValue ? Theme::orange : Theme::text, Justification::centred, false, 0.08f);
    }
}

//==============================================================================
ChoiceButtons::ChoiceButtons (AudioProcessorValueTreeState& state, const String& paramID,
                              const StringArray& l, Style s, float gp)
    : labels (l), style (s), gap (gp),
      attachment (*state.getParameter (paramID), [this] (float v) { selected = roundToInt (v); repaint(); }, nullptr)
{
    attachment.sendInitialUpdate();
    setMouseCursor (MouseCursor::PointingHandCursor);
}

Rectangle<float> ChoiceButtons::itemBounds (int i) const
{
    const int n = labels.size();
    const auto b = getLocalBounds().toFloat().reduced (3.0f);
    if (style == Style::Row)
    {
        const float w = (b.getWidth() - gap * (n - 1)) / n;
        return { b.getX() + i * (w + gap), b.getY(), w, b.getHeight() };
    }
    const float h = (b.getHeight() - gap * (n - 1)) / n;
    return { b.getX(), b.getY() + i * (h + gap), b.getWidth(), h };
}

int ChoiceButtons::itemAt (Point<float> p) const
{
    for (int i = 0; i < labels.size(); ++i)
        if (itemBounds (i).expanded (gap * 0.5f).contains (p)) return i;
    return -1;
}

void ChoiceButtons::mouseDown (const MouseEvent& e)
{
    const int i = itemAt (e.position);
    if (i >= 0) attachment.setValueAsCompleteGesture ((float) i);
}

void ChoiceButtons::mouseMove (const MouseEvent& e)
{
    const int i = itemAt (e.position);
    if (i != hover) { hover = i; repaint(); }
}

void ChoiceButtons::mouseExit (const MouseEvent&) { hover = -1; repaint(); }

void ChoiceButtons::drawEnvelopeGlyph (Graphics& g, int index, Rectangle<float> r, Colour col)
{
    Path p;
    const int steps = 24;
    for (int i = 0; i <= steps; ++i)
    {
        const float x = (float) i / steps;
        const float pi = MathConstants<float>::pi;
        float v = 0;
        switch (index)
        {
            case 0: v = 0.5f - 0.5f * std::cos (2 * pi * x); break;
            case 1: v = std::exp (-0.5f * std::pow ((2 * x - 1) / 0.35f, 2.0f)); break;
            case 2: v = 1.0f - std::abs (2 * x - 1); break;
            case 3: v = 0.54f - 0.46f * std::cos (2 * pi * x); break;
            case 4: v = (x > 0.04f && x < 0.96f) ? 1.0f : 0.0f; break;
            default: v = 0.42f - 0.5f * std::cos (2 * pi * x) + 0.08f * std::cos (4 * pi * x); break;
        }
        const Point<float> pt (r.getX() + x * r.getWidth(), r.getBottom() - v * r.getHeight());
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    g.setColour (col);
    g.strokePath (p, PathStrokeType (1.2f));
}

void ChoiceButtons::paint (Graphics& g)
{
    for (int i = 0; i < labels.size(); ++i)
    {
        const auto r = itemBounds (i);
        const bool sel = i == selected;
        Theme::drawButtonBox (g, r, sel, i == hover);

        if (style == Style::Row)
        {
            Theme::drawText (g, labels[i], r, r.getHeight() > 34 ? 13.0f : 12.0f,
                             sel ? Theme::orange : Theme::textDim, Justification::centred, sel, 0.04f);
        }
        else
        {
            auto iconArea = Rectangle<float> (r.getX() + 8, r.getY() + 6, 22, r.getHeight() - 12);
            if (sel) Theme::drawLED (g, iconArea.getCentre(), 4.5f, Theme::orange, true);
            else
            {
                g.setColour (Colours::black.withAlpha (0.5f));
                g.fillRoundedRectangle (iconArea, 2.0f);
                g.setColour (Theme::border);
                g.drawRoundedRectangle (iconArea, 2.0f, 0.8f);
                drawEnvelopeGlyph (g, i, iconArea.reduced (4, 5), Theme::textDim);
            }
            Theme::drawText (g, labels[i], r.withTrimmedLeft (40), 12.0f,
                             sel ? Theme::orange : Theme::textDim, Justification::centredLeft, sel, 0.05f);
        }
    }
}

//==============================================================================
ParamToggle::ParamToggle (AudioProcessorValueTreeState& state, const String& paramID, const String& t, Look l)
    : text (t), look (l),
      attachment (*state.getParameter (paramID), [this] (float v) { on = v > 0.5f; repaint(); }, nullptr)
{
    attachment.sendInitialUpdate();
    setMouseCursor (MouseCursor::PointingHandCursor);
}

void ParamToggle::mouseDown (const MouseEvent&)
{
    attachment.setValueAsCompleteGesture (on ? 0.0f : 1.0f);
}

static void drawSnowflake (Graphics& g, Point<float> c, float r, Colour col)
{
    g.setColour (col);
    for (int i = 0; i < 6; ++i)
    {
        const float a = i * MathConstants<float>::pi / 3.0f;
        const Point<float> tip (c.x + std::cos (a) * r, c.y + std::sin (a) * r);
        g.drawLine ({ c, tip }, 1.5f);
        for (float k : { 0.5f, 0.75f })
        {
            const Point<float> m (c.x + std::cos (a) * r * k, c.y + std::sin (a) * r * k);
            const float b = r * 0.22f;
            g.drawLine (m.x, m.y, m.x + std::cos (a + 0.8f) * b, m.y + std::sin (a + 0.8f) * b, 1.2f);
            g.drawLine (m.x, m.y, m.x + std::cos (a - 0.8f) * b, m.y + std::sin (a - 0.8f) * b, 1.2f);
        }
    }
}

void ParamToggle::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (3.0f);
    const bool active = activeOverride == nullptr || activeOverride();

    switch (look)
    {
        case Look::LedText:
        {
            Theme::drawButtonBox (g, b, false, hovered);
            Theme::drawLED (g, { b.getX() + 16, b.getCentreY() }, 4.0f, Theme::orange, on);
            Theme::drawText (g, text, b.withTrimmedLeft (30), 12.5f, on ? Theme::orange : Theme::textDim,
                             Justification::centredLeft, false, 0.06f);
            break;
        }
        case Look::Freeze:
        {
            if (on)
            {
                g.setColour (Theme::teal.withAlpha (0.16f));
                g.fillRoundedRectangle (b.expanded (3), 7);
                g.setGradientFill (ColourGradient (Colour (0xff0d2a28), b.getX(), b.getY(),
                                                   Colour (0xff061514), b.getX(), b.getBottom(), false));
                g.fillRoundedRectangle (b, 5);
                g.setColour (Theme::teal);
                g.drawRoundedRectangle (b.reduced (0.5f), 5, 1.5f);
            }
            else
            {
                Theme::drawButtonBox (g, b, false, hovered, 5.0f);
                g.setColour (Theme::borderHi);
                g.drawRoundedRectangle (b.reduced (3), 4, 0.8f);
            }
            drawSnowflake (g, { b.getX() + 50, b.getCentreY() }, 15.0f, on ? Theme::teal : Theme::text.withAlpha (0.8f));
            Theme::drawText (g, text, b.withTrimmedLeft (84), 17.0f, on ? Theme::teal : Theme::text,
                             Justification::centredLeft, false, 0.1f);
            break;
        }
        case Look::Play:
        {
            const auto c = b.getCentre();
            const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
            if (on && active)
            {
                g.setColour (Theme::orange.withAlpha (0.2f));
                g.fillEllipse (c.x - r - 3, c.y - r - 3, (r + 3) * 2, (r + 3) * 2);
            }
            g.setGradientFill (ColourGradient (Colour (0xff2e3436), c.x, c.y - r, Colour (0xff0c0e0f), c.x, c.y + r, false));
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (on && active ? Theme::orange : Theme::border);
            g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.2f);

            Path tri;
            if (on && active)   // playing -> show stop square
                tri.addRectangle (c.x - r * 0.3f, c.y - r * 0.3f, r * 0.6f, r * 0.6f);
            else
                tri.addTriangle (c.x - r * 0.28f, c.y - r * 0.4f, c.x - r * 0.28f, c.y + r * 0.4f, c.x + r * 0.45f, c.y);
            g.setColour (active ? (on ? Theme::orange : Theme::text) : Theme::textFaint);
            g.fillPath (tri);
            break;
        }
    }
}

//==============================================================================
static void drawDice (Graphics& g, Point<float> c, float s, Colour col)
{
    // isometric die
    Path top, left, right;
    const float h = s * 0.5f;
    top.addQuadrilateral (c.x, c.y - s, c.x + s, c.y - h, c.x, c.y, c.x - s, c.y - h);
    left.addQuadrilateral (c.x - s, c.y - h, c.x, c.y, c.x, c.y + s, c.x - s, c.y + h);
    right.addQuadrilateral (c.x, c.y, c.x + s, c.y - h, c.x + s, c.y + h, c.x, c.y + s);
    g.setColour (col.withAlpha (0.95f)); g.fillPath (top);
    g.setColour (col.darker (0.35f));    g.fillPath (left);
    g.setColour (col.darker (0.6f));     g.fillPath (right);
    g.setColour (Colour (0xff1a0e04));
    const float d = s * 0.17f;
    g.fillEllipse (c.x - d * 0.5f, c.y - h - d * 0.4f, d, d * 0.7f);
    g.fillEllipse (c.x - s * 0.62f, c.y + s * 0.02f, d * 0.8f, d);
    g.fillEllipse (c.x - s * 0.38f, c.y + s * 0.38f, d * 0.8f, d);
    g.fillEllipse (c.x + s * 0.25f, c.y - s * 0.05f, d * 0.8f, d);
    g.fillEllipse (c.x + s * 0.55f, c.y + s * 0.3f, d * 0.8f, d);
}

void ActionButton::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (3.0f);

    if (look == Look::Load)
    {
        Theme::drawButtonBox (g, b, false, hovered, 4.0f);
        Theme::drawLED (g, { b.getX() + 18, b.getCentreY() }, 5.0f, Theme::orange, ledOn || pressed);
        Theme::drawText (g, text, b.withTrimmedLeft (44), 14.0f, Theme::text, Justification::centredLeft, false, 0.08f);
        g.setColour (Theme::textDim);
        g.drawHorizontalLine ((int) b.getCentreY(), b.getRight() - 22, b.getRight() - 14);
        return;
    }

    // RANDOMIZE
    g.setColour (Theme::orange.withAlpha (hovered ? 0.28f : 0.18f));
    g.fillRoundedRectangle (b.expanded (3), 8);
    g.setGradientFill (ColourGradient (pressed ? Colour (0xff3a210a) : Colour (0xff2a1a0b), b.getX(), b.getY(),
                                       Colour (0xff120b05), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 6);
    g.setColour (Theme::orange.withAlpha (0.95f));
    g.drawRoundedRectangle (b.reduced (0.5f), 6, 1.6f);
    g.setColour (Theme::orange.withAlpha (0.35f));
    g.drawRoundedRectangle (b.reduced (4), 4, 0.8f);
    drawDice (g, { b.getX() + 42, b.getCentreY() }, 15.0f, Theme::orange);
    Theme::drawText (g, text, b.withTrimmedLeft (76), 17.0f, Theme::cream, Justification::centredLeft, false, 0.1f);
}

//==============================================================================
PresetList::PresetList (GrainLabProcessor& p) : proc (p)
{
    setMouseCursor (MouseCursor::PointingHandCursor);
}

int PresetList::rowAt (Point<float> pt) const
{
    if (pt.y < headerH) return -1;
    const int i = (int) ((pt.y - headerH) / rowH);
    return isPositiveAndBelow (i, (int) getFactoryPresets().size()) ? i : -1;
}

void PresetList::select (int index)
{
    proc.applyPreset (index);
    repaint();
}

void PresetList::mouseDown (const MouseEvent& e)
{
    const int n = (int) getFactoryPresets().size();
    if (e.position.y < headerH)
    {
        const int cur = proc.getPresetIndex();
        if (e.position.x > getWidth() - 24)      select (cur < 0 ? 0 : (cur + 1) % n);
        else if (e.position.x > getWidth() - 44) select (cur <= 0 ? n - 1 : cur - 1);
        return;
    }
    const int r = rowAt (e.position);
    if (r >= 0) select (r);
}

void PresetList::mouseMove (const MouseEvent& e)
{
    const int r = rowAt (e.position);
    if (r != hover) { hover = r; repaint(); }
}

void PresetList::paint (Graphics& g)
{
    const float w = (float) getWidth();
    Theme::drawText (g, "PRESETS", { 10, 6, w - 60, 24 }, 14.0f, Theme::text, Justification::centredLeft, false, 0.1f);
    Theme::drawText (g, "<", { w - 44, 6, 18, 24 }, 14.0f, Theme::textDim, Justification::centred);
    Theme::drawText (g, ">", { w - 24, 6, 18, 24 }, 14.0f, Theme::textDim, Justification::centred);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawHorizontalLine ((int) headerH - 4, 4, w - 4);

    const auto& presets = getFactoryPresets();
    const int cur = proc.getPresetIndex();
    for (int i = 0; i < (int) presets.size(); ++i)
    {
        auto r = Rectangle<float> (6, headerH + i * rowH, w - 12, rowH - 3);
        if (i == cur)
        {
            g.setColour (Colour (0xff4a3212).withAlpha (0.9f));
            g.fillRect (r);
            g.setColour (Theme::orange.withAlpha (0.85f));
            g.drawRect (r, 1.0f);
        }
        else if (i == hover)
        {
            g.setColour (Colours::white.withAlpha (0.04f));
            g.fillRect (r);
        }
        Theme::drawText (g, presets[(size_t) i].name, r.withTrimmedLeft (8), 13.0f,
                         i == cur ? Theme::cream : Theme::textDim, Justification::centredLeft, false, 0.06f);
    }
}
