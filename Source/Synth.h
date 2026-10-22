// Synth.h — El "firmware": lo que hace el procesador del teclado con el chip.
// Traducción del emulador web: reparto de 8 canales, carga de sonidos, películas
// (los registros que el teclado real reescribe mientras suena la nota: vibratos,
// trémolos, barridos), caída al soltar grabada por sonido y pedal de sostén.
// Diferencia con la web: aquí todo va a la muestra exacta (allí, cada 4 ms y en bloques de 128).
//
// Extras de plugin (todos neutros por defecto: con bend 0, rueda 0 y velocidad 0 las
// escrituras al chip son exactamente las del teclado):
//  - bend de tono: reescribe la frecuencia (fnum/bloque) de lo que suena, como hace el teclado con su vibrato;
//  - rueda de modulación: el vibrato por software del emulador (5,5 Hz, reescritura cada 12 ms);
//  - velocidad: atenúa el nivel de la portadora (registro de nivel de 8 bits);
//  - sonidos editados: cada sonido puede llevar sus registros cambiados (editor).
#pragma once
#include "Chip.h"
#include "VoiceData.h"
#include <escoita_nucleo/escoita_nucleo.h>   // o pedal de sostemento e o reparto de voces, do núcleo común
#include <array>
#include <cmath>
#include <cstdint>
#include <climits>
#include <vector>

namespace ym {

using Patch = std::array<Part, 2>;

class Synth
{
public:
    Synth()
    {
        for (int v = 0; v < NUM_VOICES; ++v) user[(size_t) v] = { VOICES[v].parts[0], VOICES[v].parts[1] };
        sost.empezar = [this] (int n, int vel) { noteOn (n, vel); };
        sost.acabar = [this] (int n) { noteOff (n); };
        reset();
    }
    Synth (const Synth&) = delete;   // o pedal chama a este obxecto: non se copia
    Synth& operator= (const Synth&) = delete;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (auto& c : chips) c.setSampleRate (sampleRate);
        const size_t tmpSize = 2048;
        tmpL.assign (tmpSize, 0.0f); tmpR.assign (tmpSize, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& c : chips) c.reset();
        shadow.fill (-1);
        for (auto& c : chans) c = ChanState{};
        for (auto& h : held) h = Held{};
        for (auto& f : films) f = Film{};
        sost.reiniciar();
        clock = 0; fidSeq = 0; stamp = 1; now = 0; nextVib = 0; vibWasOn = false;
        cur = user[(size_t) voice];
    }

    // ---- ajustes ----
    int  getVoice() const { return voice; }
    void setVoice (int v)
    {
        v = ((v % NUM_VOICES) + NUM_VOICES) % NUM_VOICES;
        if (v == voice) return;
        allNotesOff();
        voice = v; ++stamp;
        cur = user[(size_t) voice];
    }
    void setFilms (bool on) { if (on == filmsOn) return; allNotesOff(); filmsOn = on; ++stamp; }

    // Sin límite: chips virtuales extra, todos iguales al original (8 canales cada uno)
    void setUnlimited (bool on)
    {
        const int want = on ? MAXCH : 8;
        if (want == numCh) return;
        allNotesOff();
        numCh = want; ++stamp;
    }
    int activeTotal() const
    {
        int n = 0;
        for (int k = 0; k < numCh / 8; ++k) { int m = chips[(size_t) k].activeMask(); while (m) { n += m & 1; m >>= 1; } }
        return n;
    }
    bool getFilms() const { return filmsOn; }

    void setBendRange (double semis) { bendRange = semis; }
    void setVelocitySens (double s) { velSens = std::max (0.0, std::min (1.0, s)); }

    // ---- sonidos editados ----
    // k: 2..13 = grupo*2 + operador (como en el emulador), 16 = registro de canal (g7)
    void setPatchReg (int v, int pi, int k, int val)
    {
        if (v < 0 || v >= NUM_VOICES || pi < 0 || pi > 1) return;
        Part& p = user[(size_t) v][(size_t) pi];
        if (k == 16) p.g7 = (uint8_t) val; else if (k >= 2 && k <= 13) p.r[k] = (uint8_t) (k == 3 ? (val & 0x7F) : val); else return;
        if (v != voice) return;
        cur[(size_t) pi] = p;
        liveWrite (pi, k);
    }
    void resetPatch (int v)
    {
        if (v < 0 || v >= NUM_VOICES) return;
        user[(size_t) v] = { VOICES[v].parts[0], VOICES[v].parts[1] };
        if (v == voice) { cur = user[(size_t) v]; ++stamp; }
    }

    // ---- MIDI ----
    // o pedal de sostemento vai polo núcleo común (escoita::Sostido): o motor só ve as notas
    // que empezan e as que acaban de verdade
    void midiNoteOn (int n, int vel = 127) { sost.noteOn (n, vel); }
    void midiNoteOff (int n) { sost.noteOff (n); }
    void midiSustain (bool down) { sost.pedal (down); }
    void allNotesOff()
    {
        sost.reiniciar();
        for (int n = 0; n < 128; ++n) if (held[(size_t) n].on) noteOff (n);
    }
    // valor de la rueda de bend en −1..+1
    void midiPitchBend (double v)
    {
        const double b = std::max (-1.0, std::min (1.0, v)) * bendRange;
        if (std::abs (b - bend) < 1e-12) return;
        bend = b; retune();
    }
    // rueda de modulación 0..1 → profundidad del vibrato por software
    void midiModWheel (double v) { mod = std::max (0.0, std::min (1.0, v)); if (mod > 0 && ! vibWasOn) nextVib = now; }
    void resetControllers() { mod = 0; if (std::abs (bend) > 0) { bend = 0; retune(); } midiSustain (false); }

    bool isHeld (int n) const { return held[(size_t) (n & 127)].on; }

    // Genera n muestras; las escrituras (películas, vibrato) caen en su muestra exacta
    void render (float* L, float* R, int n)
    {
        int pos = 0;
        while (pos < n)
        {
            filmTick();
            vibTick();
            int chunk = n - pos;
            int64_t next = nextFilmEventSamples();          // muestras hasta el próximo evento
            if (mod > 0 || vibWasOn) next = std::min (next, nextVib - now);
            if (next < chunk) chunk = (int) std::max<int64_t> (1, next);
            chips[0].process (L + pos, R + pos, chunk);
            for (int k = 1; k < numCh / 8; ++k)   // los chips extra se suman (cada uno con su propia salida, como si fueran teclados)
                for (int done = 0; done < chunk; )
                {
                    const int m = std::min (chunk - done, (int) tmpL.size());
                    chips[(size_t) k].process (tmpL.data(), tmpR.data(), m);
                    for (int i = 0; i < m; ++i) { L[pos + done + i] += tmpL[(size_t) i]; R[pos + done + i] += tmpR[(size_t) i]; }
                    done += m;
                }
            advance (chunk);
            pos += chunk;
        }
    }

    static constexpr int MAXCHIPS = 8, MAXCH = MAXCHIPS * 8;
    std::array<Chip, MAXCHIPS> chips;
    Chip& chip = chips[0];                                // el chip "de verdad" (los demás solo con Sin límite)
    void (*writeHook) (void*, int, int) = nullptr;   // para pruebas: ver cada escritura al bus
    void* hookCtx = nullptr;

private:
    struct ChanState
    {
        int note = -1; uint32_t t = 0; int loaded = 0; int part = 0; int fid = 0;
        int vstamp = -1;                 // stamp del sonido con el que se pulsó
        double semis = 0;                // nota − 60
        int base[2][2] {};               // [operador][g5, g6] sin transponer (sonido o película)
        bool op0Freq = false;            // el operador 0 lleva frecuencia propia
        int64_t onAt = 0;                // muestra de la pulsación (fase del vibrato)
        int velOff = 0, chipOff = 0;     // atenuación por velocidad (pasos de 0,1875 dB) pedida / escrita
    };
    struct Held { bool on = false; int used[2] {}; int nUsed = 0; };
    struct Film
    {
        bool live = false, released = false;
        int note = 0, id = 0, nUsed = 0, used[2] {};
        const Voice* v = nullptr;
        int64_t el = 0, relEl = 0;           // muestras desde la pulsación / desde que se soltó
        double off = 0;                      // ms que se ha corrido la película al repetir el final
        uint32_t i = 0;
        int fs[2][2][2] {};                  // [parte][operador][g5, g6] tal como los escribe el teclado
    };

    static constexpr double LOOP_FROM = 6000.0;   // si la nota dura más que lo grabado, se repite desde aquí
    static constexpr double VIB_RATE = 5.5, VIB_CENTS = 50.0, VIB_TICK_MS = 12.0;   // vibrato por software del emulador

    double sr = 48000.0;
    int voice = 0, stamp = 1, fidSeq = 0;
    uint32_t clock = 0;
    bool filmsOn = true;
    std::array<int, 256 * MAXCHIPS> shadow {};
    std::array<ChanState, MAXCH> chans {};
    int numCh = 8;
    std::vector<float> tmpL, tmpR;
    std::array<Held, 128> held {};
    std::array<Film, 64> films {};
    escoita::Sostido sost;
    std::array<Patch, NUM_VOICES> user {};
    Patch cur {};
    double bend = 0, bendRange = 2, mod = 0, velSens = 0;
    int64_t now = 0, nextVib = 0;
    bool vibWasOn = false;

    // escritura al bus del chip k
    void bus (int k, int a, int d)
    {
        shadow[(size_t) (k * 256 + a)] = d;
        int w = d;
        if ((a >> 4) == 2 && ((a >> 1) & 1))   // nivel de la portadora: más la atenuación por velocidad
        {
            auto& ch = chans[(size_t) (k * 8 + decCh (a))];
            w = std::min (255, d + ch.velOff); ch.chipOff = ch.velOff;
        }
        chips[(size_t) k].write (a, w);
        if (k == 0 && writeHook) writeHook (hookCtx, a, w);
    }
    // por canal global C (0..63): chip C/8, canal C%8
    void wr (int C, int g, int op, int d) { bus (C >> 3, adr (g, C & 7, op), d); }
    void wrCh (int C, int d) { bus (C >> 3, 0x70 + (C & 7), d); }
    int  raw (int C, int g, int op) const { return shadow[(size_t) ((C >> 3) * 256 + adr (g, C & 7, op))]; }
    int  rd (int C, int g, int op) const { const int v = raw (C, g, op); return v < 0 ? 0 : v; }

    static double jsRound (double x) { return std::floor (x + 0.5); }

    static void freqRegs (int d5, int d6, double semis, int& o5, int& o6)
    {
        int block = d5 >> 5;
        double f = (double) (((d5 & 31) << 4) | (d6 >> 4)) * std::pow (2.0, semis / 12.0);
        while (f >= 512 && block < 7) { f /= 2; ++block; }
        while (f < 256 && block > 0) { f *= 2; --block; }
        const int fi = (int) std::max (0.0, std::min (511.0, jsRound (f)));
        o5 = (block << 5) | (fi >> 4); o6 = (fi & 15) << 4;
    }

    // desplazamiento de altura del canal ahora mismo: bend + vibrato de la rueda
    double pitchOffset (const ChanState& ch) const
    {
        double s = bend;
        if (mod > 0)
        {
            const double t = (double) (now - ch.onAt) / sr;
            s += mod * VIB_CENTS / 100.0 * std::sin (2 * 3.14159265358979323846 * VIB_RATE * t);
        }
        return s;
    }

    // escribe la frecuencia de un operador; 'onlyIfChanged' para no repetir escrituras al chip
    void writeFreq (int c, int op, bool onlyIfChanged)
    {
        auto& ch = chans[(size_t) c];
        int a5, a6; freqRegs (ch.base[op][0], ch.base[op][1], ch.semis + pitchOffset (ch), a5, a6);
        if (! onlyIfChanged || raw (c, 5, op) != a5) wr (c, 5, op, a5);
        if (! onlyIfChanged || raw (c, 6, op) != a6) wr (c, 6, op, a6);
    }

    void retune()
    {
        for (int c = 0; c < numCh; ++c)
        {
            auto& ch = chans[(size_t) c];
            if (! ((chips[(size_t) (c >> 3)].activeMask() >> (c & 7)) & 1) || ch.vstamp < 0) continue;
            writeFreq (c, 1, true);
            if (ch.op0Freq) writeFreq (c, 0, true);
        }
    }

    void vibTick()
    {
        if (mod <= 0 && ! vibWasOn) return;
        if (now < nextVib) return;
        retune();                                   // con la rueda a 0, una última vez para volver a la altura exacta
        vibWasOn = mod > 0;
        nextVib = now + std::max<int64_t> (1, (int64_t) std::llround (VIB_TICK_MS * sr / 1000.0));
    }

    // Sonido de la parte pi, con lo que la película fija antes de pulsar
    void partRegs (const Voice& v, int pi, bool useInit, int out[16], int& g7) const
    {
        const Part& p = cur[(size_t) pi];
        for (int k = 0; k < 16; ++k) out[k] = p.r[k];
        g7 = p.g7;
        if (useInit)
        {
            for (int k = 2; k <= 13; ++k) if (v.init[pi][k] >= 0) out[k] = v.init[pi][k];
            if (v.init[pi][14] >= 0) g7 = v.init[pi][14];
        }
    }

    void loadPart (int c, const int p[16], int g7)
    {
        for (int k = 2; k <= 13; ++k)
        {
            const int g = k >> 1, op = k & 1;
            if (g == 1 && op == 1) continue;   // tecla: se escribe al tocar
            if (g == 5 || g == 6) continue;    // frecuencia: al tocar
            wr (c, g, op, p[k]);
        }
        wrCh (c, g7);
    }

    // el editor cambió un registro: aplicarlo ya a los canales cargados con este sonido (como liveWrite de la web)
    void liveWrite (int pi, int k)
    {
        const Part& p = cur[(size_t) pi];
        for (int c = 0; c < numCh; ++c)
        {
            auto& ch = chans[(size_t) c];
            if (ch.part != pi || (ch.loaded != stamp && ch.vstamp != stamp)) continue;
            if (k == 16) { wrCh (c, p.g7); continue; }
            const int g = k >> 1, op = k & 1;
            if (g == 5 || g == 6) continue;
            int d = p.r[k];
            if (g == 1 && op == 1) d = (d & 0x7F) | (rd (c, 1, 1) & 0x80);
            wr (c, g, op, d);
        }
    }

    int velocityOffset (int vel) const
    {
        if (velSens <= 0) return 0;
        const double db = std::min (36.0, -40.0 * std::log10 (std::max (1, vel) / 127.0)) * velSens;
        return (int) jsRound (db / chip.dbStep);
    }

    void noteOn (int n, int vel)
    {
        if (held[(size_t) n].on) return;
        const Voice& v = VOICES[voice];
        const bool film = filmsOn && v.evN > 0;
        const bool useInit = filmsOn;
        Held h; h.on = true;
        int regs[2][16], g7s[2];
        const int velOff = velocityOffset (vel);
        for (int pi = 0; pi < v.nParts; ++pi)
        {
            partRegs (v, pi, useInit, regs[pi], g7s[pi]);
            const int* p = regs[pi]; const int g7 = g7s[pi];
            // a canle: a libre que leva máis tempo libre, se non a que leva máis tempo tocando
            // (o reparto do núcleo común); nunca unha que xa use esta nota
            const int c = escoita::escollerVoz (numCh,
                [this] (int k) { const auto& ch = chans[(size_t) k]; return escoita::EstadoVoz { ch.note < 0, ch.t }; },
                [&h] (int k) { for (int u = 0; u < h.nUsed; ++u) if (h.used[u] == k) return true; return false; });
            h.used[h.nUsed++] = c;
            auto& ch = chans[(size_t) c];
            if (ch.note >= 0) wr (c, 1, 1, rd (c, 1, 1) & 0x7F);
            ch.velOff = velOff;
            if (ch.loaded != stamp || ch.part != pi) { loadPart (c, p, g7); ch.loaded = stamp; ch.part = pi; }
            else if (ch.chipOff != ch.velOff) wr (c, 2, 1, rd (c, 2, 1));
            wr (c, 4, 0, p[8]); wr (c, 4, 1, p[9]);
            ch.semis = n - 60; ch.onAt = now; ch.vstamp = stamp;
            ch.base[1][0] = p[11]; ch.base[1][1] = p[13];
            ch.base[0][0] = p[10]; ch.base[0][1] = p[12];
            ch.op0Freq = ! (g7 >> 7);
            writeFreq (c, 1, false);
            if (ch.op0Freq) writeFreq (c, 0, false);
            wr (c, 1, 1, p[3] | 0x80);
            ch.note = n; ch.t = ++clock; ch.fid = 0;
        }
        held[(size_t) n] = h;
        if (film) filmStart (n, h, regs);
    }

    void noteOff (int n)
    {
        Held& h = held[(size_t) n];
        if (! h.on) return;
        h.on = false;
        for (auto& f : films) if (f.live && ! f.released && f.note == n) { f.released = true; f.relEl = 0; }
        const Voice& v = VOICES[voice];
        for (int u = 0; u < h.nUsed; ++u)
        {
            const int c = h.used[u];
            auto& ch = chans[(size_t) c];
            if (ch.note != n) continue;
            wr (c, 1, 1, rd (c, 1, 1) & 0x7F);
            // caída al soltar: cada sonido escribe la suya (grabado del teclado real)
            const int pi = ch.part < v.nParts ? ch.part : 0;
            if (v.rel[pi][0] >= 0) wr (c, 4, 0, v.rel[pi][0]);
            if (v.rel[pi][1] >= 0) wr (c, 4, 1, v.rel[pi][1]);
            ch.note = -1; ch.t = ++clock;
        }
    }

    // ---- películas ----
    void filmStart (int n, const Held& h, const int regs[2][16])
    {
        const int id = ++fidSeq;
        for (int u = 0; u < h.nUsed; ++u) { chans[(size_t) h.used[u]].loaded = -1; chans[(size_t) h.used[u]].fid = id; }
        // hueco: uno libre, o el más antiguo
        Film* slot = nullptr;
        for (auto& f : films) if (! f.live) { slot = &f; break; }
        if (! slot) { slot = &films[0]; for (auto& f : films) if (f.id < slot->id) slot = &f; }
        Film& F = *slot;
        F = Film{};
        F.live = true; F.note = n; F.id = id; F.v = &VOICES[voice]; F.nUsed = h.nUsed;
        for (int u = 0; u < h.nUsed; ++u) F.used[u] = h.used[u];
        for (int pi = 0; pi < 2; ++pi)
        {
            F.fs[pi][0][0] = regs[pi][10]; F.fs[pi][0][1] = regs[pi][12];
            F.fs[pi][1][0] = regs[pi][11]; F.fs[pi][1][1] = regs[pi][13];
        }
    }

    bool filmOwns (const Film& F) const
    {
        for (int u = 0; u < F.nUsed; ++u) if (chans[(size_t) F.used[u]].fid == F.id) return true;
        return false;
    }

    void filmApply (Film& F, const FilmEv& e)
    {
        const int pi = e.part, k = e.k, d = e.d;
        if (pi >= F.nUsed) return;
        const int c = F.used[pi];
        auto& ch = chans[(size_t) c];
        if (ch.fid != F.id) return;
        if (k == 14) { wrCh (c, d); return; }
        const int g = k >> 1, op = k & 1;
        if (F.released && ((g == 1 && op == 1) || g == 3 || g == 4)) return;   // ya soltada: ni re-pulsar ni tocar la caída
        if (g == 5 || g == 6)
        {
            F.fs[pi][op][g - 5] = d;
            ch.base[op][0] = F.fs[pi][op][0]; ch.base[op][1] = F.fs[pi][op][1];
            if (op == 0) ch.op0Freq = true;
            writeFreq (c, op, false);
            return;
        }
        wr (c, g, op, d);
    }

    void filmTick()
    {
        for (auto& F : films)
        {
            if (! F.live) continue;
            // fuera si ya no le queda ningún canal suyo, o si se soltó hace más de 4 s
            if (! filmOwns (F) || (F.released && F.relEl > msToSamples (4000.0))) { F.live = false; continue; }
            const Voice& v = *F.v;
            const FilmEv* ev = FILM_EVENTS + v.ev0;
            while (F.i < v.evN && msToSamples (ev[F.i].t + F.off) <= F.el) filmApply (F, ev[F.i++]);
            if (F.i >= v.evN && v.end > 11000 && F.el >= msToSamples (v.end + F.off))
            {
                F.off += v.end - LOOP_FROM;
                uint32_t j = 0; while (j < v.evN && ev[j].t < LOOP_FROM) ++j;
                F.i = j;
                while (F.i < v.evN && msToSamples (ev[F.i].t + F.off) <= F.el) filmApply (F, ev[F.i++]);
            }
        }
    }

    // la muestra (contada desde la pulsación) en la que cae un instante de la película
    int64_t msToSamples (double ms) const { return (int64_t) std::ceil (ms * sr / 1000.0 - 1e-6); }

    int64_t nextFilmEventSamples() const
    {
        int64_t best = INT64_MAX;
        for (auto& F : films)
        {
            if (! F.live) continue;
            const Voice& v = *F.v;
            int64_t t = INT64_MAX;
            if (F.i < v.evN) t = msToSamples (FILM_EVENTS[v.ev0 + F.i].t + F.off) - F.el;
            else if (v.end > 11000) t = msToSamples (v.end + F.off) - F.el;
            if (F.released) t = std::min (t, msToSamples (4000.0) + 1 - F.relEl);
            best = std::min (best, std::max<int64_t> (0, t));
        }
        return best;
    }

    void advance (int samples)
    {
        now += samples;
        for (auto& F : films) if (F.live) { F.el += samples; if (F.released) F.relEl += samples; }
    }
};

} // namespace ym
