#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
}

namespace ui
{
// ================================================================ tipografía
static juce::Typeface::Ptr tf (int which)
{
    static juce::Typeface::Ptr t[3] = {
        juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowSemiCondensedMedium_ttf, BinaryData::BarlowSemiCondensedMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowSemiCondensedSemiBold_ttf, BinaryData::BarlowSemiCondensedSemiBold_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensedSemiBold_ttf, BinaryData::BarlowCondensedSemiBold_ttfSize) };
    return t[which];
}
juce::Font body (float s) { return juce::Font (juce::FontOptions (tf (0)).withHeight (s)); }
juce::Font bold (float s) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s)); }
juce::Font cond (float s) { return juce::Font (juce::FontOptions (tf (2)).withHeight (s)); }

// rótulo impreso en el panel: seminegrita, espaciado
void printed (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, float size, juce::Colour c, juce::Justification j, float tracking)
{
    g.setColour (c);
    g.setFont (juce::Font (juce::FontOptions (tf (1)).withHeight (size).withKerningFactor (tracking)));
    g.drawText (t, r, j, false);
}

// ================================================================ aspecto
Look::Look()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::body);
    setColour (juce::TextButton::buttonColourId, col::rubber);
    setColour (juce::TextButton::textColourOffId, col::print);
    setColour (juce::TextButton::textColourOnId, col::print);
    setColour (juce::Label::textColourId, col::printDim);
    setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxTextColourId, col::print);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, col::bodyLo);
    setColour (juce::TextEditor::textColourId, col::print);
    setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::focusedOutlineColourId, col::teal);
    setColour (juce::TextEditor::highlightColourId, col::teal.withAlpha (0.35f));
    setColour (juce::CaretComponent::caretColourId, col::print);
}

juce::Typeface::Ptr Look::getTypefaceForFont (const juce::Font& f) { return tf (f.isBold() ? 1 : 0); }

static juce::Colour tintOf (juce::Button& b)
{
    const auto t = b.getProperties()["tint"].toString();
    if (t == "mustard") return col::mustard;
    if (t == "cream")   return col::cream;
    if (t == "green")   return col::greenBtn;
    if (t == "red")     return col::redBtn;
    return col::rubber;
}

// botón de goma: cara gris con un brillo arriba; se hunde al pulsar. Con "led", luz verde a la izquierda
void Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f, 1.0f);
    const bool led = (bool) b.getProperties()["led"];
    const bool sunk = down || (b.getToggleState() && ! led);
    r.removeFromBottom (2.5f);
    g.setColour (col::bodyLo); g.fillRoundedRectangle (r.translated (0, 2.5f), 3.5f);
    auto face = sunk ? r.translated (0, 1.5f) : r;
    auto c = tintOf (b);
    if (sunk) c = c.darker (0.25f); else if (over) c = c.brighter (0.08f);
    if (! b.isEnabled()) c = c.withMultipliedSaturation (0.4f).darker (0.3f);
    g.setColour (c); g.fillRoundedRectangle (face, 3.5f);
    g.setColour (juce::Colours::white.withAlpha (sunk ? 0.06f : 0.14f));
    g.fillRoundedRectangle (face.withHeight (2.0f).reduced (3.0f, 0).translated (0, 1.0f), 1.0f);
    if (led)
    {
        const bool on = b.getToggleState();
        auto l = juce::Rectangle<float> (face.getX() + 11.0f, face.getCentreY() - 3.5f, 7.0f, 7.0f);
        if (on) { g.setColour (col::green.withAlpha (0.3f)); g.fillEllipse (l.expanded (3.5f)); }
        g.setColour (on ? col::green : col::greenOff);
        g.fillEllipse (l);
    }
}

void Look::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    auto r = b.getLocalBounds().withTrimmedBottom (3);
    if (down || (b.getToggleState() && ! (bool) b.getProperties()["led"])) r.translate (0, 1);
    const auto tint = tintOf (b);
    const auto ink = tint == col::rubber ? col::print : juce::Colour (0xff1b1404);
    const int arrow = b.getProperties()["arrow"];
    if (arrow != 0)
    {
        const auto c = r.getCentre().toFloat(); const float s = 7.0f;
        juce::Path t; t.addTriangle (c.x - s * 0.6f * (float) arrow, c.y - s, c.x - s * 0.6f * (float) arrow, c.y + s, c.x + s * 0.8f * (float) arrow, c.y);
        g.setColour (ink); g.fillPath (t);
        return;
    }
    if ((bool) b.getProperties()["led"]) r.removeFromLeft (18);
    printed (g, escoita::maiusculas (b.getButtonText()), r.reduced (6, 0).toFloat(), 13.0f, ink.withAlpha (b.isEnabled() ? 1.0f : 0.4f), juce::Justification::centred, 0.06f);
}

void Look::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&)
{
    auto r = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float s = juce::jmin (r.getWidth(), r.getHeight());
    r = r.withSizeKeepingCentre (s, s);
    g.setColour (col::rubber); g.fillEllipse (r);
    const float ang = a0 + pos * (a1 - a0), rad = s * 0.5f;
    const auto c = r.getCentre();
    g.setColour (col::redBtn);
    g.drawLine (c.x, c.y, c.x + rad * 0.85f * std::sin (ang), c.y - rad * 0.85f * std::cos (ang), 2.5f);
}

// fader como los del teclado: ranura negra, capuchón gris con una raya roja y marcas al lado
void Look::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle style, juce::Slider&)
{
    if (style == juce::Slider::LinearVertical)
    {
        const float cx = (float) x + (float) w * 0.5f - 8.0f;
        auto slot = juce::Rectangle<float> (cx - 4.0f, (float) y, 8.0f, (float) h);
        g.setColour (juce::Colour (0xff050506)); g.fillRoundedRectangle (slot, 3.0f);
        g.setColour (juce::Colour (0xff6a6b70));
        for (int t = 0; t <= 4; ++t) g.fillRect (cx + 16.0f, (float) y + (float) t * (float) (h - 2) / 4.0f, 9.0f, 1.2f);
        auto cap = juce::Rectangle<float> (cx - 13.0f, pos - 9.0f, 26.0f, 18.0f);
        g.setColour (col::bodyLo); g.fillRoundedRectangle (cap.translated (0, 2.0f), 2.5f);
        g.setColour (juce::Colour (0xff3a3b40)); g.fillRoundedRectangle (cap, 2.5f);
        g.setColour (juce::Colours::white.withAlpha (0.10f)); g.fillRect (cap.withHeight (2.0f).reduced (2.0f, 0).translated (0, 1.0f));
        g.setColour (col::redBtn); g.fillRect (cap.getX(), pos - 1.0f, cap.getWidth(), 2.0f);
        return;
    }
    const float cy = (float) y + (float) h * 0.5f;
    auto slot = juce::Rectangle<float> ((float) x, cy - 3.0f, (float) w, 6.0f);
    g.setColour (juce::Colour (0xff050506)); g.fillRoundedRectangle (slot, 3.0f);
    auto cap = juce::Rectangle<float> (pos - 7.0f, cy - 10.0f, 14.0f, 20.0f);
    g.setColour (col::bodyLo); g.fillRoundedRectangle (cap.translated (0, 2.0f), 2.5f);
    g.setColour (juce::Colour (0xff3a3b40)); g.fillRoundedRectangle (cap, 2.5f);
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.fillRect (cap.withHeight (2.0f).reduced (2.0f, 0).translated (0, 1.0f));
    g.setColour (col::redBtn); g.fillRect (pos - 1.0f, cap.getY(), 2.0f, cap.getHeight());
}

void Look::drawLabel (juce::Graphics& g, juce::Label& l)
{
    if (! l.findColour (juce::Label::backgroundColourId).isTransparent()) g.fillAll (l.findColour (juce::Label::backgroundColourId));
    if (l.isBeingEdited()) return;
    g.setColour (l.findColour (juce::Label::textColourId).withMultipliedAlpha (l.isEnabled() ? 1.0f : 0.5f));
    g.setFont (getLabelFont (l));
    g.drawFittedText (l.getText(), l.getLocalBounds().reduced (2, 0), l.getJustificationType(), 2, 0.9f);
}

juce::Label* Look::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (bold (13.5f));
    l->setColour (juce::Label::textColourId, col::print);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setJustificationType (juce::Justification::centred);
    return l;
}

void Look::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    auto r = juce::Rectangle<int> (w, h).toFloat();
    g.setColour (col::bodyHi); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (col::line); g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
    juce::AttributedString s; s.setJustification (juce::Justification::topLeft);
    s.append (text, body (14.0f), col::print);
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    tl.draw (g, r.reduced (10.0f, 7.0f));
}

juce::Rectangle<int> Look::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    juce::AttributedString s; s.append (text, body (14.0f));
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    const int w = (int) tl.getWidth() + 22, h = (int) tl.getHeight() + 16;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 12,
                                 pos.y > parent.getCentreY() ? pos.y - (h + 8) : pos.y + 18, w, h).constrainedWithin (parent);
}

// ================================================================ un sonido de la lista impresa
void VoiceButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f, 1.5f);
    if (selected) { g.setColour (col::teal.withAlpha (0.2f)); g.fillRoundedRectangle (r, 3.0f); }
    else if (over || down) { g.setColour (col::bodyHi.brighter (down ? 0.12f : 0.05f)); g.fillRoundedRectangle (r, 3.0f); }

    const bool amberCol = (index / 10) % 2 == 1;   // como en el panel: columnas alternas verde agua / ámbar
    auto t = r.reduced (6.0f, 2.0f);
    auto num = t.removeFromLeft (23.0f);
    const bool dual = BarulloProcessor::voiceParts (index) > 1;
    const auto numCol = selected ? juce::Colours::white : (amberCol ? col::amber : col::teal);
    printed (g, juce::String (index).paddedLeft ('0', 2), dual ? num.withTrimmedBottom (num.getHeight() * 0.32f) : num, 14.0f, numCol,
             juce::Justification::centredLeft, 0.02f);
    if (dual)   // dos canales por nota
    {
        g.setColour (numCol.withAlpha (0.75f)); g.setFont (body (10.5f));
        g.drawText (U ("×2"), num.withTrimmedTop (num.getHeight() * 0.56f), juce::Justification::topLeft);
    }
    if (edited)
    {
        g.setColour (col::mustard);
        g.fillEllipse (r.getRight() - 8.0f, r.getY() + 4.0f, 5.0f, 5.0f);
    }
    // nombre: una línea si cabe; si no, dos, partidas por la última palabra (letra estrecha, como el panel)
    const auto name = escoita::maiusculas (BarulloProcessor::voiceName (index));
    const auto f1 = juce::Font (juce::FontOptions (tf (0)).withHeight (12.5f).withKerningFactor (0.03f));
    const auto f2 = juce::Font (juce::FontOptions (tf (2)).withHeight (12.5f).withKerningFactor (0.02f));
    g.setColour (selected ? juce::Colours::white : col::print);
    const float w1 = juce::GlyphArrangement::getStringWidth (f1, name);
    if (w1 <= t.getWidth() || ! name.containsChar (' '))
    {
        g.setFont (w1 <= t.getWidth() ? f1 : f2);
        g.drawFittedText (name, t.toNearestInt(), juce::Justification::centredLeft, 1, 0.75f);
    }
    else
    {
        const int sp = name.lastIndexOfChar (' ');
        g.setFont (f2);
        auto top = t.withTrimmedBottom (t.getHeight() * 0.5f), bot = t.withTrimmedTop (t.getHeight() * 0.5f);
        g.drawFittedText (name.substring (0, sp), top.toNearestInt(), juce::Justification::bottomLeft, 1, 0.75f);
        g.drawFittedText (name.substring (sp + 1), bot.toNearestInt(), juce::Justification::topLeft, 1, 0.75f);
    }
}

// ================================================================ fila del editor
RegRow::RegRow (juce::String label, int max, std::function<juce::String (int)> f) : fmt (std::move (f))
{
    name.setText (label, juce::dontSendNotification);
    name.setFont (body (13.5f));
    name.setColour (juce::Label::textColourId, col::print);
    value.setFont (bold (13.5f));
    value.setColour (juce::Label::textColourId, col::print);
    value.setJustificationType (juce::Justification::centredRight);
    slider.setRange (0, max, 1);
    slider.onValueChange = [this] { const int v = (int) slider.getValue(); value.setText (fmt (v), juce::dontSendNotification); if (onChange) onChange (v); };
    slider.setWantsKeyboardFocus (false);
    for (auto* c : std::initializer_list<juce::Component*> { &name, &slider, &value }) addAndMakeVisible (c);
}

void RegRow::resized()
{
    auto r = getLocalBounds();
    name.setBounds (r.removeFromLeft (r.getWidth() * 36 / 100));
    value.setBounds (r.removeFromRight (62));
    slider.setBounds (r.reduced (8, 0));
}

// ================================================================ editor de sonidos
static juce::TextButton* ledButton (juce::TextButton* b) { b->getProperties().set ("led", true); b->setWantsKeyboardFocus (false); return b; }

PatchEditor::PatchEditor (BarulloProcessor& p) : proc (p)
{
    static const float MULT[16] = { 0.5f, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 12, 12, 15, 15 };
    fields = { { 2, 0, 15 }, { 4, 0, 255 }, { 6, 4, 15 }, { 6, 0, 15 }, { 8, 4, 15 }, { 8, 0, 15 } };
    checks = { { 2, 4, 1 }, { 2, 5, 1 } };
    const juce::String labels[] = { "Multiplicador", "Nivel", "Ataque", U ("Caída"), U ("Sostemento"), U ("Relaxación") };
    const juce::String tips[] = {
        U ("Cantas veces a frecuencia da nota (×0,5 a ×15)."),
        U ("Atenuación do operador, en pasos de 0,1875 dB. No modulador decide canto brillo hai."),
        U ("Velocidade de subida ao premer (0 = non sobe, 15 = instantáneo)."),
        U ("Velocidade de baixada ata o sostemento."),
        U ("Nivel no que queda a nota despois da caída."),
        U ("Velocidade á que segue baixando mentres mantés a tecla. Ao soltar, o teclado escribe a súa propia caída.") };
    const std::function<juce::String (int)> fmts[] = {
        [] (int v) { return U ("×") + juce::String (MULT[v & 15], v == 0 ? 1 : 0); },
        [] (int v) { return v == 0 ? juce::String ("0 dB") : U ("−") + juce::String (v * 0.1875, 1) + " dB"; },
        [] (int v) { return juce::String (v); },
        [] (int v) { return juce::String (v); },
        [] (int v) { return v == 0 ? juce::String ("0 dB") : v == 15 ? U ("−93 dB") : U ("−") + juce::String (v * 3) + " dB"; },
        [] (int v) { return juce::String (v); } };
    const juce::String checkLabels[] = { "KSR", "Media onda" };
    const juce::String checkTips[] = { U ("A envolvente vai máis rápida nas notas agudas."), U ("Deixa só a metade positiva do seno: son máis oco e nasal.") };

    for (int op = 0; op < 2; ++op)
    {
        for (size_t i = 0; i < fields.size(); ++i)
        {
            auto* row = opRows[op].add (new RegRow (labels[i], fields[i].m, fmts[i]));
            row->name.setTooltip (tips[i]); row->slider.setTooltip (tips[i]);
            const auto f = fields[i];
            row->onChange = [this, f, op] (int v) { setBits (f.k + op, f.sh, f.m, v); repaint(); };
            addAndMakeVisible (row);
        }
        for (size_t i = 0; i < checks.size(); ++i)
        {
            auto* t = ledButton (opChecks[op].add (new juce::TextButton (checkLabels[i])));
            t->setTooltip (checkTips[i]);
            const auto f = checks[i];
            t->onClick = [this, f, op, t] { t->setToggleState (! t->getToggleState(), juce::dontSendNotification); setBits (f.k + op, f.sh, 1, t->getToggleState() ? 1 : 0); };
            addAndMakeVisible (t);
        }
    }
    fbRow = std::make_unique<RegRow> (U ("Realimentación"), 7, [] (int v) { return juce::String (v); });
    fbRow->slider.setTooltip (U ("O modulador modúlase a si mesmo: de 0 (seno puro) a 7 (case ruído)."));
    fbRow->onChange = [this] (int v) { setG7 (0, 7, v); repaint(); };
    addAndMakeVisible (*fbRow);
    const juce::String chLabels[] = { "Mesma frecuencia", "Esquerda", "Dereita" };
    const juce::String chTips[] = { U ("O modulador usa a frecuencia da portadora."), U ("A canle sae polo lado esquerdo."), U ("A canle sae polo lado dereito.") };
    const int chBits[] = { 7, 6, 5 };
    for (int i = 0; i < 3; ++i)
    {
        auto* t = ledButton (chChecks.add (new juce::TextButton (chLabels[i])));
        t->setTooltip (chTips[i]);
        const int sh = chBits[i];
        t->onClick = [this, sh, t] { t->setToggleState (! t->getToggleState(), juce::dontSendNotification); setG7 (sh, 1, t->getToggleState() ? 1 : 0); repaint(); };
        addAndMakeVisible (t);
    }

    partA.setButtonText ("Parte 1"); partB.setButtonText ("Parte 2");
    ledButton (&partA); ledButton (&partB);
    partA.onClick = [this] { part = 0; refresh(); };
    partB.onClick = [this] { part = 1; refresh(); };
    resetBtn.setButtonText ("Restaurar o orixinal");
    resetBtn.getProperties().set ("tint", "cream");
    resetBtn.onClick = [this] { proc.resetPatch (proc.currentVoice()); refresh(); };
    chipBtn.setButtonText ("Ver no chip");
    chipBtn.setTooltip (U ("Como quedan estes valores dentro da memoria do chip, en directo."));
    chipBtn.onClick = [this] { if (onShowChip) onShowChip(); };
    for (auto* b : { &partA, &partB, &resetBtn, &chipBtn }) { b->setWantsKeyboardFocus (false); addAndMakeVisible (b); }

    hex.setFont (body (11.5f));
    hex.setColour (juce::Label::textColourId, col::printDim);
    hex.setTooltip (U ("Os rexistros tal como van ao chip."));
    note.setFont (body (13.0f));
    note.setColour (juce::Label::textColourId, col::printDim);
    note.setJustificationType (juce::Justification::topLeft);
    addChildComponent (hex); addAndMakeVisible (note);
}

void PatchEditor::setBits (int k, int sh, int m, int val)
{
    const int v = proc.currentVoice();
    const int old = proc.getPatch (v)[(size_t) part].r[k];
    proc.setPatchReg (v, part, k, (old & ~(m << sh)) | ((val & m) << sh));
    hex.setText (BarulloProcessor::patchToString (proc.getPatch (v), BarulloProcessor::voiceParts (v)), juce::dontSendNotification);
    resetBtn.setEnabled (proc.isEdited (v));
}

void PatchEditor::setG7 (int sh, int m, int val)
{
    const int v = proc.currentVoice();
    const int old = proc.getPatch (v)[(size_t) part].g7;
    proc.setPatchReg (v, part, 16, (old & ~(m << sh)) | ((val & m) << sh));
    hex.setText (BarulloProcessor::patchToString (proc.getPatch (v), BarulloProcessor::voiceParts (v)), juce::dontSendNotification);
    resetBtn.setEnabled (proc.isEdited (v));
}

void PatchEditor::refresh()
{
    const int v = proc.currentVoice();
    const int nParts = BarulloProcessor::voiceParts (v);
    if (v != shownVoice) { shownVoice = v; part = 0; }
    if (part >= nParts) part = 0;
    partA.setVisible (nParts > 1); partB.setVisible (nParts > 1);
    if (nParts > 1)
    {
        auto sideName = [&] (int pi)
        {
            const int g7 = proc.getPatch (v)[(size_t) pi].g7;
            const bool L = (g7 >> 6) & 1, R = (g7 >> 5) & 1;
            if (L && ! R) return juce::String ("Esquerda (L)");
            if (R && ! L) return juce::String ("Dereita (R)");
            return (L && R ? "Capa " : "Parte ") + juce::String (pi + 1) + (L && R ? " (L+R)" : " (muda)");
        };
        partA.setButtonText (sideName (0)); partB.setButtonText (sideName (1));
        partA.setTooltip (U ("Primeira canle de cada nota: ") + sideName (0).toLowerCase() + ".");
        partB.setTooltip (U ("Segunda canle de cada nota: ") + sideName (1).toLowerCase() + ".");
    }
    partA.setToggleState (part == 0, juce::dontSendNotification);
    partB.setToggleState (part == 1, juce::dontSendNotification);

    const auto& p = proc.getPatch (v)[(size_t) part];
    for (int op = 0; op < 2; ++op)
    {
        for (size_t i = 0; i < fields.size(); ++i)
            opRows[op][(int) i]->setValue ((p.r[fields[i].k + op] >> fields[i].sh) & fields[i].m);
        for (size_t i = 0; i < checks.size(); ++i)
            opChecks[op][(int) i]->setToggleState ((p.r[checks[i].k + op] >> checks[i].sh) & 1, juce::dontSendNotification);
    }
    fbRow->setValue (p.g7 & 7);
    chChecks[0]->setToggleState ((p.g7 >> 7) & 1, juce::dontSendNotification);
    chChecks[1]->setToggleState ((p.g7 >> 6) & 1, juce::dontSendNotification);
    chChecks[2]->setToggleState ((p.g7 >> 5) & 1, juce::dontSendNotification);
    hex.setText (BarulloProcessor::patchToString (proc.getPatch (v), nParts), juce::dontSendNotification);

    const auto& V = ym::VOICES[v];
    bool initAll = false;
    for (int k = 2; k <= 14; ++k) if (V.init[0][k] >= 0) initAll = true;
    juce::String txt;
    if (initAll)
        txt = U ("Co Movemento acendido, este son arrinca cos rexistros da súa película e non con estes. Apágao para oír os teus cambios.");
    else if (V.evN > 0)
    {
        bool w[15] {};
        for (uint32_t i = 0; i < V.evN; ++i) w[ym::FILM_EVENTS[V.ev0 + i].k] = true;
        juce::StringArray what;
        if (w[2] || w[3]) what.add ("multiplicador");
        if (w[4] || w[5]) what.add ("nivel");
        if (w[6] || w[7] || w[8] || w[9]) what.add ("envolvente");
        if (w[10] || w[11] || w[12] || w[13]) what.add ("frecuencia");
        if (w[14]) what.add ("canle");
        juce::String list = what.joinIntoString (", ");
        if (what.size() > 1) list = what.joinIntoString (", ", 0, what.size() - 1) + " e " + what[what.size() - 1];
        txt = U ("Co Movemento acendido, a película deste son vai cambiando ") + list + " mentres soa.";
    }
    else txt = U ("Este son non ten película: o que ves é exactamente o que soa.");
    note.setText (txt, juce::dontSendNotification);
    resetBtn.setEnabled (proc.isEdited (v));
    repaint();
}

static void ledWindow (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff050506)); g.fillRoundedRectangle (r.expanded (2.0f), 5.0f);
    g.setColour (col::ledBg); g.fillRoundedRectangle (r, 4.0f);
}

// envolvente aproximada (los mismos tiempos que la web), dibujada en LED
void PatchEditor::drawEnvelope (juce::Graphics& g, juce::Rectangle<int> area, int op) const
{
    auto r = area.toFloat();
    ledWindow (g, r);
    auto in = r.reduced (10.0f, 9.0f);
    g.setColour (col::ledOff.brighter (0.25f));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (in.getY() + in.getHeight() * (float) i / 4.0f), in.getX(), in.getRight());

    const int v = proc.currentVoice();
    const auto& p = proc.getPatch (v)[(size_t) part];
    const int ar = p.r[6 + op] >> 4, dr = p.r[6 + op] & 15, sl = p.r[8 + op] >> 4, rr = p.r[8 + op] & 15;
    const int relReg = ym::VOICES[v].rel[part][op];
    const int rrRel = relReg >= 0 ? (relReg & 15) : rr;
    const double INF = 1e9;
    auto atk = [&] (int R) { return R == 0 ? INF : R >= 15 ? 0.0 : 2.826 / std::pow (2.0, R - 1); };
    auto dec = [&] (int R, double dB) { return R == 0 ? INF : 39.28 / std::pow (2.0, R - 1) * dB / 96.0; };
    auto wOf = [&] (double t) { return t >= INF ? 1.6 : 0.15 + std::log10 (1 + t * 1000) / 4.5; };
    const double slDb = sl == 15 ? 93 : sl * 3;
    const double ta = atk (ar), td = dec (dr, juce::jmin (48.0, slDb));
    const double holdRate = rr == 0 ? 0 : 96.0 / (39.28 / std::pow (2.0, rr - 1));
    const double afterHold = juce::jmin (48.0, juce::jmin (48.0, slDb) + holdRate * 1.0);
    const double tr = dec (rrRel, 48.0 - afterHold);
    const double wa = wOf (ta), wd = slDb <= 0 ? 0.15 : wOf (td), wh = 0.9, wr = afterHold >= 48 ? 0.15 : wOf (tr);
    const double sum = wa + wd + wh + wr;
    auto X = [&] (double w) { return in.getX() + (float) (w / sum) * in.getWidth(); };
    auto Y = [&] (double dB) { return in.getY() + (float) (juce::jmin (48.0, dB) / 48.0) * in.getHeight(); };

    juce::Path path;
    path.startNewSubPath (X (0), Y (48));
    if (ar == 0) path.lineTo (X (sum), Y (48));
    else
    {
        for (int i = 1; i <= 12; ++i) { const double f = i / 12.0; path.lineTo (X (wa * f), Y (48.0 * std::pow (1.0 - f, 2.2))); }
        path.lineTo (X (wa + wd), Y (juce::jmin (48.0, slDb)));
        path.lineTo (X (wa + wd + wh), Y (afterHold));
        path.lineTo (X (sum), Y (48));
    }
    g.setColour (col::led.withAlpha (0.25f)); g.strokePath (path, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (col::led); g.strokePath (path, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float xr = X (wa + wd + wh);
    g.setColour (col::led.withAlpha (0.45f));
    for (float y = in.getY(); y < in.getBottom(); y += 5.0f) g.drawLine (xr, y, xr, y + 2.5f, 1.0f);
    g.setFont (body (11.5f));
    g.drawText ("soltas", juce::Rectangle<float> (xr + 4, in.getBottom() - 13, 60, 14), juce::Justification::centredLeft);
}

void PatchEditor::drawFlow (juce::Graphics& g, juce::Rectangle<int> area) const
{
    auto r = area.toFloat();
    ledWindow (g, r);
    const int v = proc.currentVoice();
    const auto& p = proc.getPatch (v)[(size_t) part];
    const int fb = p.g7 & 7; const bool link = (p.g7 >> 7) & 1, L = (p.g7 >> 6) & 1, R = (p.g7 >> 5) & 1;

    auto in = r.reduced (12.0f, 12.0f);
    const float bw = 64.0f, bh = 30.0f, cy = in.getCentreY() + 8.0f;
    auto mod = juce::Rectangle<float> (in.getX() + 6.0f, cy - bh / 2, bw, bh);
    auto car = juce::Rectangle<float> (mod.getRight() + 34.0f, cy - bh / 2, bw, bh);
    const float ox = car.getRight() + 26.0f;
    auto outL = juce::Rectangle<float> (ox, cy - 26.0f, 44.0f, 20.0f), outR = juce::Rectangle<float> (ox, cy + 6.0f, 44.0f, 20.0f);
    const auto dim = col::ledOff.brighter (0.9f);

    auto box = [&] (juce::Rectangle<float> b, const juce::String& t, bool on)
    {
        if (on) { g.setColour (col::led.withAlpha (0.12f)); g.fillRoundedRectangle (b, 4.0f); }
        g.setColour (on ? col::led : dim); g.drawRoundedRectangle (b, 4.0f, 1.4f);
        g.setFont (body (12.5f)); g.drawText (t, b, juce::Justification::centred);
    };
    box (mod, "Modulador", true);
    box (car, "Portadora", true);
    box (outL, "Esq.", L);
    box (outR, "Der.", R);
    g.setColour (col::led);
    g.drawArrow ({ mod.getRight(), cy, car.getX() - 1, cy }, 1.4f, 7.0f, 7.0f);
    g.setColour (L ? col::led : dim);
    g.drawArrow ({ car.getRight(), cy, outL.getX() - 1, outL.getCentreY() }, 1.2f, 6.0f, 6.0f);
    g.setColour (R ? col::led : dim);
    g.drawArrow ({ car.getRight(), cy, outR.getX() - 1, outR.getCentreY() }, 1.2f, 6.0f, 6.0f);

    juce::Path loop;
    const float lx0 = mod.getRight() - 10.0f, lx1 = mod.getX() + 10.0f, top = mod.getY() - 22.0f;
    loop.startNewSubPath (lx0, mod.getY());
    loop.cubicTo (lx0 + 4.0f, top, lx1 - 4.0f, top, lx1, mod.getY() - 1.0f);
    g.setColour (fb > 0 ? col::led : dim);
    if (fb > 0) g.strokePath (loop, juce::PathStrokeType (1.2f + (float) fb * 0.25f));
    else { juce::Path d; const float dl[] = { 3.0f, 3.0f }; juce::PathStrokeType (1.0f).createDashedStroke (d, loop, dl, 2); g.fillPath (d); }
    g.setFont (body (12.0f));
    g.drawText ("realim. " + juce::String (fb), juce::Rectangle<float> (mod.getX() - 4.0f, top - 16.0f, bw + 8.0f, 14.0f), juce::Justification::centred);
    if (link)
    {
        g.setColour (col::led.withAlpha (0.8f));
        g.drawText ("mesma frecuencia", juce::Rectangle<float> (mod.getX(), mod.getBottom() + 6.0f, car.getRight() - mod.getX(), 14.0f), juce::Justification::centred);
    }
}

void PatchEditor::paint (juce::Graphics& g)
{
    const juce::String titles[] = { "MODULADOR", "PORTADORA", "CANLE" };
    const juce::String subs[] = { "op0, o que modula", "op1, o que soa", "" };
    for (int i = 0; i < 3; ++i)
    {
        auto c = panels[i].toFloat();
        g.setColour (col::bodyHi); g.fillRoundedRectangle (c, 5.0f);
        g.setColour (col::line); g.drawRoundedRectangle (c.reduced (0.5f), 5.0f, 1.0f);
        auto head = c.reduced (14.0f, 8.0f).removeFromTop (20.0f);
        printed (g, titles[i], head, 14.0f, col::teal, juce::Justification::centredLeft, 0.12f);
        g.setColour (col::printDim); g.setFont (body (12.5f));
        g.drawText (subs[i], head, juce::Justification::centredRight);
    }
    drawEnvelope (g, envArea[0], 0);
    drawEnvelope (g, envArea[1], 1);
    drawFlow (g, flowArea);
}

void PatchEditor::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (38).withTrimmedBottom (6);
    partA.setBounds (top.removeFromLeft (140)); top.removeFromLeft (8);
    partB.setBounds (top.removeFromLeft (140));
    resetBtn.setBounds (top.removeFromRight (170));
    top.removeFromRight (8);
    chipBtn.setBounds (top.removeFromRight (160));
    const int gap = 14, colW = (r.getWidth() - 2 * gap) / 3;
    for (int i = 0; i < 3; ++i) panels[i] = juce::Rectangle<int> (r.getX() + i * (colW + gap), r.getY(), colW, r.getHeight());
    for (int op = 0; op < 2; ++op)
    {
        auto c = panels[op].reduced (14, 10);
        c.removeFromTop (26);
        envArea[op] = c.removeFromTop (54);
        c.removeFromTop (8);
        for (auto* row : opRows[op]) row->setBounds (c.removeFromTop (24));
        c.removeFromTop (6);
        auto bt = c.removeFromTop (30);
        opChecks[op][0]->setBounds (bt.removeFromLeft (bt.getWidth() / 2 - 4));
        bt.removeFromLeft (8);
        opChecks[op][1]->setBounds (bt);
    }
    auto c = panels[2].reduced (14, 10);
    c.removeFromTop (26);
    flowArea = c.removeFromTop (104);
    c.removeFromTop (8);
    fbRow->setBounds (c.removeFromTop (24));
    c.removeFromTop (6);
    chChecks[0]->setBounds (c.removeFromTop (30));
    c.removeFromTop (6);
    auto bt = c.removeFromTop (30);
    chChecks[1]->setBounds (bt.removeFromLeft (bt.getWidth() / 2 - 3)); bt.removeFromLeft (6);
    chChecks[2]->setBounds (bt);
    c.removeFromTop (8);
    note.setBounds (c);
}


// ================================================================ el chip por dentro
void ChipView::tick()
{
    auto& b = proc.bus;
    for (int a = 0; a < 128; ++a)
    {
        regs[(size_t) a] = b.regs[(size_t) a].load (std::memory_order_relaxed);
        const uint32_t w = b.writes[(size_t) a].load (std::memory_order_relaxed);
        if (w != seen[(size_t) a]) { seen[(size_t) a] = w; flash[(size_t) a] = 1.0f; }
        else flash[(size_t) a] *= 0.72f;
    }
    const uint32_t t = b.total.load (std::memory_order_relaxed);
    if (t != lastTotal) { lastTotal = t; busFlash = 1.0f; data = b.lastData.load (std::memory_order_relaxed); }
    else busFlash *= 0.85f;
    mask = proc.activeChannels.load();
    repaint();
}

void ChipView::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    // texto de ayuda abajo
    auto foot = area.removeFromBottom (36.0f);
    g.setColour (col::printDim); g.setFont (body (13.0f));
    g.drawFittedText (U ("Cada cela é un rexistro da memoria do chip, en hexadecimal: en cada canle, a da esquerda é o modulador e a da dereita a portadora. "
                         "O teclado escríbeos byte a byte polas patas D0–D7; acéndense ao escribirse. Toca algo e mírao."),
                      foot.toNearestInt(), juce::Justification::centred, 2, 1.0f);

    // patas: 9 abajo (1–9, de izquierda a derecha) y 9 arriba (10–18, de derecha a izquierda), como un DIP-18 visto desde arriba
    const float legLen = 20.0f, labelH = 30.0f;
    auto bodyR = area.reduced (40.0f, legLen + labelH + 2.0f);
    bodyR = bodyR.withSizeKeepingCentre (juce::jmin (bodyR.getWidth(), 820.0f), bodyR.getHeight());
    const char* names[19] = { "", "A0", "/WE", "reloxo", "reloxo", "?", "GND", "?", "?", "+5 V", "?", "D7", "D6", "D5", "D4", "D3", "D2", "D1", "D0" };
    const float pitch = (bodyR.getWidth() - 60.0f) / 8.0f;
    for (int pin = 1; pin <= 18; ++pin)
    {
        const bool bottom = pin <= 9;
        const int i = bottom ? pin - 1 : 18 - pin;
        const float x = bodyR.getX() + 30.0f + (float) i * pitch;
        auto leg = juce::Rectangle<float> (x - 6.0f, bottom ? bodyR.getBottom() : bodyR.getY() - legLen, 12.0f, legLen);
        // brillo de la pata: los datos se encienden con los bits del último byte, A0 y /WE con cada escritura
        float lit = 0.0f;
        if (pin >= 11) lit = ((data >> (18 - pin)) & 1) ? busFlash : 0.0f;
        if (pin == 1 || pin == 2) lit = busFlash;
        juce::ColourGradient metal (juce::Colour (0xffc9ccd1), leg.getX(), 0, juce::Colour (0xff7d8187), leg.getRight(), 0, false);
        g.setGradientFill (metal); g.fillRect (leg);
        if (lit > 0.05f) { g.setColour (col::amber.withAlpha (0.85f * lit)); g.fillRect (leg); }
        // nombre y número
        auto lab = juce::Rectangle<float> (x - pitch / 2, bottom ? leg.getBottom() + 3.0f : leg.getY() - labelH - 3.0f, pitch, labelH);
        const bool unknown = juce::String (names[pin]) == "?";
        auto nameR = bottom ? lab.removeFromTop (16.0f) : lab.removeFromBottom (16.0f);
        printed (g, U (names[pin]), nameR, 13.0f, unknown ? col::printDim : (pin >= 11 ? col::amber : col::teal), juce::Justification::centred, 0.04f);
        g.setColour (col::printDim); g.setFont (body (11.5f));
        g.drawText (juce::String (pin), lab, juce::Justification::centred);
    }

    // cuerpo de resina con la muesca y el punto de la pata 1
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (bodyR.translated (0, 4.0f), 6.0f);
    juce::ColourGradient resin (juce::Colour (0xff26272b), bodyR.getX(), bodyR.getY(), juce::Colour (0xff111113), bodyR.getX(), bodyR.getBottom(), false);
    g.setGradientFill (resin); g.fillRoundedRectangle (bodyR, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawRoundedRectangle (bodyR.reduced (0.5f), 6.0f, 1.0f);
    g.setColour (col::body.darker (0.6f));
    g.fillEllipse (bodyR.getX() - 14.0f, bodyR.getCentreY() - 14.0f, 28.0f, 28.0f);
    g.setColour (juce::Colour (0xff0c0c0d)); g.fillEllipse (bodyR.getX() + 7.0f, bodyR.getBottom() - 15.0f, 8.0f, 8.0f);
    printed (g, "YM7129", { bodyR.getX() + 32.0f, bodyR.getY() + 8.0f, 160.0f, 18.0f }, 15.0f, juce::Colour (0xff8a8c92), juce::Justification::centredLeft, 0.18f);

    // la memoria de registros
    auto grid = bodyR.reduced (24.0f, 10.0f);
    grid.removeFromTop (22.0f);
    auto head = grid.removeFromTop (18.0f);
    const float labelW = 108.0f;
    head.removeFromLeft (labelW);
    const float cellW = grid.getWidth() > labelW ? (grid.getWidth() - labelW) / 16.0f : 10.0f;
    for (int c = 0; c < 8; ++c)
    {
        const bool on = (mask >> c) & 1;
        auto h = juce::Rectangle<float> (head.getX() + (float) c * 2.0f * cellW, head.getY(), cellW * 2.0f, head.getHeight());
        if (on) { g.setColour (col::green.withAlpha (0.25f)); g.fillEllipse (h.getCentreX() - 22.0f, h.getCentreY() - 3.0f, 6.0f, 6.0f); }
        g.setColour (on ? col::green : col::greenOff.brighter (0.6f));
        g.fillEllipse (h.getCentreX() - 21.0f, h.getCentreY() - 2.5f, 5.0f, 5.0f);
        printed (g, "canle " + juce::String (c + 1), h.withTrimmedLeft (8.0f), 11.5f, on ? col::print : col::printDim, juce::Justification::centred, 0.02f);
    }
    const juce::String rowNames[7] = { "Multiplicador", "Nivel", U ("Ataque e caída"), U ("Sostemento e relax."), "Frecuencia alta", "Frecuencia baixa", U ("Saída e realim.") };
    const float rowH = grid.getHeight() / 7.0f;
    for (int row = 0; row < 7; ++row)
    {
        const int gnum = row + 1;
        auto line = juce::Rectangle<float> (grid.getX(), grid.getY() + (float) row * rowH, grid.getWidth(), rowH);
        g.setColour (col::printDim); g.setFont (body (12.5f));
        g.drawText (rowNames[row], line.withWidth (labelW - 6.0f), juce::Justification::centredLeft);
        for (int c = 0; c < 8; ++c)
            for (int op = 0; op < (gnum == 7 ? 1 : 2); ++op)
            {
                const int a = gnum == 7 ? 0x70 + c : ym::adr (gnum, c, op);
                const float w = gnum == 7 ? cellW * 2.0f : cellW;
                auto cell = juce::Rectangle<float> (line.getX() + labelW + (float) c * 2.0f * cellW + (float) op * cellW, line.getY(), w, rowH).reduced (1.5f, 2.0f);
                const float f = flash[(size_t) a];
                const bool on = (mask >> c) & 1;
                g.setColour (juce::Colours::white.withAlpha (on ? 0.06f : 0.03f)); g.fillRoundedRectangle (cell, 2.0f);
                if (f > 0.03f) { g.setColour (col::amber.withAlpha (0.45f * f)); g.fillRoundedRectangle (cell, 2.0f); }
                const auto txt = juce::String::toHexString ((int) regs[(size_t) a]).toUpperCase().paddedLeft ('0', 2);
                printed (g, txt, cell, 12.5f, f > 0.3f ? juce::Colours::white : (on ? col::print : juce::Colour (0xff6f7176)), juce::Justification::centred, 0.04f);
            }
        if (row < 6) { g.setColour (juce::Colours::white.withAlpha (0.04f)); g.drawHorizontalLine ((int) line.getBottom(), line.getX(), line.getRight()); }
    }
    // separadores entre canales
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    for (int c = 1; c < 8; ++c) { const float x = grid.getX() + labelW + (float) c * 2.0f * cellW; g.drawVerticalLine ((int) x, grid.getY(), grid.getBottom()); }
}

// ================================================================ vista principal
MainView::MainView (BarulloProcessor& p)
    : proc (p), keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard), patchEditor (p), chipView (p)
{
    setLookAndFeel (&look);

    prevBtn.getProperties().set ("arrow", -1); nextBtn.getProperties().set ("arrow", 1);
    for (auto* b : { &prevBtn, &nextBtn }) { b->setWantsKeyboardFocus (false); addAndMakeVisible (b); }
    prevBtn.onClick = [this] { step (-1); };
    nextBtn.onClick = [this] { step (+1); };
    prevBtn.setTooltip ("Son anterior");
    nextBtn.setTooltip ("Son seguinte");

    tabSounds.setButtonText ("Sons"); tabEdit.setButtonText ("Editar"); tabChip.setButtonText ("Chip");
    ledButton (&tabSounds); ledButton (&tabEdit); ledButton (&tabChip);
    tabSounds.onClick = [this] { showPage (0); };
    tabEdit.onClick = [this] { showPage (1); };
    tabChip.onClick = [this] { showPage (2); };
    tabChip.setTooltip (U ("O chip por dentro: que hai en cada rexistro e que pasa polas súas patas mentres tocas."));
    for (auto* b : { &tabSounds, &tabEdit, &tabChip }) addAndMakeVisible (b);

    for (int i = 0; i < ym::NUM_VOICES; ++i)
    {
        auto* b = voiceBtns.add (new VoiceButton (i));
        b->setTooltip (BarulloProcessor::voiceDesc (i));
        b->onClick = [this, i] { selectVoice (i); };
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (b);
    }
    addChildComponent (patchEditor);
    addChildComponent (chipView);
    patchEditor.onShowChip = [this] { showPage (2); };

    filmsBtn.setButtonText ("Movemento");
    speakerBtn.setButtonText (U ("Saturación"));
    eqSpeaker.setButtonText ("Altofalante"); eqFlat.setButtonText ("Plano");
    unlimitedBtn.setButtonText (U ("Sen límite"));
    for (auto* b : { &filmsBtn, &speakerBtn, &unlimitedBtn, &eqSpeaker, &eqFlat }) { ledButton (b); addAndMakeVisible (b); }
    filmsBtn.setClickingTogglesState (true); speakerBtn.setClickingTogglesState (true); unlimitedBtn.setClickingTogglesState (true);
    unlimitedBtn.setTooltip (U ("Apagado: 8 canles, coma o teclado (4 notas cos sons dobres). Acendido: chips virtuais iguais ao orixinal, ata 64 canles, e xa non se corta ningunha nota."));
    filmsBtn.setTooltip (U ("O que o teclado real reescribe no chip mentres soa a nota: vibratos, trémolos, varridos. Apagado: só o son base."));
    speakerBtn.setTooltip (U ("Satura coma un altofalante pequeno a moito volume."));
    eqSpeaker.setTooltip (U ("Ecualizado coma o teclado polo seu altofalante."));
    eqFlat.setTooltip (U ("O chip tal cal, sen ecualizar."));

    bend.setTooltip (U ("Semitóns da roda de bend. O teclado orixinal non ten."));
    vel.setTooltip (U ("Canto baixa o volume ao tocar suave. A 0 % soa coma o teclado, que non ten sensibilidade."));
    volume.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    volume.setTextValueSuffix (" dB");
    volume.setDoubleClickReturnValue (true, 0.0);
    bend.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 18);
    bend.setTextValueSuffix (" st");
    vel.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 18);
    vel.setTextValueSuffix (" %");
    for (auto* s : { &volume, &bend, &vel })
    {
        s->setColour (juce::Slider::textBoxTextColourId, col::print);
        s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s->setWantsKeyboardFocus (false); addAndMakeVisible (s);
    }

    filmsAtt   = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, "films", filmsBtn);
    speakerAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, "speaker", speakerBtn);
    unlimitedAtt = std::make_unique<APVTS::ButtonAttachment> (proc.apvts, "unlimited", unlimitedBtn);
    volAtt     = std::make_unique<APVTS::SliderAttachment> (proc.apvts, "volume", volume);
    bendAtt    = std::make_unique<APVTS::SliderAttachment> (proc.apvts, "bend", bend);
    velAtt     = std::make_unique<APVTS::SliderAttachment> (proc.apvts, "velocity", vel);
    eqAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter ("eq"), [this] (float v)
    {
        eqSpeaker.setToggleState (v < 0.5f, juce::dontSendNotification);
        eqFlat.setToggleState (v >= 0.5f, juce::dontSendNotification);
    });
    eqSpeaker.onClick = [this] { eqAtt->setValueAsCompleteGesture (0.0f); };
    eqFlat.onClick = [this] { eqAtt->setValueAsCompleteGesture (1.0f); };
    eqAtt->sendInitialUpdate();

    keyboard.setAvailableRange (36, 96);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyPressBaseOctave (5);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, col::ivory);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, col::blackKey);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, col::led.withAlpha (0.5f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, col::teal.withAlpha (0.18f));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xffb9b8b2));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId, col::printDim);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, col::rubber);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, col::print);
    addAndMakeVisible (keyboard);

    escoita.setButtonText ("ESCOITA.");
    escoita.setURL (juce::URL ("https://www.escoita.link"));
    escoita.setFont (juce::Font (juce::FontOptions (tf (1)).withHeight (17.0f).withKerningFactor (0.16f)), false, juce::Justification::centredRight);
    escoita.setColour (juce::HyperlinkButton::textColourId, col::print);
    escoita.setTooltip (U ("Barullo FM é unha ferramenta de Escoita · escoita.link (clic dereito: a versión e as descargas)"));
    escoita.setWantsKeyboardFocus (false);
    addAndMakeVisible (escoita);
    escoita.onMenu = [this] { showEscoitaMenu(); };
    updateBtn.onClick = [] { juce::URL (escoita::Actualizacions::downloadsUrl()).launchInDefaultBrowser(); };
    addChildComponent (updateBtn);
    refreshUpdate();

    setSize (W, H);
    showPage (0);
    startTimerHz (30);
    timerCallback();
}

// o aviso da versión nova: aparece cando o fío das versións sabe dunha
void MainView::refreshUpdate()
{
    const auto v = updates->newVersion();
    if (v == updateBtn.version && updateBtn.isVisible() == v.isNotEmpty()) return;
    updateBtn.version = v;
    updateBtn.setTitle (U ("Hai unha versión nova: ") + v);
    updateBtn.setTooltip (U ("Tes a ") + ESCOITA_VERSION + U (". Abre a páxina de descargas para baixar a ") + v + ".");
    updateBtn.setVisible (v.isNotEmpty());
}

void MainView::showEscoitaMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader (U ("Barullo FM ") + ESCOITA_VERSION);
    m.addItem (U ("Abrir escoita.link"), [] { juce::URL ("https://www.escoita.link").launchInDefaultBrowser(); });
    m.addItem (U ("Descargas e versións"), [] { juce::URL (escoita::Actualizacions::downloadsUrl()).launchInDefaultBrowser(); });
    m.addSeparator();
    m.addItem (U ("Avisar das versións novas"), true, updates->enabled(), [sp = juce::Component::SafePointer<MainView> (this)]
    {
        if (sp == nullptr) return;
        sp->updates->setEnabled (! sp->updates->enabled());
        sp->refreshUpdate();
    });
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&escoita));
}

void UpdateButton::paintButton (juce::Graphics& g, bool over, bool)
{
    auto r = getLocalBounds().toFloat();
    const auto t = U ("VERSIÓN NOVA ") + version + U (" · DESCARGALA");
    // un LED aceso á dereita e o texto impreso á súa esquerda
    const float cx = r.getRight() - 7.0f, cy = r.getCentreY();
    g.setColour (col::led.withAlpha (0.25f)); g.fillEllipse (cx - 7.0f, cy - 7.0f, 14.0f, 14.0f);
    g.setColour (col::led); g.fillEllipse (cx - 3.5f, cy - 3.5f, 7.0f, 7.0f);
    printed (g, t, r.withTrimmedRight (20.0f), 12.0f, over ? col::print : col::amber, juce::Justification::centredRight, 0.12f);
    if (hasKeyboardFocus (false)) { g.setColour (col::amber); g.drawRoundedRectangle (r.reduced (1.0f), 4.0f, 1.2f); }
}

MainView::~MainView() { setLookAndFeel (nullptr); }

void MainView::showPage (int p)
{
    page = p;
    for (auto* b : voiceBtns) b->setVisible (p == 0);
    patchEditor.setVisible (p == 1);
    chipView.setVisible (p == 2);
    if (p == 1) patchEditor.refresh();
    tabSounds.setToggleState (p == 0, juce::dontSendNotification);
    tabEdit.setToggleState (p == 1, juce::dontSendNotification);
    tabChip.setToggleState (p == 2, juce::dontSendNotification);
    repaint();
}

void MainView::selectVoice (int v)
{
    if (auto* prm = proc.apvts.getParameter ("voice"))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) juce::jlimit (0, ym::NUM_VOICES - 1, v)));
        prm->endChangeGesture();
    }
    keyboard.grabKeyboardFocus();
}

void MainView::step (int d) { selectVoice ((proc.currentVoice() + d + ym::NUM_VOICES) % ym::NUM_VOICES); }

void MainView::timerCallback()
{
    if (++updateTick >= 60) { updateTick = 0; refreshUpdate(); }
    const int v = proc.currentVoice();
    if (v != shownVoice)
    {
        shownVoice = v;
        for (auto* b : voiceBtns) { const bool s = b->index == v; if (s != b->selected) { b->selected = s; b->repaint(); } }
        if (page == 1) patchEditor.refresh();
        repaint (displayArea.expanded (8));
    }
    if (++editTick >= 15)
    {
        editTick = 0;
        for (auto* b : voiceBtns)
        {
            const bool e = proc.isEdited (b->index);
            if (e != b->edited) { b->edited = e; b->repaint(); }
        }
        repaint (displayArea);
    }
    const int mask = proc.activeChannels.load();
    meter = juce::jmax (proc.outLevel.load(), meter * 0.82f);
    const int total = proc.activeTotal.load();
    if (mask != shownMask || total != shownTotal) { shownMask = mask; shownTotal = total; repaint (displayArea); }
    if (std::abs (meter - shownMeter) > 0.002f) { shownMeter = meter; repaint (grilleArea); }
    if (page == 2) chipView.tick();
}

static void drawSevenSeg (juce::Graphics& g, juce::Rectangle<float> r, int digit)
{
    static const uint8_t map[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };
    const float w = r.getWidth(), h = r.getHeight(), t = w * 0.17f, gp = t * 0.18f;
    const float x0 = r.getX(), y0 = r.getY(), ym = y0 + h / 2;
    auto hseg = [&] (float y) { juce::Path p; const float a = x0 + gp + t / 2, b = x0 + w - gp - t / 2;
        p.startNewSubPath (a, y); p.lineTo (a + t / 2, y - t / 2); p.lineTo (b - t / 2, y - t / 2); p.lineTo (b, y); p.lineTo (b - t / 2, y + t / 2); p.lineTo (a + t / 2, y + t / 2); p.closeSubPath(); return p; };
    auto vseg = [&] (float x, float ya, float yb) { juce::Path p; const float a = ya + gp + t / 2, b = yb - gp - t / 2;
        p.startNewSubPath (x, a); p.lineTo (x + t / 2, a + t / 2); p.lineTo (x + t / 2, b - t / 2); p.lineTo (x, b); p.lineTo (x - t / 2, b - t / 2); p.lineTo (x - t / 2, a + t / 2); p.closeSubPath(); return p; };
    const float yt = y0 + t / 2, yb = y0 + h - t / 2, xl = x0 + t / 2, xr = x0 + w - t / 2;
    juce::Path segs[7] = { hseg (yt), vseg (xr, yt, ym), vseg (xr, ym, yb), hseg (yb), vseg (xl, ym, yb), vseg (xl, yt, ym), hseg (ym) };
    const auto shear = juce::AffineTransform::shear (-0.08f, 0.0f).translated (0.08f * (y0 + h), 0.0f);
    for (int s = 0; s < 7; ++s)
    {
        const bool on = (map[digit % 10] >> s) & 1;
        segs[s].applyTransform (shear);
        if (on) { g.setColour (col::led.withAlpha (0.25f)); g.strokePath (segs[s], juce::PathStrokeType (5.0f)); }
        g.setColour (on ? col::led : col::ledOff);
        g.fillPath (segs[s]);
    }
}

void MainView::paintDisplay (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto r = area.toFloat();
    printed (g, "SON", r.withY (r.getY() - 22.0f).withHeight (16.0f), 13.0f, col::teal, juce::Justification::centredLeft, 0.14f);
    g.setColour (juce::Colour (0xff050506)); g.fillRoundedRectangle (r.expanded (4.0f), 7.0f);
    g.setColour (col::ledBg); g.fillRoundedRectangle (r, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.03f));
    g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.42f).reduced (3.0f, 3.0f), 4.0f);

    const int v = juce::jmax (0, shownVoice);
    auto in = r.reduced (18.0f, 14.0f);
    auto digits = in.removeFromLeft (112.0f);
    const float dh = 78.0f, dw = 46.0f;
    auto d1 = juce::Rectangle<float> (digits.getX(), digits.getCentreY() - dh / 2 - 4.0f, dw, dh);
    drawSevenSeg (g, d1, v / 10);
    drawSevenSeg (g, d1.translated (dw + 12.0f, 0), v % 10);
    in.removeFromLeft (14.0f);

    const auto name = escoita::maiusculas (BarulloProcessor::voiceName (v));
    auto nameR = in.removeFromTop (40.0f);
    g.setFont (juce::Font (juce::FontOptions (tf (2)).withHeight (36.0f).withKerningFactor (0.03f)));
    g.setColour (col::led.withAlpha (0.14f));
    for (auto o : { juce::Point<float> (-1.5f, 0), { 1.5f, 0 }, { 0, -1.5f }, { 0, 1.5f } })
        g.drawText (name, nameR.translated (o.x, o.y), juce::Justification::bottomLeft);
    g.setColour (col::led); g.drawText (name, nameR, juce::Justification::bottomLeft);
    g.setFont (body (14.0f));
    g.setColour (col::led.withAlpha (0.7f));
    juce::String sub = "Unha canle por nota";
    if (BarulloProcessor::voiceParts (v) > 1)
    {
        const int a = proc.getPatch (v)[0].g7, b = proc.getPatch (v)[1].g7;
        const bool stereo = ((a >> 5) & 3) != 3 && ((b >> 5) & 3) != 3 && ((a >> 5) & 3) != ((b >> 5) & 3);
        sub = stereo ? juce::String::fromUTF8 ("Dúas canles por nota: unha á esquerda e outra á dereita") : juce::String::fromUTF8 ("Dúas canles por nota, superpostas no centro");
    }
    if (proc.isEdited (v)) sub += ", editado";
    g.drawText (sub, in.removeFromTop (20.0f), juce::Justification::centredLeft);
    g.setColour (col::led.withAlpha (0.5f));
    g.setFont (body (13.5f));
    g.drawFittedText (BarulloProcessor::voiceDesc (v), in.removeFromTop (34.0f).toNearestInt(), juce::Justification::topLeft, 2, 0.95f);

    auto row = in.removeFromBottom (14.0f);
    const int mask = juce::jmax (0, shownMask);
    for (int c = 0; c < 8; ++c)
    {
        auto led = juce::Rectangle<float> (row.getX() + (float) c * 15.0f, row.getCentreY() - 3.5f, 7.0f, 7.0f);
        const bool on = (mask >> c) & 1;
        if (on) { g.setColour (col::led.withAlpha (0.3f)); g.fillEllipse (led.expanded (3.0f)); }
        g.setColour (on ? col::led : col::ledOff.brighter (0.3f));
        g.fillEllipse (led);
    }
    g.setColour (col::led.withAlpha (0.45f)); g.setFont (body (12.5f));
    const int extra = juce::jmax (0, shownTotal) - [&] { int n = 0, m = mask; while (m) { n += m & 1; m >>= 1; } return n; }();
    g.drawText (extra > 0 ? U ("canles do chip en uso, e ") + juce::String (extra) + U (" máis en chips virtuais")
                          : U ("canles do chip en uso"),
                row.withTrimmedLeft (8 * 15.0f + 6.0f), juce::Justification::centredLeft);
}

// rejilla del altavoz: agujeros en el plástico negro que se encienden con el nivel
void MainView::paintGrille (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto r = area.toFloat();
    const auto c = r.getCentre();
    const float R = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f - 4.0f;
    const float lvlDb = juce::Decibels::gainToDecibels (meter, -48.0f);
    const float lit = juce::jlimit (0.0f, 1.0f, (lvlDb + 48.0f) / 48.0f) * R;
    const float step = 9.0f;
    for (float y = -R; y <= R + 0.1f; y += step * 0.866f)
    {
        const int rowI = juce::roundToInt ((y + R) / (step * 0.866f));
        for (float x = -R + ((rowI & 1) ? step / 2 : 0); x <= R + 0.1f; x += step)
        {
            const float d = std::sqrt (x * x + y * y);
            if (d > R) continue;
            auto hole = juce::Rectangle<float> (c.x + x - 2.7f, c.y + y - 2.7f, 5.4f, 5.4f);
            const bool on = d <= lit && meter > 0.0005f;
            g.setColour (juce::Colours::white.withAlpha (0.07f)); g.fillEllipse (hole.translated (0, 0.8f));
            g.setColour (on ? col::led.withAlpha (0.5f + 0.5f * (1.0f - d / (R + 1))) : juce::Colours::black);
            g.fillEllipse (hole);
        }
    }
}

void MainView::paint (juce::Graphics& g)
{
    g.fillAll (col::body);

    // marco impreso del panel superior, como en el teclado
    auto frame = juce::Rectangle<float> (16.0f, 10.0f, (float) getWidth() - 32.0f, (float) (keysFrame.getY() - 22));
    g.setColour (col::line); g.drawRoundedRectangle (frame, 3.0f, 1.2f);

    printed (g, "BARULLO FM", { 32.0f, 20.0f, 220.0f, 36.0f }, 30.0f, col::print, juce::Justification::centredLeft, 0.04f);
    printed (g, U ("SÍNTESE FM · 2 OPERADORES · 8 CANLES"), { 236.0f, 28.0f, 420.0f, 22.0f }, 13.5f, col::gold, juce::Justification::centredLeft, 0.12f);

    paintGrille (g, grilleArea);
    paintDisplay (g, displayArea);

    printed (g, "VOLUME", volArea.toFloat().removeFromTop (16.0f), 12.5f, col::teal, juce::Justification::centred, 0.14f);
    auto pa = perfArea.toFloat();
    printed (g, "BEND", pa.withHeight (16.0f), 12.5f, col::teal, juce::Justification::centredLeft, 0.14f);
    printed (g, "VELOCIDADE", pa.withY ((float) vel.getY() - 18.0f).withHeight (16.0f), 12.5f, col::teal, juce::Justification::centredLeft, 0.14f);

    auto lab = [&] (juce::Component& from, juce::Component& to, const char* t)
    {
        auto rr = from.getBounds().getUnion (to.getBounds()).toFloat();
        printed (g, U (t), rr.withY (rr.getY() - 19.0f).withHeight (15.0f), 12.5f, col::teal, juce::Justification::centred, 0.14f);
    };
    lab (tabSounds, tabChip, "PANEL");
    lab (filmsBtn, unlimitedBtn, "FUNCIÓNS");
    lab (eqSpeaker, eqFlat, "ECUALIZADOR");

    if (page == 0)
    {
        g.setColour (col::line);
        g.drawHorizontalLine (chartArea.getY() - 8, (float) chartArea.getX(), (float) chartArea.getRight());
    }

    auto kf = keysFrame.toFloat();
    g.setColour (juce::Colour (0xff050506)); g.fillRoundedRectangle (kf, 5.0f);
}

void MainView::resized()
{
    escoita.setBounds (getWidth() - 32 - 160, 22, 160, 30);
    updateBtn.setBounds (getWidth() - 32 - 160 - 8 - 340, 22, 340, 30);
    auto r = getLocalBounds().reduced (32, 0);
    r.removeFromTop (64);

    auto head = r.removeFromTop (150);
    grilleArea = head.removeFromLeft (132).withSizeKeepingCentre (132, 132).translated (0, 6);
    head.removeFromLeft (22);
    auto right = head.removeFromRight (252);
    auto arrows = head.removeFromRight (50);
    head.removeFromRight (14);
    displayArea = head.withTrimmedTop (22).withTrimmedBottom (2);
    auto ar = arrows.withTrimmedTop (22);
    prevBtn.setBounds (ar.removeFromTop (ar.getHeight() / 2).reduced (2, 3));
    nextBtn.setBounds (ar.reduced (2, 3));

    right.removeFromLeft (16);
    volArea = right.removeFromLeft (70);
    volume.setBounds (volArea.withTrimmedTop (18));
    right.removeFromLeft (14);
    perfArea = right;
    auto pr = right.withTrimmedTop (18);
    bend.setBounds (pr.removeFromTop (34));
    pr.removeFromTop (28);
    vel.setBounds (pr.removeFromTop (34));

    r.removeFromTop (34);
    auto bar = r.removeFromTop (36);
    tabSounds.setBounds (bar.removeFromLeft (112)); bar.removeFromLeft (6);
    tabEdit.setBounds (bar.removeFromLeft (94)); bar.removeFromLeft (6);
    tabChip.setBounds (bar.removeFromLeft (80));
    auto sw = bar.removeFromRight (600);
    eqFlat.setBounds (sw.removeFromRight (88)); sw.removeFromRight (6);
    eqSpeaker.setBounds (sw.removeFromRight (106)); sw.removeFromRight (20);
    unlimitedBtn.setBounds (sw.removeFromRight (122)); sw.removeFromRight (6);
    speakerBtn.setBounds (sw.removeFromRight (124)); sw.removeFromRight (6);
    filmsBtn.setBounds (sw.removeFromRight (128));
    r.removeFromTop (24);

    auto keys = r.removeFromBottom (114);
    keysFrame = keys.withTrimmedBottom (10).expanded (16, 0);
    keyboard.setBounds (keysFrame.reduced (6, 6));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f);
    r.removeFromBottom (32);

    chartArea = r;
    legendArea = {};
    patchEditor.setBounds (r.withTrimmedTop (-8));
    chipView.setBounds (r.withTrimmedTop (-8));
    const int cols = 10, rows = 10;
    const float cw = (float) r.getWidth() / (float) cols, ch = (float) r.getHeight() / (float) rows;
    for (int i = 0; i < voiceBtns.size(); ++i)
    {
        const int cx = i / rows, cy = i % rows;   // por columnas, como la lista impresa del teclado
        voiceBtns[i]->setBounds (juce::Rectangle<float> ((float) r.getX() + (float) cx * cw, (float) r.getY() + (float) cy * ch, cw, ch).toNearestInt());
    }
}
} // namespace ui

// ================================================================ ventana (escalable)
BarulloEditor::BarulloEditor (BarulloProcessor& p) : AudioProcessorEditor (&p), proc (p), main (p)
{
    addAndMakeVisible (main);
    setResizable (true, true);
    setResizeLimits (ui::MainView::W * 6 / 10, ui::MainView::H * 6 / 10, ui::MainView::W * 2, ui::MainView::H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ui::MainView::W / ui::MainView::H);
    const float s = proc.uiScale;
    setSize (juce::roundToInt (ui::MainView::W * s), juce::roundToInt (ui::MainView::H * s));
}

void BarulloEditor::resized()
{
    const float s = (float) getWidth() / (float) ui::MainView::W;
    main.setBounds (0, 0, ui::MainView::W, ui::MainView::H);
    main.setTransform (juce::AffineTransform::scale (s));
    proc.uiScale = s;
}
