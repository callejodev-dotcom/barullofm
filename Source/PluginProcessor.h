#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Synth.h"
#include "Fx.h"

class BarulloProcessor : public juce::AudioProcessor
{
public:
    BarulloProcessor();
    ~BarulloProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Barullo FM"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    // Los 100 sonidos también como "programas" del anfitrión
    int getNumPrograms() override { return ym::NUM_VOICES; }
    int getCurrentProgram() override { return currentVoice(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    int currentVoice() const { return (int) voiceParam->load(); }
    static juce::String voiceName (int i);
    static juce::String voiceDesc (int i);
    static int voiceParts (int i);

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    // Para la pantalla: qué canales del chip suenan (bit por canal)
    std::atomic<int> activeChannels { 0 };
    std::atomic<int> activeTotal { 0 };      // canales sonando en todos los chips (con Sin límite)
    std::atomic<float> outLevel { 0 };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // ---- sonidos editados (llamar solo desde el hilo de la interfaz) ----
    // k: 2..13 = grupo*2 + operador, 16 = registro de canal
    const ym::Patch& getPatch (int v) const { return patches[(size_t) juce::jlimit (0, ym::NUM_VOICES - 1, v)]; }
    bool isEdited (int v) const;
    void setPatchReg (int v, int part, int k, int value);
    void resetPatch (int v);
    static ym::Patch originalPatch (int v);
    static juce::String patchToString (const ym::Patch&, int nParts);

    // lo que pasa por el bus del chip, para la página "Chip" (se escribe desde el hilo de audio)
    struct BusView
    {
        std::array<std::atomic<uint8_t>, 128> regs {};
        std::array<std::atomic<uint32_t>, 128> writes {};
        std::atomic<int> lastData { 0 };
        std::atomic<uint32_t> total { 0 };
    };
    BusView bus;

    // tamaño de la ventana (se guarda con el proyecto)
    float uiScale = 1.0f;

private:
    ym::Synth synth;
    ym::OutputChain fx;

    std::atomic<float>* voiceParam = nullptr;
    std::atomic<float>* filmsParam = nullptr;
    std::atomic<float>* eqParam = nullptr;
    std::atomic<float>* speakerParam = nullptr;
    std::atomic<float>* volumeParam = nullptr;
    std::atomic<float>* bendParam = nullptr;
    std::atomic<float>* velParam = nullptr;
    std::atomic<float>* unlimitedParam = nullptr;

    // cambios del editor hacia el hilo de audio, sin bloqueos
    struct Edit { int16_t voice, part, k, value; };   // k = -1: restaurar el sonido
    juce::AbstractFifo editFifo { 8192 };
    std::vector<Edit> editBuf = std::vector<Edit> (8192);
    std::array<ym::Patch, ym::NUM_VOICES> patches {};   // copia de la interfaz
    void pushEdit (Edit);
    void drainEdits();
    std::vector<float> scratch;

    int lastEq = -1, lastSpeaker = -1; float lastVolume = -1000.0f;
    int pendingProgram = -1;

    void syncParams();
    void handleMidi (const juce::MidiMessage&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BarulloProcessor)
};
