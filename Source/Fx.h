// Fx.h — Cadena de salida del emulador web, sin latencia:
// filtro de salida (2 × paso bajo a 9 kHz), ecualizador "Altavoz del PSS",
// compensación de volumen, saturación de altavoz opcional y limitador sin anticipación.
// Los biquads usan exactamente las fórmulas de la especificación Web Audio
// (en paso bajo/alto la Q va en dB; en los shelves S = 1), así suena igual que en la web.
#pragma once
#include <cmath>
#include <algorithm>
#include <escoita_nucleo/escoita_nucleo.h>   // o limitador de seguridade (escoita::Teito), do núcleo común

namespace ym {

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[2] {}, z2[2] {};

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    inline double run (double x, int ch)
    {
        const double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }

    static constexpr double PI = 3.14159265358979323846;
    void lowpass (double fs, double f, double qDb)
    {
        const double w = 2 * PI * std::min (f, fs / 2) / fs, c = std::cos (w), al = std::sin (w) / (2 * std::pow (10.0, qDb / 20));
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highpass (double fs, double f, double qDb)
    {
        const double w = 2 * PI * std::min (f, fs / 2) / fs, c = std::cos (w), al = std::sin (w) / (2 * std::pow (10.0, qDb / 20));
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void peaking (double fs, double f, double q, double g)
    {
        const double w = 2 * PI * std::min (f, fs / 2) / fs, c = std::cos (w), al = std::sin (w) / (2 * q), A = std::pow (10.0, g / 40);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowshelf (double fs, double f, double g)
    {
        const double w = 2 * PI * std::min (f, fs / 2) / fs, c = std::cos (w), A = std::pow (10.0, g / 40);
        const double al = std::sin (w) / 2 * std::sqrt (2.0), k = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + k), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - k),
             (A + 1) + (A - 1) * c + k, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - k);
    }
    void highshelf (double fs, double f, double g)
    {
        const double w = 2 * PI * std::min (f, fs / 2) / fs, c = std::cos (w), A = std::pow (10.0, g / 40);
        const double al = std::sin (w) / 2 * std::sqrt (2.0), k = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + k), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - k),
             (A + 1) - (A - 1) * c + k, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - k);
    }
};

class OutputChain
{
public:
    // Ecualizador "Altavoz del PSS" (el ajuste del usuario) — por defecto
    struct Eq { double hpf = 310, lowF = 270, low = -15, midF = 200, mid = 13, midQ = 0.3, highF = 11200, high = -10; };

    bool   eqOn       = true;
    bool   speakerOn  = false;   // saturación de altavoz pequeño
    double speakerDrive = 40;
    double volumeDb   = 0;
    Eq     eq;

    void prepare (double sampleRate) { fs = sampleRate; teito.preparar (fs, 0.080); update(); reset(); }

    void reset()
    {
        for (auto* b : { &out1, &out2, &eqH, &eqL, &eqM, &eqT, &spHp, &spPk, &spLp }) b->reset();
        env = 0; teito.reiniciar(); volS = volT; makeS = makeT;
    }

    // Recalcula coeficientes; llamar al cambiar ajustes (no en cada muestra)
    void update()
    {
        out1.lowpass (fs, std::min (9000.0, fs / 2 - 100), 0.54);
        out2.lowpass (fs, std::min (9000.0, fs / 2 - 100), 1.31);
        eqH.highpass (fs, eqOn ? eq.hpf : 10, 0.71);
        eqL.lowshelf (fs, eq.lowF, eqOn ? eq.low : 0);
        eqM.peaking  (fs, eq.midF, eq.midQ, eqOn ? eq.mid : 0);
        eqT.highshelf (fs, eq.highF, eqOn ? eq.high : 0);
        double peak = 0;
        if (eqOn) for (double f = 40; f < 16000; f *= 1.05) peak = std::max (peak, eqDb (f));
        makeT = std::pow (10.0, (2 - peak) / 20);   // +2 dB de base menos lo que suba el EQ: caben 8 notas sin saturar
        spHp.highpass (fs, 280, 0.9);
        spPk.peaking  (fs, 1600, 1.1, 6);
        spLp.lowpass  (fs, 4800, 0.7);
        spPre = 1 + speakerDrive / 12; spK = 1 + speakerDrive / 25; spNorm = 1 / std::tanh (spK);
        // compensación: al encenderla, el volumen queda parecido (medido con piano, órgano, sitar y cuerdas: ±3 dB)
        spComp = 1.7 / (spPre * spK * spNorm);
        volT = std::pow (10.0, volumeDb / 20);
        atk = std::exp (-1 / (0.002 * fs)); rel = std::exp (-1 / (0.150 * fs));
        smooth = 1 - std::exp (-1 / (0.02 * fs));
    }

    // Respuesta aproximada del EQ (la misma que dibuja la web, y con la que calcula la compensación)
    double eqDb (double f) const
    {
        if (! eqOn) return 0;
        double db = 0;
        const double r = f / eq.hpf; db += -10 * std::log10 (1 + std::pow (1 / r, 4));
        db += eq.low / (1 + std::pow (f / eq.lowF, 2));
        db += eq.high / (1 + std::pow (eq.highF / f, 2));
        const double x = std::log2 (f / eq.midF) * eq.midQ * 1.4; db += eq.mid / (1 + x * x);
        return db;
    }

    void process (float* L, float* R, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            makeS += (makeT - makeS) * smooth; volS += (volT - volS) * smooth;
            double l = L[i], r = R[i];
            l = out2.run (out1.run (l, 0), 0); r = out2.run (out1.run (r, 1), 1);
            l = eqT.run (eqM.run (eqL.run (eqH.run (l, 0), 0), 0), 0);
            r = eqT.run (eqM.run (eqL.run (eqH.run (r, 1), 1), 1), 1);
            l *= makeS; r *= makeS;
            if (speakerOn)
            {
                double m = 0.5 * (l + r);
                m = spLp.run (spPk.run (spHp.run (m, 0), 0), 0) * spPre;
                m = std::tanh (m * spK) * spNorm * spComp;   // curva suave, sin recorte duro
                l = r = m;
            }
            l *= volS; r *= volS;
            // limitador: umbral −3 dB, rodilla 3 dB, 20:1, ataque 2 ms, relajación 150 ms, sin anticipación
            const double pk = std::max (std::abs (l), std::abs (r));
            const double coef = pk > env ? atk : rel;
            env = pk + (env - pk) * coef;
            const double g = gainFor (env) * MAKEUP;
            l *= g; r *= g;
            // limitador de seguridade (do núcleo): teito a −1 dBFS, ataque instantáneo, solta en 80 ms, sen anticipación
            teito.procesar (l, r);
            l = softClip (l); r = softClip (r);
            L[i] = (float) l; R[i] = (float) r;
        }
    }

private:
    static constexpr double MAKEUP = 1.197;   // la compensación automática que aplica el compresor del navegador con estos ajustes
    double fs = 48000;
    Biquad out1, out2, eqH, eqL, eqM, eqT, spHp, spPk, spLp;
    double makeT = 1, makeS = 1, volT = 1, volS = 1, smooth = 0.001;
    double spPre = 1, spK = 1, spNorm = 1, spComp = 1;
    escoita::Teito teito;
    double env = 0, atk = 0, rel = 0;

    static double gainFor (double x)
    {
        if (x < 1e-9) return 1;
        const double inDb = 20 * std::log10 (x), T = -3, W = 3, Rr = 20;
        double outDb;
        if (2 * (inDb - T) < -W) outDb = inDb;
        else if (2 * std::abs (inDb - T) <= W) { const double d = inDb - T + W / 2; outDb = inDb + (1 / Rr - 1) * d * d / (2 * W); }
        else outDb = T + (inDb - T) / Rr;
        return std::pow (10.0, (outDb - inDb) / 20);
    }
    static double softClip (double x)
    {
        const double a = std::abs (x);
        if (a <= 0.95) return x;
        const double y = 0.95 + 0.05 * std::tanh ((a - 0.95) / 0.05);
        return x < 0 ? -y : y;
    }
};

} // namespace ym
