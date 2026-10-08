#include "Theme.h"

namespace Theme
{
juce::Font mono (float height, bool bold)
{
    static const juce::String face = []
    {
        const auto names = juce::Font::findAllTypefaceNames();
        for (auto candidate : { "Consolas", "Cascadia Mono", "Menlo", "DejaVu Sans Mono", "Liberation Mono" })
            if (names.contains (candidate)) return juce::String (candidate);
        return juce::Font::getDefaultMonospacedFontName();
    }();
    return juce::Font (juce::FontOptions (face, height, bold ? juce::Font::bold : juce::Font::plain));
}

const juce::Image& noise()
{
    static const juce::Image img = []
    {
        juce::Image im (juce::Image::ARGB, 256, 256, true);
        juce::Random r (0x5eed);
        for (int y = 0; y < 256; ++y)
            for (int x = 0; x < 256; ++x)
            {
                const auto v = (juce::uint8) r.nextInt (256);
                im.setPixelAt (x, y, juce::Colour (v, v, v).withAlpha ((juce::uint8) r.nextInt (34)));
            }
        return im;
    }();
    return img;
}

void drawNoise (juce::Graphics& g, juce::Rectangle<int> area, float alpha)
{
    g.setTiledImageFill (noise(), 0, 0, alpha);
    g.fillRect (area);
}

void drawScratches (juce::Graphics& g, juce::Rectangle<float> area, int count, int seed, float alpha)
{
    juce::Random r (seed);
    for (int i = 0; i < count; ++i)
    {
        const float x = area.getX() + r.nextFloat() * area.getWidth();
        const float y = area.getY() + r.nextFloat() * area.getHeight();
        const float len = 6.0f + r.nextFloat() * 40.0f;
        const float ang = r.nextFloat() * juce::MathConstants<float>::twoPi;
        const bool light = r.nextBool();
        g.setColour ((light ? juce::Colours::white : juce::Colours::black).withAlpha (alpha * (0.3f + r.nextFloat())));
        g.drawLine (x, y, x + std::cos (ang) * len, y + std::sin (ang) * len, 0.6f);
    }
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float corner, bool recessed)
{
    const auto top = recessed ? panel.darker (0.45f) : panelHi;
    const auto bot = recessed ? panel.darker (0.2f) : panel.darker (0.15f);
    g.setGradientFill (juce::ColourGradient (top, r.getX(), r.getY(), bot, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);

    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (r, corner);
        g.reduceClipRegion (clip);
        drawNoise (g, r.toNearestInt(), 0.45f);
        drawScratches (g, r, (int) (r.getWidth() * r.getHeight() / 9000.0f), (int) (r.getX() * 31 + r.getY()), 0.05f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawRoundedRectangle (r.expanded (1.0f), corner + 1.0f, 1.6f);
    g.setColour (recessed ? border.darker (0.3f) : border);
    g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
    if (! recessed)
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + corner, r.getRight() - corner);
    }
}

void drawScrew (juce::Graphics& g, juce::Point<float> c, float r)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3f40), c.x - r, c.y - r,
                                             juce::Colour (0xff0e1011), c.x + r, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 0.8f);
    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.drawLine (c.x - r * 0.6f, c.y + r * 0.3f, c.x + r * 0.6f, c.y - r * 0.3f, 1.0f);
}

void drawLED (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour colour, bool on)
{
    if (on)
    {
        g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.45f), c.x, c.y,
                                                 colour.withAlpha (0.0f), c.x + r * 3.2f, c.y, true));
        g.fillEllipse (c.x - r * 3.2f, c.y - r * 3.2f, r * 6.4f, r * 6.4f);
    }
    g.setColour (on ? colour : colour.withSaturation (0.3f).withBrightness (0.18f));
    g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
    if (on)
    {
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.fillEllipse (c.x - r * 0.4f, c.y - r * 0.45f, r * 0.7f, r * 0.6f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 0.8f);
}

void drawIcon (juce::Graphics& g, Icon icon, juce::Rectangle<float> r, juce::Colour colour)
{
    g.setColour (colour);
    const auto c = r.getCentre();
    const float s = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    juce::Path p;

    switch (icon)
    {
        case Icon::Grain:
            g.drawEllipse (c.x - s * 0.7f, c.y - s * 0.7f, s * 1.4f, s * 1.4f, 1.3f);
            g.fillEllipse (c.x - 2, c.y - 2, 4, 4);
            for (int i = 0; i < 4; ++i)
            {
                const float a = i * juce::MathConstants<float>::halfPi;
                g.drawLine (c.x + std::cos (a) * s * 0.7f, c.y + std::sin (a) * s * 0.7f,
                            c.x + std::cos (a) * s, c.y + std::sin (a) * s, 1.3f);
            }
            break;
        case Icon::Motion:
            p.startNewSubPath (c.x - s, c.y);
            p.lineTo (c.x - s * 0.5f, c.y);
            p.lineTo (c.x - s * 0.25f, c.y - s * 0.8f);
            p.lineTo (c.x + s * 0.15f, c.y + s * 0.8f);
            p.lineTo (c.x + s * 0.45f, c.y - s * 0.3f);
            p.lineTo (c.x + s * 0.6f, c.y);
            p.lineTo (c.x + s, c.y);
            g.strokePath (p, juce::PathStrokeType (1.4f));
            break;
        case Icon::Random:
        {
            const float h = s * 0.85f;
            p.startNewSubPath (c.x, c.y - h);
            p.lineTo (c.x + h, c.y - h * 0.5f); p.lineTo (c.x + h, c.y + h * 0.5f);
            p.lineTo (c.x, c.y + h); p.lineTo (c.x - h, c.y + h * 0.5f); p.lineTo (c.x - h, c.y - h * 0.5f);
            p.closeSubPath();
            p.startNewSubPath (c.x - h, c.y - h * 0.5f); p.lineTo (c.x, c.y); p.lineTo (c.x + h, c.y - h * 0.5f);
            p.startNewSubPath (c.x, c.y); p.lineTo (c.x, c.y + h);
            g.strokePath (p, juce::PathStrokeType (1.3f));
            g.fillEllipse (c.x - 1.5f, c.y - h * 0.55f, 3, 3);
            g.fillEllipse (c.x - h * 0.6f, c.y + h * 0.1f, 3, 3);
            g.fillEllipse (c.x + h * 0.45f, c.y + h * 0.1f, 3, 3);
            break;
        }
        case Icon::Envelope:
            p.startNewSubPath (c.x - s, c.y + s * 0.7f);
            p.lineTo (c.x - s * 0.45f, c.y - s * 0.7f);
            p.lineTo (c.x - s * 0.05f, c.y + s * 0.7f);
            p.startNewSubPath (c.x - s * 0.05f, c.y + s * 0.7f);
            p.quadraticTo (c.x + s * 0.45f, c.y - s * 1.3f, c.x + s, c.y + s * 0.7f);
            g.strokePath (p, juce::PathStrokeType (1.4f));
            break;
        case Icon::Time:
            g.drawEllipse (c.x - s * 0.85f, c.y - s * 0.85f, s * 1.7f, s * 1.7f, 1.3f);
            g.drawLine (c.x, c.y, c.x, c.y - s * 0.55f, 1.3f);
            g.drawLine (c.x, c.y, c.x + s * 0.4f, c.y + s * 0.2f, 1.3f);
            break;
        case Icon::None:
        default: break;
    }
}

void drawText (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float height,
               juce::Colour c, juce::Justification j, bool bold, float kerning)
{
    g.setColour (c);
    g.setFont (mono (height, bold).withExtraKerningFactor (kerning));
    g.drawText (s, r, j, false);
}

void drawSectionHeader (juce::Graphics& g, juce::Rectangle<float> pb, const juce::String& title, Icon icon)
{
    auto row = juce::Rectangle<float> (pb.getX() + 12, pb.getY() + 10, pb.getWidth() - 24, 26);
    if (icon != Icon::None)
    {
        drawIcon (g, icon, row.removeFromLeft (20).withSizeKeepingCentre (18, 18), text.withAlpha (0.85f));
        row.removeFromLeft (10);
    }
    drawText (g, title, row, 15.0f, text, juce::Justification::centredLeft, false, 0.1f);

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawHorizontalLine ((int) (pb.getY() + 42), pb.getX() + 6, pb.getRight() - 6);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawHorizontalLine ((int) (pb.getY() + 43), pb.getX() + 6, pb.getRight() - 6);
}

void drawButtonBox (juce::Graphics& g, juce::Rectangle<float> r, bool selected, bool hover, float corner)
{
    if (selected)
    {
        g.setColour (orange.withAlpha (0.18f));
        g.fillRoundedRectangle (r.expanded (2.5f), corner + 2.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a2a0c), r.getX(), r.getY(),
                                                 juce::Colour (0xff2a1706), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, corner);
        g.setColour (orange.withAlpha (0.9f));
        g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.2f);
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (hover ? juce::Colour (0xff272c2e) : juce::Colour (0xff1f2324), r.getX(), r.getY(),
                                                 juce::Colour (0xff111314), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, corner);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (r.expanded (0.6f), corner, 1.0f);
        g.setColour (hover ? borderHi : border);
        g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
    }
}

void drawPlanet (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour line, float tilt)
{
    // body with faint latitude bands
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff23292a), c.x - r * 0.5f, c.y - r * 0.6f,
                                             juce::Colour (0xff070808), c.x + r * 0.7f, c.y + r * 0.8f, true));
    g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
        g.reduceClipRegion (clip);
        g.setColour (line.withAlpha (0.12f));
        for (int i = -4; i <= 4; ++i)
        {
            const float y = c.y + i * r * 0.22f;
            g.drawLine (c.x - r, y + (float) i * 1.5f, c.x + r, y - (float) i * 1.5f, 0.7f);
        }
    }
    g.setColour (line.withAlpha (0.55f));
    g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 1.0f);

    // ring
    juce::Path ring;
    ring.addEllipse (-r * 1.7f, -r * 0.42f, r * 3.4f, r * 0.84f);
    ring.applyTransform (juce::AffineTransform::rotation (tilt).translated (c.x, c.y));
    g.setColour (line.withAlpha (0.6f));
    g.strokePath (ring, juce::PathStrokeType (1.0f));
}
}
