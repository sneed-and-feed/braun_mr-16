#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>
#include <atomic>

namespace braun::mr16
{

//==============================================================================
/**
 * @struct BraunColours
 * @brief Dieter Rams / Braun functionalist design color tokens with exact AS-42 & RB-26 parity.
 */
struct BraunColours
{
    // Colour IDs for LookAndFeel palette
    enum ColourIds
    {
        bgAppColourId               = 0x1600100,
        bgPanelColourId             = 0x1600101,
        bgPanelInsetColourId        = 0x1600102,
        bgBezelColourId             = 0x1600103,
        borderLineColourId          = 0x1600104,
        borderSubtleColourId        = 0x1600105,
        textPrimaryColourId         = 0x1600106,
        textSecondaryColourId       = 0x1600107,
        textMutedColourId           = 0x1600108,
        knobCapLightId              = 0x1600109,
        knobCapDarkId               = 0x160010A,
        knobBorderColourId          = 0x160010B,
        knobIndicatorColourId       = 0x160010C,
        knobTrackColourId           = 0x160010D,
        knobFillColourId            = 0x160010E,
        braunOrangeColourId         = 0x160010F,
        braunGreenColourId          = 0x1600110,
        braunAmberColourId          = 0x1600111,
        phosphorColourId            = 0x1600112,
        phosphorGlowColourId        = 0x1600113
    };

    // Static Color Definitions: Light Chassis (Default)
    static constexpr uint32_t Light_BgApp          = 0xFFECEBE4;
    static constexpr uint32_t Light_BgPanel        = 0xFFE2E0D8;
    static constexpr uint32_t Light_BgPanelInset   = 0xFFD7D5CC;
    static constexpr uint32_t Light_BgBezel        = 0xFF121414;
    static constexpr uint32_t Light_BorderLine     = 0xFFCBC8BD;
    static constexpr uint32_t Light_BorderSubtle   = 0xFFD8D6CD;
    static constexpr uint32_t Light_TextPrimary    = 0xFF1C1D1E;
    static constexpr uint32_t Light_TextSecondary  = 0xFF5E6064;
    static constexpr uint32_t Light_TextMuted      = 0xFF8E9094;
    static constexpr uint32_t Light_KnobCapLight   = 0xFFE0DED7;
    static constexpr uint32_t Light_KnobCapDark    = 0xFFC8C5BB;
    static constexpr uint32_t Light_KnobBorder     = 0xFFBBB8AD;
    static constexpr uint32_t Light_KnobIndicator  = 0xFF1C1D1E;
    static constexpr uint32_t Light_KnobTrack      = 0xFFD0CEC4;
    static constexpr uint32_t Light_KnobFill       = 0xFF1C1D1E;

    // Static Color Definitions: Dark Chassis (Anthracite / Matte Black)
    static constexpr uint32_t Dark_BgApp           = 0xFF141517;
    static constexpr uint32_t Dark_BgPanel         = 0xFF1E2023;
    static constexpr uint32_t Dark_BgPanelInset    = 0xFF151618;
    static constexpr uint32_t Dark_BgBezel         = 0xFF0A0B0C;
    static constexpr uint32_t Dark_BorderLine      = 0xFF3A3A3A;
    static constexpr uint32_t Dark_BorderSubtle    = 0xFF2C2E33;
    static constexpr uint32_t Dark_TextPrimary     = 0xFFF0F0F0;
    static constexpr uint32_t Dark_TextSecondary   = 0xFFBDBDBD;
    static constexpr uint32_t Dark_TextMuted       = 0xFF8E8E8E;
    static constexpr uint32_t Dark_KnobCapLight    = 0xFF35373C;
    static constexpr uint32_t Dark_KnobCapDark     = 0xFF232428;
    static constexpr uint32_t Dark_KnobBorder      = 0xFF4A4D52;
    static constexpr uint32_t Dark_KnobIndicator   = 0xFFF0F0F0;
    static constexpr uint32_t Dark_KnobTrack       = 0xFF2A2C30;
    static constexpr uint32_t Dark_KnobFill        = 0xFFEE592B; // Signature orange in dark mode

    // Signature Accent Colors (Shared)
    static constexpr uint32_t Accent_BraunOrange   = 0xFFEE592B;
    static constexpr uint32_t Accent_BraunGreen    = 0xFF24FF6A;
    static constexpr uint32_t Accent_BraunAmber    = 0xFFE5A93C;
    static constexpr uint32_t PhosphorGreen        = 0xFF24FF6A;
};

//==============================================================================
/**
 * @class BraunLookAndFeel
 * @brief Custom JUCE 8 LookAndFeel implementing Dieter Rams functionalist UI for the MR-16.
 */
class BraunLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BraunLookAndFeel();
    ~BraunLookAndFeel() override = default;

    // Theme Management
    void setDarkTheme(bool useDarkTheme);
    bool isDarkTheme() const noexcept { return darkThemeActive; }

    // Rotary Knob Hierarchy
    enum class KnobTier
    {
        Trim,       // 42px small knob (Strike velocity, Hardness, Friction, Sensitivity)
        Secondary,  // 52px medium knob (Modal damping, Spread, Coupling, Chaos, Chorus)
        Hero        // 64px large knob (Fundamental Freq, Master Volume, Dry/Wet)
    };

    static KnobTier getKnobTierForBounds(int width, int height) noexcept;

    // juce::LookAndFeel_V4 Overrides
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawTickBox(juce::Graphics& g, juce::Component& component,
                     float x, float y, float w, float h,
                     bool ticked, bool isEnabled,
                     bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    juce::Font getLabelFont(juce::Label&) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    juce::Font getComboBoxFont(juce::ComboBox&) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                           bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    juce::Font getPopupMenuFont() override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

private:
    void applyThemeColours();
    bool darkThemeActive { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BraunLookAndFeel)
};

} // namespace braun::mr16

