#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include <escoita_nucleo/escoita_nucleo.h>

namespace ui
{
    // Aspecto: panel de teclado portátil de los 80, plástico gris claro con tinta impresa,
    // ventana de metacrilato rojo ahumado con dígitos de siete segmentos.
    // Colores del teclado original (los mismos que el teclado 3D, sacados de la foto):
    // cuerpo negro, botones de goma gris, lista de sonidos impresa con los números
    // alternando verde agua y ámbar por columnas, rótulos verdes, display de LED rojo.
    namespace col
    {
        const juce::Colour body      { 0xff151517 };
        const juce::Colour bodyHi    { 0xff1e1f22 };
        const juce::Colour bodyLo    { 0xff0b0b0c };
        const juce::Colour line      { 0xff3b3c41 };
        const juce::Colour rubber    { 0xff4a4b50 };
        const juce::Colour rubberHi  { 0xff5b5c62 };
        const juce::Colour rubberLo  { 0xff2c2d31 };
        const juce::Colour print     { 0xffd8d5cc };
        const juce::Colour printDim  { 0xff9a9ca3 };
        const juce::Colour teal      { 0xff58c6a0 };
        const juce::Colour amber     { 0xffe9a24a };
        const juce::Colour mustard   { 0xffe3a52c };
        const juce::Colour gold      { 0xffc9a648 };
        const juce::Colour cream     { 0xffd9d6cc };
        const juce::Colour redBtn    { 0xffd8483c };
        const juce::Colour greenBtn  { 0xff2fae7d };
        const juce::Colour led       { 0xffff3b2f };
        const juce::Colour ledOff    { 0xff2b0806 };
        const juce::Colour ledBg     { 0xff120302 };
        const juce::Colour green     { 0xff45e39a };
        const juce::Colour greenOff  { 0xff10291d };
        const juce::Colour ivory     { 0xfff4f3ee };
        const juce::Colour blackKey  { 0xff1a1b1e };
    }

    juce::Font body (float size);
    juce::Font bold (float size);
    juce::Font cond (float size);
    void printed (juce::Graphics&, const juce::String&, juce::Rectangle<float>, float size, juce::Colour, juce::Justification, float tracking = 0.08f);

    struct Look : juce::LookAndFeel_V4
    {
        Look();
        juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;
        juce::Label* createSliderTextBox (juce::Slider&) override;
        void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
        juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
        void drawLabel (juce::Graphics&, juce::Label&) override;
    };

    // teclado con los nombres en español (do central = Do3)
    struct Keys : juce::MidiKeyboardComponent
    {
        using MidiKeyboardComponent::MidiKeyboardComponent;
        juce::String getWhiteNoteText (int note) override { return note % 12 == 0 ? "Do" + juce::String (note / 12 - 2) : juce::String(); }
    };

    struct VoiceButton : juce::Button
    {
        VoiceButton (int i) : juce::Button (juce::String (i)), index (i) {}
        void paintButton (juce::Graphics&, bool over, bool down) override;
        int index; bool selected = false, edited = false;
    };

    struct RegRow : juce::Component
    {
        RegRow (juce::String label, int max, std::function<juce::String (int)> fmt);
        void resized() override;
        void setValue (int v) { slider.setValue (v, juce::dontSendNotification); value.setText (fmt (v), juce::dontSendNotification); }
        juce::Label name, value;
        juce::Slider slider { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
        std::function<juce::String (int)> fmt;
        std::function<void (int)> onChange;
    };

    class PatchEditor : public juce::Component
    {
    public:
        explicit PatchEditor (BarulloProcessor&);
        void refresh();
        std::function<void()> onShowChip;
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        struct Field { int k; int sh; int m; };
        BarulloProcessor& proc;
        int part = 0, shownVoice = -1;
        juce::TextButton partA, partB, resetBtn, chipBtn;
        juce::OwnedArray<RegRow> opRows[2];
        juce::OwnedArray<juce::TextButton> opChecks[2];
        std::unique_ptr<RegRow> fbRow;
        juce::OwnedArray<juce::TextButton> chChecks;
        juce::Label hex, note;
        std::vector<Field> fields, checks;
        juce::Rectangle<int> panels[3], envArea[2], flowArea;

        void setBits (int k, int sh, int m, int val);
        void setG7 (int sh, int m, int val);
        void drawEnvelope (juce::Graphics&, juce::Rectangle<int>, int op) const;
        void drawFlow (juce::Graphics&, juce::Rectangle<int>) const;
    };

    // El chip visto desde arriba: sus 18 patas con nombre y, dentro, su memoria de registros en vivo
    class ChipView : public juce::Component
    {
    public:
        explicit ChipView (BarulloProcessor& p) : proc (p) {}
        void paint (juce::Graphics&) override;
        void tick();                         // 30 veces por segundo: lee el bus y apaga los destellos
    private:
        BarulloProcessor& proc;
        std::array<uint8_t, 128> regs {};
        std::array<uint32_t, 128> seen {};
        std::array<float, 128> flash {};
        uint32_t lastTotal = 0;
        float busFlash = 0;
        int data = 0, mask = 0;
    };

    // ESCOITA. (a ligazón á web); co clic dereito, un menú coa versión, as descargas e o aviso
    // das versións novas
    struct EscoitaLink : juce::HyperlinkButton
    {
        void mouseDown (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) juce::HyperlinkButton::mouseDown (e); else if (onMenu) onMenu(); }
        void mouseUp (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) juce::HyperlinkButton::mouseUp (e); }
        std::function<void()> onMenu;
    };

    // o aviso dunha versión nova, debaixo de ESCOITA. (unha ligazón á páxina de descargas)
    struct UpdateButton : juce::Button
    {
        UpdateButton() : juce::Button ("actualizar") {}
        void paintButton (juce::Graphics&, bool over, bool down) override;
        juce::String version;
    };

    class MainView : public juce::Component, private juce::Timer
    {
    public:
        static constexpr int W = 1000, H = 790;
        explicit MainView (BarulloProcessor&);
        ~MainView() override;
        void paint (juce::Graphics&) override;
        void resized() override;
        void timerCallback() override;
        void showPage (int p);
        void mouseDown (const juce::MouseEvent&) override;   // (global) cada clic, un paso de desfacer

    private:
        void selectVoice (int v);
        void step (int d);
        void paintDisplay (juce::Graphics&, juce::Rectangle<int>);
        void paintGrille (juce::Graphics&, juce::Rectangle<int>);

        BarulloProcessor& proc;
        Look look;
        juce::TooltipWindow tooltips { this, 500 };
        juce::Rectangle<int> displayArea, grilleArea, legendArea, chartArea, keysFrame, volArea, perfArea;
        juce::TextButton prevBtn, nextBtn, tabSounds, tabEdit, tabChip;
        juce::OwnedArray<VoiceButton> voiceBtns;
        juce::TextButton filmsBtn, speakerBtn, unlimitedBtn, eqSpeaker, eqFlat;
        // presets propios, A/B e desfacer (do núcleo común)
        juce::TextButton presetsBtn, abBtn[2], undoBtn, redoBtn;
        void showPresetsMenu();
        void afterStateChange();   // despois de desfacer, A/B ou un preset: a interface ao día
        void refreshStateKeys();
        EscoitaLink escoita;
        UpdateButton updateBtn;
        juce::SharedResourcePointer<escoita::Actualizacions> updates;
        int updateTick = 0;
        void refreshUpdate();
        void showEscoitaMenu();
        juce::Label volLabel, bendLabel, velLabel, eqLabel;
        juce::Slider volume { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
        juce::Slider bend { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
        juce::Slider vel { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
        Keys keyboard;
        PatchEditor patchEditor;
        ChipView chipView;
        int page = 0;

        using APVTS = juce::AudioProcessorValueTreeState;
        std::unique_ptr<APVTS::ButtonAttachment> filmsAtt, speakerAtt, unlimitedAtt;
        std::unique_ptr<juce::ParameterAttachment> eqAtt;
        std::unique_ptr<APVTS::SliderAttachment> volAtt, bendAtt, velAtt;

        int shownVoice = -1, shownMask = -1, shownTotal = -1, editTick = 0;
        float meter = 0, shownMeter = -1;
    };
}

class BarulloEditor : public juce::AudioProcessorEditor
{
public:
    explicit BarulloEditor (BarulloProcessor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (ui::col::body); }
    ui::MainView& view() { return main; }

private:
    BarulloProcessor& proc;
    ui::MainView main;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BarulloEditor)
};
