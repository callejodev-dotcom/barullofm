// Prueba sin interfaz del procesador: toca una secuencia con dos tamaños de bloque,
// comprueba que el resultado es idéntico (todo va a la muestra exacta), mide el coste
// y guarda un WAV para escuchar.
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioBuffer<float> run (int blockSize, double sr, double seconds, double* cpuSeconds)
{
    BarulloProcessor p;
    p.setPlayConfigDetails (0, 2, sr, blockSize);
    p.prepareToPlay (sr, blockSize);
    if (auto* prm = p.apvts.getParameter ("velocity")) prm->setValueNotifyingHost (prm->convertTo0to1 (70.0f));
    p.setPatchReg (0, 0, 2, 0x13 | 0x20);   // sonido 0: modulador a media onda
    p.setPatchReg (0, 0, 16, 0xE5);          // y realimentación 5
    const int total = (int) (seconds * sr);
    juce::AudioBuffer<float> out (2, total), buf (2, blockSize);

    // secuencia: [segundo, tipo, dato]
    struct Ev { double t; int type; int a; };   // 0 on, 1 off, 2 prog, 3 ped on, 4 ped off, 5 bend, 6 rueda
    std::vector<Ev> seq;
    double t = 0.05;
    for (int v : { 0, 23, 25, 50, 53, 86, 98 })
    {
        seq.push_back ({ t, 2, v }); t += 0.01;
        for (int n : { 60, 64, 67 }) seq.push_back ({ t, 0, n });
        t += 1.2;
        for (int n : { 60, 64, 67 }) seq.push_back ({ t, 1, n });
        t += 0.3;
        for (int k = 0; k < 6; ++k) { seq.push_back ({ t, 0, 72 + k * 2 }); t += 0.12; seq.push_back ({ t, 1, 72 + k * 2 }); }
        seq.push_back ({ t, 3, 0 }); seq.push_back ({ t, 0, 48 }); t += 0.2; seq.push_back ({ t, 1, 48 }); t += 0.6; seq.push_back ({ t, 4, 0 });
        t += 1.0;
    }
    // bend, rueda de modulación y velocidad (con el sonido 0 editado)
    seq.push_back ({ t, 2, 0 }); t += 0.01;
    seq.push_back ({ t, 0, 60 });
    for (int i = 0; i <= 40; ++i) seq.push_back ({ t + i * 0.01, 5, 8192 + (int) (8191 * std::sin (i * 0.1)) });
    t += 0.6; seq.push_back ({ t, 5, 8192 });
    for (int i = 0; i <= 20; ++i) seq.push_back ({ t + i * 0.02, 6, i * 6 });
    t += 1.2; seq.push_back ({ t, 6, 0 }); t += 0.3; seq.push_back ({ t, 1, 60 });
    for (int v : { 20, 60, 100, 127 }) { t += 0.3; seq.push_back ({ t, 0, 64 + v % 7 }); seq.push_back ({ t + 0.25, 1, 64 + v % 7 }); }
    t += 1.0;

    size_t k = 0;
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    for (int pos = 0; pos < total; pos += blockSize)
    {
        const int n = juce::jmin (blockSize, total - pos);
        juce::MidiBuffer midi;
        while (k < seq.size() && (int) (seq[k].t * sr) < pos + n)
        {
            const int at = juce::jmax (0, (int) (seq[k].t * sr) - pos);
            const auto& e = seq[k++];
            juce::MidiMessage m = e.type == 5 ? juce::MidiMessage::pitchWheel (1, e.a)
                               : e.type == 6 ? juce::MidiMessage::controllerEvent (1, 1, e.a)
                               : e.type == 0 ? juce::MidiMessage::noteOn (1, e.a, (juce::uint8) (40 + (e.a * 13) % 88))
                               : e.type == 1 ? juce::MidiMessage::noteOff (1, e.a)
                               : e.type == 2 ? juce::MidiMessage::programChange (1, e.a)
                               : juce::MidiMessage::controllerEvent (1, 64, e.type == 3 ? 127 : 0);
            midi.addEvent (m, at);
        }
        buf.setSize (2, n, false, false, true);
        buf.clear();
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
    }
    *cpuSeconds = (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0;
    return out;
}

static void snapshot (const juce::File& png, int voice, int page)
{
    BarulloProcessor p;
    p.setCurrentProgram (voice);
    if (page == 1) { p.setPatchReg (voice, 0, 4, 40); p.setPatchReg (voice, 0, 16, p.getPatch (voice)[0].g7 | 3); }
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    for (int n : { 60, 64, 67 }) p.keyboardState.noteOn (1, n, 1.0f);
    p.activeChannels.store (0b00000111); p.outLevel.store (0.3f);
    if (auto* ce = dynamic_cast<BarulloEditor*> (ed.get()))
    {
        ce->view().showPage (page);
        for (int i = 0; i < 16; ++i) ce->view().timerCallback();
        if (page == 2)   // que pase algo por el bus: un acorde tocado de verdad
        {
            p.prepareToPlay (48000, 512);
            juce::AudioBuffer<float> buf (2, 512);
            for (int blk = 0; blk < 40; ++blk)
            {
                juce::MidiBuffer m;
                if (blk == 0) for (int n : { 60, 64, 67 }) m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
                if (blk == 20) m.addEvent (juce::MidiMessage::noteOn (1, 72, (juce::uint8) 100), 0);
                buf.clear(); p.processBlock (buf, m);
                if (blk == 10) ce->view().timerCallback();
            }
            ce->view().timerCallback();
        }
    }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    png.deleteFile();
    juce::FileOutputStream os (png);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("PNG: %s\n", png.getFullPathName().toRawUTF8());
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc > 2 && juce::String (argv[1]) == "--snapshot") { snapshot (juce::File (juce::String (argv[2])), argc > 3 ? atoi (argv[3]) : 0, argc > 4 ? atoi (argv[4]) : 0); return 0; }
    {   // guardar y recuperar el estado con sonidos editados
        BarulloProcessor a, b;
        a.setPatchReg (41, 1, 4, 77); a.setPatchReg (41, 0, 16, 0x41); a.setPatchReg (99, 0, 9, 0x3F);
        if (auto* prm = a.apvts.getParameter ("voice")) prm->setValueNotifyingHost (prm->convertTo0to1 (41.0f));
        a.uiScale = 1.25f;
        juce::MemoryBlock mb; a.getStateInformation (mb);
        b.setPatchReg (5, 0, 4, 1);   // algo que debe desaparecer al cargar
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        bool ok = b.currentVoice() == 41 && std::abs (b.uiScale - 1.25f) < 1e-6f && ! b.isEdited (5);
        for (int v = 0; v < ym::NUM_VOICES; ++v)
            for (int pi = 0; pi < 2; ++pi)
            {
                ok = ok && a.getPatch (v)[(size_t) pi].g7 == b.getPatch (v)[(size_t) pi].g7;
                for (int k = 2; k <= 13; ++k) ok = ok && a.getPatch (v)[(size_t) pi].r[k] == b.getPatch (v)[(size_t) pi].r[k];
            }
        std::printf ("estado guardado/recuperado: %s (%d bytes)\n", ok ? "OK" : "FALLO", (int) mb.getSize());
        if (! ok) return 2;
    }
    const double sr = 48000, seconds = 36;
    double c1 = 0, c2 = 0;
    auto a = run (512, sr, seconds, &c1);
    auto b = run (37, sr, seconds, &c2);   // bloque raro a propósito

    float maxDiff = 0, peak = 0;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < a.getNumSamples(); ++i)
        {
            maxDiff = juce::jmax (maxDiff, std::abs (a.getSample (c, i) - b.getSample (c, i)));
            peak = juce::jmax (peak, std::abs (a.getSample (c, i)));
        }
    std::printf ("bloque 512: %.3f s de CPU para %.0f s de audio (%.0fx tiempo real)\n", c1, seconds, seconds / c1);
    std::printf ("bloque 37:  %.3f s de CPU (%.0fx tiempo real)\n", c2, seconds / c2);
    std::printf ("diferencia maxima entre tamanos de bloque: %g   pico: %.3f\n", maxDiff, peak);

    const juce::File f (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getChildFile ("barullo_test.wav").getFullPathName());
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (a, 0, a.getNumSamples());
            std::printf ("WAV: %s\n", f.getFullPathName().toRawUTF8());
        }
    return maxDiff == 0.0f ? 0 : 1;
}
