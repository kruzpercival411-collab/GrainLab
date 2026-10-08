#include "Displays.h"
#include "../PluginProcessor.h"

using namespace juce;

namespace
{
    float toDb (float lin) { return lin > 1.0e-5f ? 20.0f * std::log10 (lin) : -100.0f; }

    String formatSeconds (double s) { return String (s, s < 10.0 ? 2 : 1) + "s"; }
}

//==============================================================================
WaveformView::WaveformView (GrainLabProcessor& p)
    : proc (p), positionParam (p.apvts.getParameter (PID::position))
{
    setMouseCursor (MouseCursor::CrosshairCursor);
}

Rectangle<float> WaveformView::waveArea() const { return { 16.0f, 36.0f, (float) getWidth() - 44.0f, 214.0f }; }

void WaveformView::tick()
{
    meterL = jmax (proc.uiMeters.outL, meterL * 0.82f);
    meterR = jmax (proc.uiMeters.outR, meterR * 0.82f);
    repaint();
}

void WaveformView::setPositionFromX (float x)
{
    const auto wa = waveArea();
    positionParam->setValueNotifyingHost (jlimit (0.0f, 1.0f, (x - wa.getX()) / wa.getWidth()));
}

void WaveformView::mouseDown (const MouseEvent& e)
{
    if (proc.getUISample() == nullptr) return;
    dragging = true;
    positionParam->beginChangeGesture();
    setPositionFromX (e.position.x);
}

void WaveformView::mouseDrag (const MouseEvent& e) { if (dragging) setPositionFromX (e.position.x); }

void WaveformView::mouseUp (const MouseEvent&)
{
    if (dragging) positionParam->endChangeGesture();
    dragging = false;
}

void WaveformView::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto wa = waveArea();

    // CRT glass
    g.setGradientFill (ColourGradient (Colour (0xff0c2522), b.getCentreX(), b.getCentreY(),
                                       Colour (0xff030807), b.getX(), b.getY(), true));
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (Colours::black.withAlpha (0.22f));
    for (float y = 0; y < b.getHeight(); y += 3.0f) g.drawHorizontalLine ((int) y, 0, b.getWidth());

    // grid
    g.setColour (Theme::teal.withAlpha (0.07f));
    for (int i = 0; i <= 16; ++i) g.drawVerticalLine ((int) (wa.getX() + wa.getWidth() * i / 16.0f), wa.getY(), wa.getBottom());
    for (int i = 0; i <= 4; ++i)  g.drawHorizontalLine ((int) (wa.getY() + wa.getHeight() * i / 4.0f), wa.getX(), wa.getRight());
    g.setColour (Theme::teal.withAlpha (0.18f));
    g.drawRect (wa, 1.0f);

    const auto* s = proc.getUISample();
    const float pos = proc.apvts.getRawParameterValue (PID::position)->load();
    const float density = proc.apvts.getRawParameterValue (PID::density)->load();

    Theme::drawText (g, "GRAIN FIELD", { 20, 8, 260, 22 }, 14.0f, Theme::text, Justification::centredLeft, false, 0.12f);
    Theme::drawText (g, "POSITION " + String (pos * 100.0f, 1) + "%     DENSITY " + String (density, 1) + " G/S",
                     { b.getWidth() - 440, 8, 410, 22 }, 13.0f, Theme::teal, Justification::centredRight, false, 0.06f);

    // output meter strip on the right
    for (int ch = 0; ch < 2; ++ch)
    {
        const float lvl = jlimit (0.0f, 1.0f, (toDb (ch == 0 ? meterL : meterR) + 48.0f) / 48.0f);
        auto bar = Rectangle<float> (b.getWidth() - 22 + ch * 8, wa.getY(), 5, wa.getHeight());
        g.setColour (Colours::black.withAlpha (0.6f)); g.fillRect (bar);
        g.setColour (Theme::teal.withAlpha (0.85f));
        g.fillRect (bar.withTop (bar.getBottom() - bar.getHeight() * lvl));
    }
    Theme::drawText (g, "L", { b.getWidth() - 24, wa.getY() - 18, 10, 14 }, 10.0f, Theme::teal, Justification::centred);
    Theme::drawText (g, "R", { b.getWidth() - 15, wa.getY() - 18, 10, 14 }, 10.0f, Theme::teal, Justification::centred);

    if (s == nullptr)
    {
        const String msg = proc.isLoading() ? "LOADING SAMPLE..." : "DROP A SAMPLE HERE  -  OR CLICK  LOAD SAMPLE";
        Theme::drawText (g, msg, wa, 16.0f, Theme::teal.withAlpha (0.8f), Justification::centred, false, 0.12f);
        if (proc.getLastError().isNotEmpty())
            Theme::drawText (g, proc.getLastError(), wa.translated (0, 30), 13.0f, Theme::orange, Justification::centred);
        Theme::drawText (g, "0.0s", { wa.getX(), wa.getBottom() + 4, 80, 16 }, 12.0f, Theme::textDim, Justification::centredLeft);
        return;
    }

    // grain region (where grains are drawn from) = cloud centre +/- spread
    const float centre = proc.engine.regionCentre.load();
    const float spread = proc.apvts.getRawParameterValue (PID::spread)->load();
    auto drawRegion = [&] (float a, float bnorm)
    {
        const float x0 = wa.getX() + jlimit (0.0f, 1.0f, a) * wa.getWidth();
        const float x1 = wa.getX() + jlimit (0.0f, 1.0f, bnorm) * wa.getWidth();
        if (x1 - x0 < 1.0f) return;
        g.setColour (Theme::teal.withAlpha (0.10f));
        g.fillRect (x0, wa.getY(), x1 - x0, wa.getHeight());
    };
    float rA = centre - spread * 0.5f, rB = centre + spread * 0.5f;
    const float minW = 0.004f;
    if (rB - rA < minW) { rA = centre - minW; rB = centre + minW; }
    if (centre >= 0)
    {
        drawRegion (rA, rB);
        if (rA < 0) drawRegion (1.0f + rA, 1.0f);
        if (rB > 1) drawRegion (0.0f, rB - 1.0f);
        g.setColour (Theme::teal.withAlpha (0.6f));
        for (float edge : { rA, rB })
        {
            const float e = edge < 0 ? edge + 1.0f : (edge > 1 ? edge - 1.0f : edge);
            g.drawVerticalLine ((int) (wa.getX() + e * wa.getWidth()), wa.getY(), wa.getBottom());
        }
    }

    // waveform from precomputed peaks
    const int cols = (int) wa.getWidth();
    const float mid = wa.getCentreY(), half = wa.getHeight() * 0.46f;
    for (int x = 0; x < cols; ++x)
    {
        const int p0 = x * SampleData::numPeaks / cols;
        const int p1 = jmax (p0 + 1, (x + 1) * SampleData::numPeaks / cols);
        float lo = 0, hi = 0;
        for (int p = p0; p < p1; ++p) { lo = jmin (lo, s->peakMin[(size_t) p]); hi = jmax (hi, s->peakMax[(size_t) p]); }

        const float xn = (float) x / cols;
        bool inRegion = centre >= 0 && ((xn >= rA && xn <= rB) || (xn <= rB - 1.0f) || (xn >= rA + 1.0f));
        g.setColour (inRegion ? Theme::teal : Theme::teal.withAlpha (0.55f));
        g.fillRect (wa.getX() + x, mid - hi * half, 1.0f, jmax (1.0f, (hi - lo) * half));
    }

    // live grains (real engine state)
    for (int i = 0; i < GrainEngine::maxGrains; ++i)
    {
        const auto& gv = proc.engine.grainViews[(size_t) i];
        const float gp = gv.pos.load (std::memory_order_relaxed);
        if (gp < 0) continue;
        const float a = jlimit (0.15f, 1.0f, gv.amp.load (std::memory_order_relaxed) * 1.6f);
        const float x = wa.getX() + gp * wa.getWidth();
        const auto col = gv.reversed.load (std::memory_order_relaxed) ? Colour (0xffffd27a) : Theme::orange;
        const float t1 = wa.getY() + 6 + (float) ((i * 37) % 23) * 1.6f;
        const float b1 = wa.getBottom() - 6 - (float) ((i * 53) % 19) * 1.8f;
        g.setColour (col.withAlpha (a * 0.55f));
        g.drawLine (x, t1, x, b1, 1.0f);
        g.setColour (col.withAlpha (a));
        g.fillEllipse (x - 2.5f, t1 - 2.5f, 5, 5);
        g.fillEllipse (x - 2.5f, b1 - 2.5f, 5, 5);
    }

    // playhead
    const float ph = proc.engine.playheadNorm.load();
    if (ph >= 0)
    {
        const float x = wa.getX() + ph * wa.getWidth();
        g.setColour (Colours::white.withAlpha (0.15f));
        g.fillRect (x - 2.0f, wa.getY() - 18, 4.0f, wa.getHeight() + 36);
        g.setColour (Colours::white.withAlpha (0.9f));
        g.drawLine (x, wa.getY() - 18, x, wa.getBottom() + 18, 1.2f);
    }

    // Position knob marker
    {
        const float x = wa.getX() + pos * wa.getWidth();
        Path tri;
        tri.addTriangle (x - 5, wa.getBottom() + 1, x + 5, wa.getBottom() + 1, x, wa.getBottom() - 7);
        g.setColour (Theme::orange);
        g.fillPath (tri);
    }

    Theme::drawText (g, "0.0s", { wa.getX() + 4, wa.getBottom() - 20, 80, 16 }, 12.0f, Theme::text, Justification::centredLeft);
    Theme::drawText (g, formatSeconds (s->lengthSeconds()), { wa.getRight() - 84, wa.getBottom() - 20, 80, 16 }, 12.0f,
                     Theme::text, Justification::centredRight);

    // bottom timeline strip with grain ticks
    const float sy = wa.getBottom() + 36;
    g.setColour (Theme::teal.withAlpha (0.25f));
    g.drawHorizontalLine ((int) sy, wa.getX() + 40, wa.getRight() - 40);
    const float sx0 = wa.getX() + 40, sw = wa.getWidth() - 80;
    for (int i = 0; i < GrainEngine::maxGrains; ++i)
    {
        const auto& gv = proc.engine.grainViews[(size_t) i];
        const float gp = gv.pos.load (std::memory_order_relaxed);
        if (gp < 0) continue;
        const bool rev = gv.reversed.load (std::memory_order_relaxed);
        g.setColour ((rev ? Theme::teal : Theme::orange).withAlpha (0.8f));
        const float h = 6.0f + 10.0f * (1.0f - gv.progress.load (std::memory_order_relaxed));
        g.fillRect (sx0 + gp * sw - 1.5f, sy - h * 0.5f, 3.0f, h);
    }
    if (ph >= 0)
    {
        g.setColour (Colours::white);
        g.fillRect (sx0 + ph * sw - 1.0f, sy - 9, 2.0f, 18.0f);
    }
}

//==============================================================================
VUMeter::VUMeter (GrainLabProcessor& p, bool isOutput, String l) : proc (p), output (isOutput), label (std::move (l))
{
    setInterceptsMouseClicks (false, false);
}

void VUMeter::tick()
{
    const float lvl = output ? jmax (proc.uiMeters.outL, proc.uiMeters.outR)
                             : jmax (proc.uiMeters.inL, proc.uiMeters.inR);
    const float target = jlimit (-40.0f, 6.0f, toDb (lvl) + 6.0f);   // 0 VU = -6 dBFS
    needleDb += (target - needleDb) * (target > needleDb ? 0.35f : 0.10f);
    repaint();
}

void VUMeter::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0d0e));
    g.fillRoundedRectangle (b, 4);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (b.reduced (0.5f), 4, 1.2f);

    auto face = b.reduced (7.0f);
    g.setGradientFill (ColourGradient (Colour (0xfff7e4ae), face.getCentreX(), face.getBottom() - 10,
                                       Colour (0xff9c6e34), face.getX(), face.getY(), true));
    g.fillRoundedRectangle (face, 3);

    const auto pivot = Point<float> (face.getCentreX(), face.getBottom() + face.getHeight() * 0.18f);
    const float R = face.getHeight() * 0.98f;
    const float maxA = degreesToRadians (46.0f);
    auto dbToAngle = [maxA] (float db)
    {
        const float lo = std::pow (10.0f, -20.0f / 20.0f), hi = std::pow (10.0f, 3.0f / 20.0f);
        const float f = (std::pow (10.0f, jlimit (-40.0f, 6.0f, db) / 20.0f) - lo) / (hi - lo);
        return -maxA + jlimit (-0.05f, 1.06f, f) * 2.0f * maxA;
    };

    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (face.toNearestInt());

    // red zone
    {
        Path red;
        red.addCentredArc (pivot.x, pivot.y, R * 0.83f, R * 0.83f, 0, dbToAngle (0), dbToAngle (3), true);
        g.setColour (Theme::red.withAlpha (0.85f));
        g.strokePath (red, PathStrokeType (3.0f));
    }
    Path arc;
    arc.addCentredArc (pivot.x, pivot.y, R * 0.80f, R * 0.80f, 0, -maxA, maxA, true);
    g.setColour (Colour (0xff2a1c0c));
    g.strokePath (arc, PathStrokeType (1.0f));

    const int marks[] = { -20, -10, -7, -5, -3, -1, 0, 1, 2, 3 };
    for (int db : marks)
    {
        const float a = dbToAngle ((float) db);
        const float r0 = R * 0.80f, r1 = R * (db % 5 == 0 || db == -3 ? 0.88f : 0.85f);
        g.setColour (db > 0 ? Theme::red : Colour (0xff2a1c0c));
        g.drawLine (pivot.x + std::sin (a) * r0, pivot.y - std::cos (a) * r0,
                    pivot.x + std::sin (a) * r1, pivot.y - std::cos (a) * r1, 1.0f);
        if (db == -20 || db == -10 || db == -5 || db == -3 || db == 0 || db == 3)
        {
            const float rt = R * 0.95f;
            Theme::drawText (g, db > 0 ? "+" + String (db) : String (db),
                             Rectangle<float> (22, 12).withCentre ({ pivot.x + std::sin (a) * rt, pivot.y - std::cos (a) * rt }),
                             9.0f, db > 0 ? Theme::red : Colour (0xff3a2810), Justification::centred);
        }
    }
    Theme::drawText (g, "dB", Rectangle<float> (40, 14).withCentre ({ pivot.x, face.getY() + face.getHeight() * 0.58f }),
                     11.0f, Colour (0xff3a2810), Justification::centred);
    Theme::drawText (g, label, Rectangle<float> (face.getWidth(), 14).withCentre ({ pivot.x, face.getBottom() - 9 }),
                     10.5f, Colour (0xff2a1c0c), Justification::centred, false, 0.1f);

    // needle
    const float a = dbToAngle (needleDb);
    g.setColour (Colours::black.withAlpha (0.25f));
    g.drawLine (pivot.x + 2, pivot.y, pivot.x + 2 + std::sin (a) * R * 0.93f, pivot.y - std::cos (a) * R * 0.93f, 2.0f);
    g.setColour (Colour (0xff141010));
    g.drawLine (pivot.x, pivot.y, pivot.x + std::sin (a) * R * 0.93f, pivot.y - std::cos (a) * R * 0.93f, 1.3f);

    // glass vignette
    g.setGradientFill (ColourGradient (Colours::transparentBlack, face.getCentreX(), face.getCentreY(),
                                       Colours::black.withAlpha (0.35f), face.getX(), face.getY(), true));
    g.fillRect (face);
}

//==============================================================================
LedMeter::LedMeter (GrainLabProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }

void LedMeter::tick()
{
    levelL = jmax (proc.uiMeters.outL, levelL * 0.8f);
    levelR = jmax (proc.uiMeters.outR, levelR * 0.8f);
    const int ce = proc.clipEvents.load();
    if (ce != lastClipEvents) { clipHold = 30; lastClipEvents = ce; }
    else if (clipHold > 0) --clipHold;
    repaint();
}

void LedMeter::paint (Graphics& g)
{
    const float w = (float) getWidth() - 70.0f;
    const int segs = 46;
    const float segW = w / segs;
    for (int row = 0; row < 2; ++row)
    {
        const float lvl = jlimit (0.0f, 1.0f, (toDb (row == 0 ? levelL : levelR) + 48.0f) / 48.0f);
        const float y = 4.0f + row * 11.0f;
        for (int i = 0; i < segs; ++i)
        {
            const float f = (float) i / segs;
            const bool lit = f < lvl;
            const Colour base = f > 0.94f ? Theme::red : (f > 0.8f ? Theme::orange : Colour (0xff5fe6a8));
            g.setColour (lit ? base : base.withAlpha (0.12f));
            g.fillRect (i * segW, y, segW - 2.0f, 8.0f);
        }
    }
    Theme::drawLED (g, { w + 18, 13 }, 4.5f, Theme::red, clipHold > 0);
    Theme::drawText (g, "CLIP", { w + 28, 4, 40, 18 }, 11.5f, clipHold > 0 ? Theme::red : Theme::textDim, Justification::centredLeft);
}

//==============================================================================
GrainCounter::GrainCounter (GrainLabProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }

void GrainCounter::tick()
{
    current = proc.engine.activeGrains.load();
    history[(size_t) writePos] = (float) current / GrainEngine::maxGrains;
    writePos = (writePos + 1) % (int) history.size();
    repaint();
}

void GrainCounter::paint (Graphics& g)
{
    Theme::drawText (g, "ACTIVE GRAINS", { 18, 8, 200, 16 }, 12.0f, Theme::textDim, Justification::centredLeft, false, 0.08f);
    Theme::drawText (g, String (current) + " / " + String (GrainEngine::maxGrains), { 18, 30, 100, 24 }, 18.0f,
                     Theme::teal, Justification::centredLeft);

    const float x0 = 112, x1 = (float) getWidth() - 10, y1 = (float) getHeight() - 10, hMax = 30;
    const int n = (int) history.size();
    const float bw = (x1 - x0) / n;
    for (int i = 0; i < n; ++i)
    {
        const float v = history[(size_t) ((writePos + i) % n)];
        const float h = jmax (1.0f, v * hMax);
        g.setColour (Theme::teal.withAlpha (0.25f + 0.6f * v));
        g.fillRect (x0 + i * bw, y1 - h, jmax (1.0f, bw - 1.0f), h);
    }
}

//==============================================================================
RadarView::RadarView (GrainLabProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }

void RadarView::paint (Graphics& g)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    const float R = jmin (getWidth(), getHeight()) * 0.42f;

    g.setColour (Theme::textFaint.withAlpha (0.6f));
    for (float k : { 1.0f, 0.72f, 0.45f })
        g.drawEllipse (c.x - R * k, c.y - R * k, R * k * 2, R * k * 2, 0.8f);
    g.drawLine (c.x - R, c.y, c.x + R, c.y, 0.5f);
    g.drawLine (c.x, c.y - R, c.x, c.y + R, 0.5f);
    for (int i = 0; i < 72; ++i)
    {
        const float a = i * MathConstants<float>::twoPi / 72.0f;
        const float r0 = R * (i % 6 == 0 ? 1.06f : 1.03f);
        g.drawLine (c.x + std::cos (a) * R, c.y + std::sin (a) * R, c.x + std::cos (a) * r0, c.y + std::sin (a) * r0, 0.7f);
    }

    // grains: angle = position in sample, radius = progress through the grain
    for (int i = 0; i < GrainEngine::maxGrains; ++i)
    {
        const auto& gv = proc.engine.grainViews[(size_t) i];
        const float gp = gv.pos.load (std::memory_order_relaxed);
        if (gp < 0) continue;
        const float a = gp * MathConstants<float>::twoPi - MathConstants<float>::halfPi;
        const float r = 20.0f + gv.progress.load (std::memory_order_relaxed) * (R - 22.0f);
        const float amp = jlimit (0.2f, 1.0f, gv.amp.load (std::memory_order_relaxed) * 1.6f);
        const auto col = gv.reversed.load (std::memory_order_relaxed) ? Theme::teal : Theme::orange;
        g.setColour (col.withAlpha (amp));
        const float d = 2.0f + amp * 3.0f;
        g.fillEllipse (c.x + std::cos (a) * r - d * 0.5f, c.y + std::sin (a) * r - d * 0.5f, d, d);
    }

    const float ph = proc.engine.playheadNorm.load();
    if (ph >= 0)
    {
        const float a = ph * MathConstants<float>::twoPi - MathConstants<float>::halfPi;
        g.setColour (Colours::white.withAlpha (0.35f));
        g.drawLine (c.x, c.y, c.x + std::cos (a) * R, c.y + std::sin (a) * R, 1.0f);
        g.setColour (Theme::text);
        g.drawRect (Rectangle<float> (7, 7).withCentre ({ c.x + std::cos (a) * R, c.y + std::sin (a) * R }), 1.0f);
    }

    // hub
    const float hr = 18.0f;
    g.setGradientFill (ColourGradient (Colour (0xff2a3032), c.x - hr, c.y - hr, Colour (0xff050606), c.x + hr, c.y + hr, false));
    g.fillEllipse (c.x - hr, c.y - hr, hr * 2, hr * 2);
    g.setColour (Colours::black);
    g.drawEllipse (c.x - hr, c.y - hr, hr * 2, hr * 2, 1.5f);
    g.setColour (Theme::textFaint);
    g.drawEllipse (c.x - hr * 0.5f, c.y - hr * 0.5f, hr, hr, 0.8f);
}

//==============================================================================
OrbitView::OrbitView (GrainLabProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }

void OrbitView::paint (Graphics& g)
{
    const Point<float> c (getWidth() * 0.4f, getHeight() * 0.55f);
    const auto line = Theme::textDim.withAlpha (0.55f);

    for (int k = 0; k < 3; ++k)
    {
        Path orbit;
        const float rx = 95.0f + k * 22.0f, ry = 34.0f + k * 9.0f;
        orbit.addEllipse (-rx, -ry, rx * 2, ry * 2);
        orbit.applyTransform (AffineTransform::rotation (-0.28f + k * 0.12f).translated (c.x, c.y));
        g.setColour (line.withAlpha (0.35f - k * 0.08f));
        g.strokePath (orbit, PathStrokeType (0.8f));
    }
    Theme::drawPlanet (g, c, 46.0f, Theme::textDim, -0.28f);

    const float ph = jmax (0.0f, proc.engine.playheadNorm.load());
    const float a = ph * MathConstants<float>::twoPi;
    const Point<float> dot (c.x + std::cos (a) * 117.0f, c.y + std::sin (a) * 43.0f);
    const auto rotated = dot.rotatedAboutOrigin (0.0f);
    g.setColour (Theme::orange.withAlpha (proc.engine.playheadNorm.load() >= 0 ? 0.9f : 0.3f));
    g.fillEllipse (rotated.x - 3, rotated.y - 3, 6, 6);
    g.setColour (Theme::text.withAlpha (0.6f));
    g.drawEllipse (c.x + 70, c.y - 70, 7, 7, 0.8f);
    g.fillEllipse (c.x - 120, c.y + 18, 4, 4);

    Theme::drawText (g, "GRAIN LAB", { (float) getWidth() - 110, c.y + 10, 100, 16 }, 11.0f, Theme::textDim,
                     Justification::centredLeft, false, 0.2f);
}

//==============================================================================
StatusView::StatusView (GrainLabProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }

void StatusView::tick()
{
    const bool active = proc.engine.activeGrains.load() > 0;
    if (active != lastActive) { lastActive = active; repaint(); }
}

void StatusView::paint (Graphics& g)
{
    const auto* s = proc.getUISample();
    const float rowH = 21.0f;
    auto row = [&] (int i, float x, const String& key, const String& value, Colour vc)
    {
        Theme::drawText (g, key, { x, 8 + i * rowH, 120, rowH }, 13.0f, Theme::textDim, Justification::centredLeft);
        Theme::drawText (g, value, { x + (key.length() * 8.0f) + 6, 8 + i * rowH, 220, rowH }, 13.0f, vc, Justification::centredLeft);
    };
    const String dot = String (CharPointer_UTF8 ("\xe2\x80\xa2 "));
    row (0, 16, "SYSTEM:", dot + "ONLINE", Theme::teal);
    row (1, 16, "GRAIN ENGINE:", dot + (lastActive ? "ACTIVE" : "IDLE"), lastActive ? Theme::teal : Theme::textDim);
    row (2, 16, "SAMPLE:", s ? s->fileName.toUpperCase().substring (0, 18) : String ("NONE"), Theme::teal);

    const float x2 = 268;
    row (0, x2, "SR:", s ? String (s->sampleRate / 1000.0, 1) + "kHz" : "--", Theme::text);
    row (1, x2, "LENGTH:", s ? String (s->lengthSeconds(), 1) + "s" : "--", Theme::text);
    row (2, x2, "SIZE:", s ? String ((double) s->fileSize / (1024.0 * 1024.0), 1) + "MB" : "--", Theme::text);
}

//==============================================================================
SampleCard::SampleCard (GrainLabProcessor& p) : proc (p) {}

void SampleCard::mouseMove (const MouseEvent& e)
{
    const bool h = closeBounds().contains (e.position) && proc.getUISample() != nullptr;
    if (h != hoverClose) { hoverClose = h; repaint(); }
    setMouseCursor (h ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
}

void SampleCard::mouseDown (const MouseEvent& e)
{
    if (closeBounds().contains (e.position) && proc.getUISample() != nullptr)
        proc.clearSample();
}

void SampleCard::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0a0d0d));
    g.fillRoundedRectangle (b, 3);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (b.reduced (0.5f), 3, 1.0f);

    const auto* s = proc.getUISample();
    const String name = proc.isLoading() ? "LOADING..." : (s ? s->fileName.toUpperCase() : "NO SAMPLE");
    Theme::drawText (g, name, { 10, 4, b.getWidth() - 36, 20 }, 12.5f, Theme::text, Justification::centredLeft);
    if (s)
        Theme::drawText (g, "x", closeBounds(), 13.0f, hoverClose ? Theme::orange : Theme::textDim, Justification::centred);
    g.setColour (Colours::black.withAlpha (0.7f));
    g.drawHorizontalLine (26, 4, b.getWidth() - 4);

    if (s == nullptr)
    {
        if (proc.getLastError().isNotEmpty())
            Theme::drawText (g, proc.getLastError(), { 10, 32, b.getWidth() - 20, 60 }, 11.0f, Theme::orange, Justification::topLeft);
        else
            Theme::drawText (g, "WAV / AIFF / MP3 / FLAC", { 10, 32, b.getWidth() - 20, 18 }, 11.0f, Theme::textDim, Justification::centredLeft);
        return;
    }

    const String info = String (s->lengthSeconds(), 1) + " s   " + String (s->sampleRate / 1000.0, 1) + " kHz   "
                      + (s->bitDepth > 0 ? String (s->bitDepth) + " bit" : String());
    Theme::drawText (g, info, { 10, 30, b.getWidth() - 20, 18 }, 11.5f, Theme::text.withAlpha (0.8f), Justification::centredLeft);

    const auto wave = Rectangle<float> (8, 54, b.getWidth() - 16, b.getHeight() - 60);
    const int cols = (int) wave.getWidth();
    g.setColour (Theme::teal.withAlpha (0.85f));
    for (int x = 0; x < cols; ++x)
    {
        const int p0 = x * SampleData::numPeaks / cols;
        const int p1 = jmax (p0 + 1, (x + 1) * SampleData::numPeaks / cols);
        float lo = 0, hi = 0;
        for (int p = p0; p < p1; ++p) { lo = jmin (lo, s->peakMin[(size_t) p]); hi = jmax (hi, s->peakMax[(size_t) p]); }
        g.fillRect (wave.getX() + x, wave.getCentreY() - hi * wave.getHeight() * 0.5f, 1.0f,
                    jmax (1.0f, (hi - lo) * wave.getHeight() * 0.5f));
    }
}

//==============================================================================
TransportView::TransportView (GrainLabProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }

void TransportView::tick()
{
    progress = jmax (0.0f, proc.engine.playheadNorm.load());
    active = proc.engine.activeGrains.load() > 0;
    midiFlash = proc.engine.midiActivity.exchange (false) || (midiFlash && Random::getSystemRandom().nextFloat() < 0.6f);
    repaint();
}

void TransportView::paint (Graphics& g)
{
    const float cy = getHeight() * 0.5f;
    Theme::drawLED (g, { 20, cy }, 4.0f, Theme::teal, active);
    Theme::drawLED (g, { 42, cy }, 4.0f, Theme::orange, midiFlash);

    const float x0 = 140, x1 = (float) getWidth() - 22;
    g.setColour (Colours::black.withAlpha (0.8f));
    g.fillRoundedRectangle (x0, cy - 3, x1 - x0, 6, 3);
    g.setColour (Theme::border);
    g.drawRoundedRectangle (x0, cy - 3, x1 - x0, 6, 3, 0.8f);
    const float px = x0 + progress * (x1 - x0);
    g.setGradientFill (ColourGradient (Theme::tealDim, x0, cy, Theme::teal, px, cy, false));
    g.fillRoundedRectangle (x0 + 1, cy - 2, jmax (0.0f, px - x0 - 1), 4, 2);
    g.setGradientFill (ColourGradient (Colour (0xffb5bcba), px, cy - 7, Colour (0xff3a4042), px, cy + 7, false));
    g.fillEllipse (px - 7, cy - 7, 14, 14);
    g.setColour (Colours::black);
    g.drawEllipse (px - 7, cy - 7, 14, 14, 1.0f);
}
