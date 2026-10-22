#include "PluginProcessor.h"
#include "PluginEditor.h"

BarulloProcessor::BarulloProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "BarulloFM", createLayout())
{
    voiceParam   = apvts.getRawParameterValue ("voice");
    filmsParam   = apvts.getRawParameterValue ("films");
    eqParam      = apvts.getRawParameterValue ("eq");
    speakerParam = apvts.getRawParameterValue ("speaker");
    volumeParam  = apvts.getRawParameterValue ("volume");
    bendParam    = apvts.getRawParameterValue ("bend");
    velParam     = apvts.getRawParameterValue ("velocity");
    unlimitedParam = apvts.getRawParameterValue ("unlimited");
    for (int v = 0; v < ym::NUM_VOICES; ++v) patches[(size_t) v] = originalPatch (v);
    synth.hookCtx = &bus;
    synth.writeHook = [] (void* ctx, int a, int d)
    {
        auto& b = *static_cast<BusView*> (ctx);
        if (a >= 0 && a < 128)
        {
            b.regs[(size_t) a].store ((uint8_t) d, std::memory_order_relaxed);
            b.writes[(size_t) a].fetch_add (1, std::memory_order_relaxed);
        }
        b.lastData.store (d, std::memory_order_relaxed);
        b.total.fetch_add (1, std::memory_order_relaxed);
    };
}

// ---------------------------------------------------------------- sonidos editados
ym::Patch BarulloProcessor::originalPatch (int v)
{
    v = juce::jlimit (0, ym::NUM_VOICES - 1, v);
    return { ym::VOICES[v].parts[0], ym::VOICES[v].parts[1] };
}

bool BarulloProcessor::isEdited (int v) const
{
    const auto o = originalPatch (v);
    const auto& p = patches[(size_t) juce::jlimit (0, ym::NUM_VOICES - 1, v)];
    for (int pi = 0; pi < voiceParts (v); ++pi)
    {
        if (p[(size_t) pi].g7 != o[(size_t) pi].g7) return true;
        for (int k = 2; k <= 13; ++k) if (p[(size_t) pi].r[k] != o[(size_t) pi].r[k]) return true;
    }
    return false;
}

void BarulloProcessor::pushEdit (Edit e)
{
    const auto scope = editFifo.write (1);
    if (scope.blockSize1 > 0) editBuf[(size_t) scope.startIndex1] = e;
    else if (scope.blockSize2 > 0) editBuf[(size_t) scope.startIndex2] = e;
}

void BarulloProcessor::drainEdits()
{
    const auto scope = editFifo.read (editFifo.getNumReady());
    auto apply = [this] (int start, int size)
    {
        for (int i = start; i < start + size; ++i)
        {
            const auto& e = editBuf[(size_t) i];
            if (e.k < 0) synth.resetPatch (e.voice);
            else synth.setPatchReg (e.voice, e.part, e.k, e.value);
        }
    };
    apply (scope.startIndex1, scope.blockSize1);
    apply (scope.startIndex2, scope.blockSize2);
}

void BarulloProcessor::setPatchReg (int v, int part, int k, int value)
{
    if (v < 0 || v >= ym::NUM_VOICES || part < 0 || part > 1) return;
    auto& p = patches[(size_t) v][(size_t) part];
    if (k == 16) p.g7 = (uint8_t) value; else if (k >= 2 && k <= 13) p.r[k] = (uint8_t) (k == 3 ? (value & 0x7F) : value); else return;
    pushEdit ({ (int16_t) v, (int16_t) part, (int16_t) k, (int16_t) value });
}

void BarulloProcessor::resetPatch (int v)
{
    if (v < 0 || v >= ym::NUM_VOICES) return;
    patches[(size_t) v] = originalPatch (v);
    pushEdit ({ (int16_t) v, 0, -1, 0 });
}

// ---- os cambios da interface, que se poden desfacer (o estado do núcleo)
escoita::Estado::Opcions BarulloProcessor::opcionsEstado()
{
    escoita::Estado::Opcions o;
    o.extension = "bfmpreset";
    o.extra = [this] { return patchesState(); };
    o.aplicarExtra = [this] (const juce::ValueTree& t) { applyPatchesState (t); };
    return o;
}

void BarulloProcessor::setWholePatch (int v, const ym::Patch& p)
{
    for (int pi = 0; pi < 2; ++pi)
    {
        for (int k = 2; k <= 13; ++k) setPatchReg (v, pi, k, p[(size_t) pi].r[k]);
        setPatchReg (v, pi, 16, p[(size_t) pi].g7);
    }
}

void BarulloProcessor::editPatchReg (int v, int part, int k, int value)
{
    if (v < 0 || v >= ym::NUM_VOICES || part < 0 || part > 1) return;
    const auto& r = patches[(size_t) v][(size_t) part];
    const int old = k == 16 ? r.g7 : (k >= 2 && k <= 13 ? r.r[k] : -1);
    if (old < 0 || old == value) return;
    estado.cambio ("son", [this, v, part, k, value] { setPatchReg (v, part, k, value); },
                          [this, v, part, k, old] { setPatchReg (v, part, k, old); });
}

void BarulloProcessor::restorePatch (int v)
{
    if (v < 0 || v >= ym::NUM_VOICES || ! isEdited (v)) return;
    const auto old = patches[(size_t) v];
    estado.cambio ("restaurar", [this, v] { resetPatch (v); }, [this, v, old] { setWholePatch (v, old); });
}

juce::ValueTree BarulloProcessor::patchesState() const
{
    juce::ValueTree pt ("Patches");
    for (int v = 0; v < ym::NUM_VOICES; ++v)
        if (isEdited (v))
        {
            juce::ValueTree e ("Patch");
            e.setProperty ("voice", v, nullptr);
            e.setProperty ("regs", patchToString (patches[(size_t) v], voiceParts (v)), nullptr);
            pt.appendChild (e, nullptr);
        }
    return pt;
}

void BarulloProcessor::applyPatchesState (const juce::ValueTree& pt)
{
    // sons: todos ao orixinal e despois os editados
    for (int v = 0; v < ym::NUM_VOICES; ++v) if (isEdited (v)) resetPatch (v);
    for (const auto e : pt)
    {
        const int v = e.getProperty ("voice", -1);
        if (v < 0 || v >= ym::NUM_VOICES) continue;
        for (const auto& tok : juce::StringArray::fromTokens (e.getProperty ("regs").toString(), " ", ""))
        {
            const int a = tok.upToFirstOccurrenceOf ("=", false, false).getHexValue32();
            const int d = tok.fromFirstOccurrenceOf ("=", false, false).getHexValue32() & 0xFF;
            const int g = a >> 4;
            if (g == 7) { setPatchReg (v, a & 1, 16, d); continue; }
            if (g < 1 || g > 6) continue;
            setPatchReg (v, a & 1, g * 2 + ((a >> 1) & 1), d);
        }
    }
}

// "10=13 12=81 …" como en el emulador web (direcciones del canal 0 / canal 1)
juce::String BarulloProcessor::patchToString (const ym::Patch& p, int nParts)
{
    static const char* names[] = { "", "", "10", "12", "20", "22", "30", "32", "40", "42", "50", "52", "60", "62" };
    juce::StringArray out;
    for (int pi = 0; pi < nParts; ++pi)
    {
        for (int k = 2; k <= 13; ++k)
        {
            int a = juce::String (names[k]).getHexValue32() | pi;
            out.add (juce::String::toHexString (a).toUpperCase().paddedLeft ('0', 2) + "=" + juce::String::toHexString ((int) p[(size_t) pi].r[k]).toUpperCase().paddedLeft ('0', 2));
        }
        out.add (juce::String::toHexString (0x70 + pi).toUpperCase() + "=" + juce::String::toHexString ((int) p[(size_t) pi].g7).toUpperCase().paddedLeft ('0', 2));
    }
    return out.joinIntoString (" ");
}

juce::AudioProcessorValueTreeState::ParameterLayout BarulloProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "voice", 1 }, "Son", 0, ym::NUM_VOICES - 1, 0,
        AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) {
            return String (v).paddedLeft ('0', 2) + " " + BarulloProcessor::voiceName (v); })));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "films", 1 }, "Movemento", true));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "eq", 1 }, "Ecualizador", StringArray { "Altofalante", "Plano" }, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "speaker", 1 }, juce::String::fromUTF8 ("Saturación de altofalante"), false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume",
        NormalisableRange<float> (-30.0f, 6.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "unlimited", 1 }, juce::String::fromUTF8 ("Sen límite de notas"), false));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { "bend", 1 }, "Alcance do bend", 0, 12, 2,
        AudioParameterIntAttributes().withLabel ("st")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "velocity", 1 }, "Velocidade",
        NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("%")));
    return l;
}

juce::String BarulloProcessor::voiceName (int i)
{
    i = juce::jlimit (0, ym::NUM_VOICES - 1, i);
    const juce::String n = juce::String::fromUTF8 (ym::VOICES[i].name);
    return n.isEmpty() ? juce::String ("Sen nome") : n;
}
juce::String BarulloProcessor::voiceDesc (int i)
{
    return juce::String::fromUTF8 (ym::VOICES[juce::jlimit (0, ym::NUM_VOICES - 1, i)].desc);
}
int BarulloProcessor::voiceParts (int i) { return ym::VOICES[juce::jlimit (0, ym::NUM_VOICES - 1, i)].nParts; }

const juce::String BarulloProcessor::getProgramName (int index)
{
    return juce::String (index).paddedLeft ('0', 2) + " " + voiceName (index);
}

void BarulloProcessor::setCurrentProgram (int index)
{
    if (auto* p = apvts.getParameter ("voice"))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (0, ym::NUM_VOICES - 1, index)));
}

bool BarulloProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void BarulloProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    scratch.assign ((size_t) juce::jmax (samplesPerBlock, 4096), 0.0f);
    setLatencySamples (0);   // sin latencia: el chip se calcula muestra a muestra, sin anticipación
    synth.prepare (sampleRate);
    fx.prepare (sampleRate);
    lastEq = lastSpeaker = -1; lastVolume = -1000.0f;
    syncParams();
    fx.reset();
    keyboardState.reset();
}

void BarulloProcessor::syncParams()
{
    synth.setVoice ((int) voiceParam->load());
    synth.setFilms (filmsParam->load() > 0.5f);
    synth.setUnlimited (unlimitedParam->load() > 0.5f);
    synth.setBendRange (bendParam->load());
    synth.setVelocitySens (velParam->load() / 100.0);
    const int eq = (int) eqParam->load(), sp = speakerParam->load() > 0.5f ? 1 : 0;
    const float vol = volumeParam->load();
    if (eq != lastEq || sp != lastSpeaker || std::abs (vol - lastVolume) > 1.0e-4f)
    {
        fx.eqOn = (eq == 0); fx.speakerOn = sp; fx.volumeDb = vol;
        fx.update();
        lastEq = eq; lastSpeaker = sp; lastVolume = vol;
    }
}

void BarulloProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn()) synth.midiNoteOn (m.getNoteNumber(), m.getVelocity());
    else if (m.isNoteOff()) synth.midiNoteOff (m.getNoteNumber());
    else if (m.isSustainPedalOn()) synth.midiSustain (true);
    else if (m.isSustainPedalOff()) synth.midiSustain (false);
    else if (m.isAllNotesOff() || m.isAllSoundOff()) synth.allNotesOff();
    else if (m.isPitchWheel()) synth.midiPitchBend ((m.getPitchWheelValue() - 8192) / 8192.0);
    else if (m.isControllerOfType (1)) synth.midiModWheel (m.getControllerValue() / 127.0);
    else if (m.isResetAllControllers()) synth.resetControllers();
    else if (m.isProgramChange())
    {
        // cambia el sonido ya, y avisa al parámetro para que la interfaz y el anfitrión lo vean
        synth.setVoice (m.getProgramChangeNumber() % ym::NUM_VOICES);
        pendingProgram = m.getProgramChangeNumber() % ym::NUM_VOICES;
    }
}

void BarulloProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    keyboardState.processNextMidiBuffer (midi, 0, n, true);   // añade lo que se toque en el teclado de la pantalla
    syncParams();
    drainEdits();

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    if (! R)
    {
        if (scratch.size() < (size_t) n) scratch.resize ((size_t) n);   // solo si el anfitrión manda más de lo anunciado
        R = scratch.data();
    }

    // cada evento MIDI entra en su muestra exacta
    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, n, meta.samplePosition);
        if (t > pos) { synth.render (L + pos, R + pos, t - pos); pos = t; }
        handleMidi (meta.getMessage());
    }
    if (pos < n) synth.render (L + pos, R + pos, n - pos);

    fx.process (L, R, n);

    if (buffer.getNumChannels() == 1)
        for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + R[i]);
    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);

    activeChannels.store (synth.chip.activeMask());
    activeTotal.store (synth.activeTotal());
    outLevel.store (buffer.getMagnitude (0, n));

    if (pendingProgram >= 0)
    {
        const int p = pendingProgram; pendingProgram = -1;
        if (auto* prm = apvts.getParameter ("voice"))
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) p));
    }
}

void BarulloProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.removeChild (st.getChildWithName ("Patches"), nullptr);
    st.appendChild (patchesState(), nullptr);
    st.setProperty ("uiScale", uiScale, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, destData);
}

void BarulloProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    uiScale = juce::jlimit (0.6f, 2.0f, (float) tree.getProperty ("uiScale", 1.0f));
    applyPatchesState (tree.getChildWithName ("Patches"));
    tree.removeChild (tree.getChildWithName ("Patches"), nullptr);
    apvts.replaceState (tree);
}

juce::AudioProcessorEditor* BarulloProcessor::createEditor() { return new BarulloEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BarulloProcessor(); }
