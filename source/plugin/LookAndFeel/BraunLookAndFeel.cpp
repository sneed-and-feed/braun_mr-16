#include "BraunLookAndFeel.h"
#include <cmath>

namespace braun::mr16
{
namespace mr16 = braun::mr16;


//==============================================================================
BraunLookAndFeel::BraunLookAndFeel()
{
    applyThemeColours();
}

void BraunLookAndFeel::setDarkTheme(bool useDarkTheme)
{
    if (darkThemeActive != useDarkTheme)
    {
        darkThemeActive = useDarkTheme;
        applyThemeColours();
    }
}

void BraunLookAndFeel::applyThemeColours()
{
    if (darkThemeActive)
    {
        setColour(BraunColours::bgAppColourId,          juce::Colour(BraunColours::Dark_BgApp));
        setColour(BraunColours::bgPanelColourId,        juce::Colour(BraunColours::Dark_BgPanel));
        setColour(BraunColours::bgPanelInsetColourId,   juce::Colour(BraunColours::Dark_BgPanelInset));
        setColour(BraunColours::bgBezelColourId,        juce::Colour(BraunColours::Dark_BgBezel));
        setColour(BraunColours::borderLineColourId,     juce::Colour(BraunColours::Dark_BorderLine));
        setColour(BraunColours::borderSubtleColourId,   juce::Colour(BraunColours::Dark_BorderSubtle));
        setColour(BraunColours::textPrimaryColourId,    juce::Colour(BraunColours::Dark_TextPrimary));
        setColour(BraunColours::textSecondaryColourId,  juce::Colour(BraunColours::Dark_TextSecondary));
        setColour(BraunColours::textMutedColourId,      juce::Colour(BraunColours::Dark_TextMuted));
        setColour(BraunColours::knobCapLightId,         juce::Colour(BraunColours::Dark_KnobCapLight));
        setColour(BraunColours::knobCapDarkId,          juce::Colour(BraunColours::Dark_KnobCapDark));
        setColour(BraunColours::knobBorderColourId,     juce::Colour(BraunColours::Dark_KnobBorder));
        setColour(BraunColours::knobIndicatorColourId,  juce::Colour(BraunColours::Dark_KnobIndicator));
        setColour(BraunColours::knobTrackColourId,      juce::Colour(BraunColours::Dark_KnobTrack));
        setColour(BraunColours::knobFillColourId,       juce::Colour(BraunColours::Dark_KnobFill));

        // JUCE standard color mappings
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(BraunColours::Dark_BgApp));
        setColour(juce::Label::textColourId,                 juce::Colour(BraunColours::Dark_TextPrimary));
        setColour(juce::TextButton::buttonColourId,          juce::Colour(BraunColours::Dark_BgPanelInset));
        setColour(juce::TextButton::buttonOnColourId,        juce::Colour(BraunColours::Accent_BraunOrange));
        setColour(juce::TextButton::textColourOffId,         juce::Colour(BraunColours::Dark_TextPrimary));
        setColour(juce::TextButton::textColourOnId,          juce::Colours::white);

        // ComboBox & PopupMenu dark palette
        setColour(juce::ComboBox::backgroundColourId,        juce::Colour(BraunColours::Dark_BgPanelInset));
        setColour(juce::ComboBox::textColourId,              juce::Colour(BraunColours::Dark_TextPrimary));
        setColour(juce::ComboBox::outlineColourId,           juce::Colour(BraunColours::Dark_BorderLine));
        setColour(juce::ComboBox::arrowColourId,             juce::Colour(BraunColours::Dark_TextSecondary));
        setColour(juce::ComboBox::focusedOutlineColourId,    juce::Colour(BraunColours::Accent_BraunOrange));

        setColour(juce::PopupMenu::backgroundColourId,            juce::Colour(BraunColours::Dark_BgPanel));
        setColour(juce::PopupMenu::textColourId,                  juce::Colour(BraunColours::Dark_TextPrimary));
        setColour(juce::PopupMenu::headerTextColourId,            juce::Colour(BraunColours::Accent_BraunOrange));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(BraunColours::Dark_BgPanelInset));
        setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colour(BraunColours::Accent_BraunOrange));
    }
    else
    {
        setColour(BraunColours::bgAppColourId,          juce::Colour(BraunColours::Light_BgApp));
        setColour(BraunColours::bgPanelColourId,        juce::Colour(BraunColours::Light_BgPanel));
        setColour(BraunColours::bgPanelInsetColourId,   juce::Colour(BraunColours::Light_BgPanelInset));
        setColour(BraunColours::bgBezelColourId,        juce::Colour(BraunColours::Light_BgBezel));
        setColour(BraunColours::borderLineColourId,     juce::Colour(BraunColours::Light_BorderLine));
        setColour(BraunColours::borderSubtleColourId,   juce::Colour(BraunColours::Light_BorderSubtle));
        setColour(BraunColours::textPrimaryColourId,    juce::Colour(BraunColours::Light_TextPrimary));
        setColour(BraunColours::textSecondaryColourId,  juce::Colour(BraunColours::Light_TextSecondary));
        setColour(BraunColours::textMutedColourId,      juce::Colour(BraunColours::Light_TextMuted));
        setColour(BraunColours::knobCapLightId,         juce::Colour(BraunColours::Light_KnobCapLight));
        setColour(BraunColours::knobCapDarkId,          juce::Colour(BraunColours::Light_KnobCapDark));
        setColour(BraunColours::knobBorderColourId,     juce::Colour(BraunColours::Light_KnobBorder));
        setColour(BraunColours::knobIndicatorColourId,  juce::Colour(BraunColours::Light_KnobIndicator));
        setColour(BraunColours::knobTrackColourId,      juce::Colour(BraunColours::Light_KnobTrack));
        setColour(BraunColours::knobFillColourId,       juce::Colour(BraunColours::Light_KnobFill));

        // JUCE standard color mappings
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(BraunColours::Light_BgApp));
        setColour(juce::Label::textColourId,                 juce::Colour(BraunColours::Light_TextPrimary));
        setColour(juce::TextButton::buttonColourId,          juce::Colour(BraunColours::Light_BgPanelInset));
        setColour(juce::TextButton::buttonOnColourId,        juce::Colour(BraunColours::Accent_BraunOrange));
        setColour(juce::TextButton::textColourOffId,         juce::Colour(BraunColours::Light_TextPrimary));
        setColour(juce::TextButton::textColourOnId,          juce::Colours::white);

        // ComboBox & PopupMenu light palette
        setColour(juce::ComboBox::backgroundColourId,        juce::Colour(BraunColours::Light_BgPanelInset));
        setColour(juce::ComboBox::textColourId,              juce::Colour(BraunColours::Light_TextPrimary));
        setColour(juce::ComboBox::outlineColourId,           juce::Colour(BraunColours::Light_BorderLine));
        setColour(juce::ComboBox::arrowColourId,             juce::Colour(BraunColours::Light_TextSecondary));
        setColour(juce::ComboBox::focusedOutlineColourId,    juce::Colour(BraunColours::Accent_BraunOrange));

        setColour(juce::PopupMenu::backgroundColourId,            juce::Colour(BraunColours::Light_BgPanel));
        setColour(juce::PopupMenu::textColourId,                  juce::Colour(BraunColours::Light_TextPrimary));
        setColour(juce::PopupMenu::headerTextColourId,            juce::Colour(BraunColours::Accent_BraunOrange));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(BraunColours::Light_BgPanelInset));
        setColour(juce::PopupMenu::highlightedTextColourId,       juce::Colour(BraunColours::Accent_BraunOrange));
    }

    setColour(BraunColours::braunOrangeColourId,  juce::Colour(BraunColours::Accent_BraunOrange));
    setColour(BraunColours::braunGreenColourId,   juce::Colour(BraunColours::Accent_BraunGreen));
    setColour(BraunColours::braunAmberColourId,   juce::Colour(BraunColours::Accent_BraunAmber));
    setColour(BraunColours::phosphorColourId,     juce::Colour(BraunColours::PhosphorGreen));
    setColour(BraunColours::phosphorGlowColourId, juce::Colour(BraunColours::PhosphorGreen).withAlpha(0.40f));
}

BraunLookAndFeel::KnobTier BraunLookAndFeel::getKnobTierForBounds(int width, int height) noexcept
{
    const int minDim = juce::jmin(width, height);
    if (minDim >= 60) return KnobTier::Hero;       // 64px
    if (minDim >= 48) return KnobTier::Secondary;  // 52px
    return KnobTier::Trim;                         // 42px
}

//==============================================================================
void BraunLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
    auto center = bounds.getCentre();
    auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    auto radius = diameter / 2.0f;

    const auto tier = getKnobTierForBounds(width, height);
    float trackStrokeWidth = (tier == KnobTier::Hero) ? 4.5f : (tier == KnobTier::Secondary) ? 4.0f : 3.0f;
    float capMargin = (tier == KnobTier::Hero) ? 9.0f : (tier == KnobTier::Secondary) ? 8.0f : 6.5f;

    auto trackRadius = radius - (trackStrokeWidth * 0.5f) - 1.0f;
    if (trackRadius <= 0.0f) return;

    auto currentAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // 1. Quiescent Background Track Arc
    juce::Path trackPath;
    trackPath.addCentredArc(center.x, center.y, trackRadius, trackRadius,
                            0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(findColour(BraunColours::knobTrackColourId));
    g.strokePath(trackPath, juce::PathStrokeType(trackStrokeWidth,
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // 2. Active Parameter Fill Arc
    if (sliderPosProportional > 0.001f)
    {
        juce::Path fillPath;
        fillPath.addCentredArc(center.x, center.y, trackRadius, trackRadius,
                               0.0f, rotaryStartAngle, currentAngle, true);

        auto fillColour = slider.isMouseOverOrDragging()
                            ? findColour(BraunColours::braunOrangeColourId)
                            : findColour(BraunColours::knobFillColourId);

        g.setColour(fillColour);
        g.strokePath(fillPath, juce::PathStrokeType(trackStrokeWidth,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // 3. Precision Turned Aluminum Cap
    auto capRadius = radius - capMargin;
    if (capRadius <= 2.0f) return;

    auto capBounds = juce::Rectangle<float>(center.x - capRadius, center.y - capRadius,
                                            capRadius * 2.0f, capRadius * 2.0f);

    // Drop shadow under cap
    g.setColour(juce::Colours::black.withAlpha(darkThemeActive ? 0.35f : 0.12f));
    g.fillEllipse(capBounds.translated(0.0f, 2.0f));

    // Radial aluminum gradient
    juce::ColourGradient capGradient(
        findColour(BraunColours::knobCapLightId), center.x - capRadius * 0.35f, center.y - capRadius * 0.35f,
        findColour(BraunColours::knobCapDarkId),  center.x + capRadius * 0.35f, center.y + capRadius * 0.35f,
        true);
    g.setGradientFill(capGradient);
    g.fillEllipse(capBounds);

    // Concentric knurling grooves
    if (tier != KnobTier::Trim)
    {
        g.setColour(findColour(BraunColours::knobBorderColourId).withAlpha(0.35f));
        g.drawEllipse(capBounds.reduced(capRadius * 0.28f), 0.75f);
        g.drawEllipse(capBounds.reduced(capRadius * 0.52f), 0.75f);
    }

    // Machined perimeter bezel rim
    g.setColour(findColour(BraunColours::knobBorderColourId));
    g.drawEllipse(capBounds, 1.0f);

    // Top specular highlight bevel
    juce::Path highlightBevel;
    highlightBevel.addCentredArc(center.x, center.y, capRadius - 0.5f, capRadius - 0.5f,
                                 0.0f, -juce::MathConstants<float>::pi * 0.75f,
                                 juce::MathConstants<float>::pi * 0.25f, true);
    g.setColour(juce::Colours::white.withAlpha(darkThemeActive ? 0.10f : 0.40f));
    g.strokePath(highlightBevel, juce::PathStrokeType(0.8f));

    // 4. Milled Indicator Notch
    float notchLength = (tier == KnobTier::Hero) ? 10.0f : (tier == KnobTier::Secondary) ? 8.0f : 6.0f;
    float notchWidth = (tier == KnobTier::Hero) ? 2.2f : 1.8f;
    float notchStart = 3.0f;

    juce::Path notch;
    notch.startNewSubPath(center.x, center.y - capRadius + notchStart);
    notch.lineTo(center.x, center.y - capRadius + notchStart + notchLength);

    auto indicatorColour = slider.isMouseOverOrDragging()
                             ? findColour(BraunColours::braunOrangeColourId)
                             : findColour(BraunColours::knobIndicatorColourId);

    g.setColour(indicatorColour);
    g.strokePath(notch,
                 juce::PathStrokeType(notchWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                 juce::AffineTransform::rotation(currentAngle, center.x, center.y));
}

//==============================================================================
void BraunLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    float tickSize = juce::jmin(bounds.getHeight() - 4.0f, 20.0f);
    float tickX = bounds.getX() + 4.0f;
    float tickY = bounds.getCentreY() - (tickSize * 0.5f);

    drawTickBox(g, button, tickX, tickY, tickSize, tickSize,
                button.getToggleState(), button.isEnabled(),
                shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    // Button label text
    auto textBounds = bounds.withTrimmedLeft(tickX + tickSize + 8.0f);
    g.setColour(findColour(BraunColours::textPrimaryColourId));
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawFittedText(button.getButtonText(), textBounds.toNearestInt(),
                     juce::Justification::centredLeft, 1);
}

void BraunLookAndFeel::drawTickBox(juce::Graphics& g, juce::Component&,
                                   float x, float y, float w, float h,
                                   bool ticked, bool isEnabled,
                                   bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    auto box = juce::Rectangle<float>(x, y, w, h);

    // Inset bezel cavity
    g.setColour(findColour(BraunColours::bgPanelInsetColourId));
    g.fillRoundedRectangle(box, 3.0f);

    g.setColour(findColour(BraunColours::borderLineColourId));
    g.drawRoundedRectangle(box, 3.0f, 1.0f);

    // Circular LED indicator
    float ledRadius = juce::jmin(w, h) * 0.28f;
    auto ledCenter = box.getCentre();
    auto ledBounds = juce::Rectangle<float>(ledCenter.x - ledRadius, ledCenter.y - ledRadius,
                                            ledRadius * 2.0f, ledRadius * 2.0f);

    if (ticked)
    {
        // Glowing Braun Orange LED
        auto glowColour = findColour(BraunColours::braunOrangeColourId).withAlpha(0.45f);
        g.setColour(glowColour);
        g.fillEllipse(ledBounds.expanded(3.0f));

        g.setColour(findColour(BraunColours::braunOrangeColourId));
        g.fillEllipse(ledBounds);

        // Core bright hotspot
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.fillEllipse(ledBounds.reduced(ledRadius * 0.45f).translated(-0.5f, -0.5f));
    }
    else
    {
        // Unlit recessed LED
        g.setColour(findColour(BraunColours::borderLineColourId));
        g.fillEllipse(ledBounds);

        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.drawEllipse(ledBounds, 0.75f);
    }
}

void BraunLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(backgroundColour);

    auto bounds = button.getLocalBounds().toFloat();
    const float cornerRadius = 3.0f;

    bool isActive = button.getToggleState() || shouldDrawButtonAsDown;

    if (isActive)
    {
        g.setColour(findColour(BraunColours::braunOrangeColourId));
        g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(findColour(BraunColours::borderLineColourId));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
    }
    else
    {
        auto fill = shouldDrawButtonAsHighlighted
                        ? findColour(BraunColours::bgPanelInsetColourId).brighter(0.05f)
                        : findColour(BraunColours::bgPanelInsetColourId);

        g.setColour(fill);
        g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(findColour(BraunColours::borderLineColourId));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
    }
}

void BraunLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                      bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsHighlighted);

    bool isActive = button.getToggleState() || shouldDrawButtonAsDown;
    auto textColour = isActive ? juce::Colours::white : findColour(BraunColours::textPrimaryColourId);

    g.setColour(textColour);
    g.setFont(getTextButtonFont(button, button.getHeight()));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds(),
                     juce::Justification::centred, 1);
}

void BraunLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));

    if (!label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        auto font = getLabelFont(label);
        auto textColour = label.findColour(juce::Label::textColourId);

        if (auto* box = dynamic_cast<juce::ComboBox*>(label.getParentComponent()))
        {
            textColour = box->findColour(juce::ComboBox::textColourId);
            font = getComboBoxFont(*box);
        }

        g.setColour(textColour.withMultipliedAlpha(alpha));
        g.setFont(font);

        auto textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());
        g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
                         juce::jmax(1, (int)((float)textArea.getHeight() / font.getHeight())),
                         label.getMinimumHorizontalScale());
    }
}

juce::Font BraunLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(juce::FontOptions(juce::jmin(11.0f, (float)buttonHeight * 0.55f),
                                        juce::Font::bold));
}

juce::Font BraunLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::Font(juce::FontOptions(10.0f, juce::Font::bold));
}

void BraunLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                    int buttonX, int buttonY, int buttonW, int buttonH,
                                    juce::ComboBox& box)
{
    juce::ignoreUnused(buttonX, buttonY, buttonW, buttonH);
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height).reduced(1.0f);
    const float cornerRadius = 3.0f;

    g.setColour(findColour(BraunColours::bgPanelInsetColourId));
    g.fillRoundedRectangle(bounds, cornerRadius);

    auto outlineColour = isButtonDown
                            ? findColour(BraunColours::braunOrangeColourId)
                            : findColour(BraunColours::borderLineColourId);
    g.setColour(outlineColour);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

    // Right-hand arrow glyph
    juce::Path arrow;
    float arrowSize = 6.0f;
    float arrowX = (float)width - 14.0f;
    float arrowY = (float)height * 0.5f - (arrowSize * 0.25f);
    arrow.startNewSubPath(arrowX - arrowSize * 0.5f, arrowY);
    arrow.lineTo(arrowX + arrowSize * 0.5f, arrowY);
    arrow.lineTo(arrowX, arrowY + arrowSize * 0.5f);
    arrow.closeSubPath();

    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.fillPath(arrow);
}

juce::Font BraunLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions(11.0f, juce::Font::bold));
}

void BraunLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    g.setColour(findColour(BraunColours::bgPanelColourId));
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(findColour(BraunColours::borderLineColourId));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void BraunLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                        bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                                        bool hasSubMenu, const juce::String& text,
                                        const juce::String& shortcutKeyText,
                                        const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        auto r = area.reduced(5, 0);
        g.setColour(findColour(BraunColours::borderLineColourId).withAlpha(0.6f));
        g.fillRect(r.removeFromTop(1));
        return;
    }

    auto itemBounds = area.toFloat();

    if (isHighlighted && isActive)
    {
        g.setColour(findColour(BraunColours::bgPanelInsetColourId));
        g.fillRoundedRectangle(itemBounds.reduced(2.0f, 1.0f), 2.0f);
    }

    juce::Colour defaultTextColour = isHighlighted
                                        ? findColour(BraunColours::braunOrangeColourId)
                                        : findColour(BraunColours::textPrimaryColourId);

    if (!isActive)
        defaultTextColour = findColour(BraunColours::textMutedColourId);

    g.setColour(textColour != nullptr ? *textColour : defaultTextColour);
    g.setFont(getPopupMenuFont());

    auto font = getPopupMenuFont();
    auto leftMargin = 12;
    if (isTicked)
    {
        auto tickBounds = juce::Rectangle<float>((float)area.getX() + 4.0f, (float)area.getY() + ((float)area.getHeight() - 6.0f) * 0.5f, 6.0f, 6.0f);
        g.setColour(findColour(BraunColours::braunOrangeColourId));
        g.fillEllipse(tickBounds);
        leftMargin = 16;
    }

    auto textBounds = area.reduced(leftMargin, 0);
    g.drawFittedText(text, textBounds, juce::Justification::centredLeft, 1);

    if (shortcutKeyText.isNotEmpty())
    {
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        g.drawFittedText(shortcutKeyText, area.reduced(10, 0), juce::Justification::centredRight, 1);
    }

    if (hasSubMenu)
    {
        juce::Path arrow;
        float arrowSize = 5.0f;
        float ax = (float)area.getRight() - 10.0f;
        float ay = (float)area.getCentreY();
        arrow.startNewSubPath(ax - arrowSize, ay - arrowSize);
        arrow.lineTo(ax, ay);
        arrow.lineTo(ax - arrowSize, ay + arrowSize);
        g.strokePath(arrow, juce::PathStrokeType(1.5f));
    }

    juce::ignoreUnused(icon);
}

juce::Font BraunLookAndFeel::getPopupMenuFont()
{
    return juce::Font(juce::FontOptions(11.0f, juce::Font::bold));
}

void BraunLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float minSliderPos, float maxSliderPos,
                                        juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos, style);

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    float trackHeight = 4.0f;
    float trackY = bounds.getCentreY() - trackHeight * 0.5f;

    // Track
    auto trackRect = juce::Rectangle<float>(bounds.getX(), trackY, bounds.getWidth(), trackHeight);
    g.setColour(findColour(BraunColours::knobTrackColourId));
    g.fillRoundedRectangle(trackRect, 2.0f);

    // Active fill
    float activeW = sliderPos - bounds.getX();
    if (activeW > 0.0f)
    {
        auto activeRect = juce::Rectangle<float>(bounds.getX(), trackY, activeW, trackHeight);
        g.setColour(slider.isMouseOverOrDragging() ? findColour(BraunColours::braunOrangeColourId)
                                                   : findColour(BraunColours::knobFillColourId));
        g.fillRoundedRectangle(activeRect, 2.0f);
    }

    // Thumb
    float thumbW = 12.0f;
    float thumbH = bounds.getHeight() - 4.0f;
    auto thumbRect = juce::Rectangle<float>(sliderPos - thumbW * 0.5f, bounds.getY() + 2.0f, thumbW, thumbH);

    g.setColour(findColour(BraunColours::knobCapLightId));
    g.fillRoundedRectangle(thumbRect, 2.0f);

    g.setColour(findColour(BraunColours::knobBorderColourId));
    g.drawRoundedRectangle(thumbRect, 2.0f, 1.0f);

    // Thumb center pip
    g.setColour(slider.isMouseOverOrDragging() ? findColour(BraunColours::braunOrangeColourId)
                                               : findColour(BraunColours::knobIndicatorColourId));
    g.fillRect(sliderPos - 0.75f, thumbRect.getY() + 4.0f, 1.5f, thumbRect.getHeight() - 8.0f);
}

} // namespace braun::mr16
