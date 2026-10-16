// Resampler.h — Paso de la frecuencia del chip (55 930 Hz) a la del DAW sin aliasing y sin latencia.
//
// La interpolación lineal (la del emulador web) dobla hacia abajo las imágenes de lo que el chip
// tiene entre 8 y 28 kHz: en sonidos brillantes ese ruido queda solo 23–39 dB por debajo de la nota.
// Aquí se usa un filtro paso bajo (Kaiser, 90 dB) convolucionado con el triángulo de la
// interpolación lineal, de modo que el tono es el mismo que en la web (y el ecualizador "Altavoz"
// sigue valiendo), pero el aliasing baja a unos −89 dB. El filtro se pasa a fase mínima (cepstro):
// solo usa muestras pasadas, así que no hace falta mirar el futuro; el retraso es ~0,07 ms.
#pragma once
#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>
#include <mutex>

namespace ym {

class Resampler
{
public:
    static constexpr int L = 256;   // fases por muestra del chip

    void design (double chipRate, double outRate)
    {
        // el diseño cuesta unos 40 ms: se guarda y lo reutilizan los demás chips
        static std::mutex m;
        static double cRate = -1, oRate = -1;
        static std::vector<float> cTable;
        static int cTaps = 0;
        {
            std::lock_guard<std::mutex> lk (m);
            if (std::abs (cRate - chipRate) < 1e-9 && std::abs (oRate - outRate) < 1e-9) { table = cTable; taps = cTaps; allocHistory(); return; }
        }
        designNow (chipRate, outRate);
        std::lock_guard<std::mutex> lk (m);
        cRate = chipRate; oRate = outRate; cTable = table; cTaps = taps;
    }

    void designNow (double chipRate, double outRate)
    {
        const double PI = 3.14159265358979323846;
        const double fpass = std::min (20000.0, 0.45 * std::min (chipRate, outRate));
        const double fstop = std::min (chipRate, outRate) - fpass;
        const double A = 90.0, beta = 0.1102 * (A - 8.7);
        const double dw = 2 * PI * (fstop - fpass) / chipRate;
        const int N = (int) std::ceil ((A - 8) / (2.285 * dw)) + 1;
        const double cut = (fpass + fstop) / 2 / chipRate;

        // prototipo de fase lineal, muy sobremuestreado (L puntos por muestra del chip)
        const int P = N * L;
        std::vector<double> h ((size_t) P);
        const double i0b = besselI0 (beta);
        for (int i = 0; i < P; ++i)
        {
            const double t = (i - (P - 1) / 2.0) / L;
            const double x = 2 * cut * t;
            const double sinc = std::abs (x) < 1e-12 ? 1.0 : std::sin (PI * x) / (PI * x);
            const double r = 2.0 * i / (P - 1) - 1.0;
            h[(size_t) i] = 2 * cut * sinc * besselI0 (beta * std::sqrt (std::max (0.0, 1 - r * r))) / i0b;
        }
        // por el triángulo de la interpolación lineal (ancho 2 muestras): mismo tono que la web
        std::vector<double> tri ((size_t) (2 * L + 1));
        for (int i = 0; i <= 2 * L; ++i) tri[(size_t) i] = (i <= L ? i : 2 * L - i) / (double) L;
        std::vector<double> g (h.size() + tri.size() - 1, 0.0);
        for (size_t i = 0; i < h.size(); ++i)
            for (size_t j = 0; j < tri.size(); ++j) g[i + j] += h[i] * tri[j];

        // a fase mínima por el cepstro real
        size_t M = 1; while (M < g.size() * 8) M <<= 1;
        std::vector<std::complex<double>> X (M);
        for (size_t i = 0; i < g.size(); ++i) X[i] = g[i];
        fft (X, false);
        for (auto& v : X) v = std::log (std::max (std::abs (v), 1e-9));
        fft (X, true);
        for (size_t i = 1; i < M / 2; ++i) X[i] *= 2.0;
        for (size_t i = M / 2 + 1; i < M; ++i) X[i] = 0.0;
        for (auto& v : X) v = std::complex<double> (v.real(), 0.0);
        fft (X, false);
        for (auto& v : X) v = std::exp (v);
        fft (X, true);

        taps = (int) (g.size() + L - 1) / L + 1;
        table.assign ((size_t) (taps * L + 2), 0.0f);
        for (size_t i = 0; i < g.size() && i < table.size(); ++i) table[i] = (float) X[i].real();

        // cada fase suma exactamente 1: ni rizado ni cambio de volumen
        for (int ph = 0; ph < L; ++ph)
        {
            double s = 0; for (int k = 0; k < taps; ++k) s += table[(size_t) (ph + k * L)];
            if (std::abs (s) > 1e-12) for (int k = 0; k < taps; ++k) table[(size_t) (ph + k * L)] = (float) (table[(size_t) (ph + k * L)] / s);
        }
        allocHistory();
    }

    void allocHistory()
    {
        size_t H = 1; while ((int) H < taps + 1) H <<= 1;
        histL.assign (H, 0.0); histR.assign (H, 0.0); mask = H - 1; head = 0;
    }

    void reset() { std::fill (histL.begin(), histL.end(), 0.0); std::fill (histR.begin(), histR.end(), 0.0); head = 0; }

    inline void push (double l, double r) { head = (head + 1) & mask; histL[head] = l; histR[head] = r; }

    // salida en el instante (última muestra del chip + frac), frac en [0, 1)
    inline void out (double frac, double& l, double& r) const
    {
        const double p = frac * L;
        const int ip = (int) p; const float a = (float) (p - ip);
        double sl = 0, sr = 0;
        size_t idx = head;
        const float* t = table.data() + ip;
        for (int k = 0; k < taps; ++k)
        {
            const float c = t[0] + (t[1] - t[0]) * a;
            sl += histL[idx] * c; sr += histR[idx] * c;
            idx = (idx - 1) & mask; t += L;
        }
        l = sl; r = sr;
    }

    int numTaps() const { return taps; }

private:
    std::vector<float> table;
    std::vector<double> histL, histR;
    size_t mask = 0, head = 0;
    int taps = 0;

    static double besselI0 (double x)
    {
        double s = 1, t = 1;
        for (int k = 1; k < 60; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; if (t < 1e-14 * s) break; }
        return s;
    }

    static void fft (std::vector<std::complex<double>>& a, bool inverse)
    {
        const size_t n = a.size();
        for (size_t i = 1, j = 0; i < n; ++i)
        {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap (a[i], a[j]);
        }
        const double PI = 3.14159265358979323846;
        for (size_t len = 2; len <= n; len <<= 1)
        {
            const double ang = 2 * PI / (double) len * (inverse ? 1 : -1);
            const std::complex<double> wl (std::cos (ang), std::sin (ang));
            for (size_t i = 0; i < n; i += len)
            {
                std::complex<double> w (1);
                for (size_t j = 0; j < len / 2; ++j)
                {
                    const auto u = a[i + j], v = a[i + j + len / 2] * w;
                    a[i + j] = u + v; a[i + j + len / 2] = u - v;
                    w *= wl;
                }
            }
        }
        if (inverse) for (auto& v : a) v /= (double) n;
    }
};

} // namespace ym
