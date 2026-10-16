// Chip.h — Emulación del YM7129 (familia OPLL de Yamaha), sin dependencias.
// Traducción directa del motor "familia OPLL" del emulador web, calibrado con
// la grabación del chip real: profundidad FM 16, nivel de 8 bits a 0,1875 dB,
// bit 5 del grupo 1 = media onda, chip a reloj/64 (8 canales × 2 operadores).
#pragma once
#include <cstdint>
#include <cmath>
#include <array>
#include <algorithm>
#include "Resampler.h"

namespace ym {

inline int decCh (int a) { return (a & 1) | ((a >> 2) & 1) << 1 | ((a >> 3) & 1) << 2; }
inline int adr (int g, int c, int op) { return (g << 4) | (c & 1) | ((c >> 1) & 1) << 2 | ((c >> 2) & 1) << 3 | (op << 1); }

class Chip
{
public:
    static constexpr double clockHz = 3579545.0;
    static constexpr double chipRate = clockHz / 64.0;   // 55 930 Hz

    // Ajustes (los mismos que el panel de calibración del emulador)
    int    grain   = 1;        // 0 limpio, 1 tablas del chip (12 bits), 2 salida cruda de 9 bits
    double depth   = 16.0;     // profundidad FM (16 = medido)
    double dbStep  = 0.1875;   // dB por paso de nivel (medido)
    double gain    = 16.0;     // volumen interno
    int    interp  = 1;        // 0 = interpolación lineal (la del emulador web), 1 = filtro sin aliasing

    explicit Chip (double sampleRate = 48000.0) { initTables(); setSampleRate (sampleRate); reset(); }

    void setSampleRate (double sr)
    {
        if (std::abs (sr - designedFor) < 1e-9) { outRate = sr; return; }
        outRate = designedFor = sr;
        rs.design (chipRate, sr);
    }

    void reset()
    {
        regs.fill (0);
        for (auto& ch : chans) { for (auto& o : ch.op) o = Op{}; ch.q1 = ch.q2 = 0; ch.q1f = ch.q2f = 0; }
        acc = 0; prevL = prevR = curL = curR = 0; egc = 0; silentRun = 0;
        rs.reset();
        paramsDirty = true;
    }

    void write (int a, int d)
    {
        a &= 0xFF; d &= 0xFF;
        const int g = a >> 4;
        if (g == 7) { regs[(size_t) a] = (uint8_t) d; paramsDirty = true; return; }
        if (g < 1 || g > 6) return;
        const int c = decCh (a), op = (a >> 1) & 1, old = regs[(size_t) a];
        regs[(size_t) a] = (uint8_t) d;
        paramsDirty = true;
        if (g == 1 && op == 1)
        {
            auto& ch = chans[(size_t) c];
            const int was = old >> 7, now = d >> 7;
            if (now && ! was) { for (auto& o : ch.op) { o.st = 1; o.p = 0; } ch.q1 = ch.q2 = 0; ch.q1f = ch.q2f = 0; }
            else if (! now && was) { for (auto& o : ch.op) if (o.st) o.st = 4; }
        }
    }

    uint8_t reg (int a) const { return regs[(size_t) (a & 0xFF)]; }

    int activeMask() const   // bit c = el canal c está sonando
    {
        int m = 0;
        for (int c = 0; c < 8; ++c) if (chans[(size_t) c].op[0].st || chans[(size_t) c].op[1].st) m |= 1 << c;
        return m;
    }

    bool anyActive() const
    {
        for (auto& ch : chans) if (ch.op[0].st || ch.op[1].st) return true;
        return false;
    }

    // Genera n muestras estéreo a la frecuencia del anfitrión (remuestreo lineal desde 55 930 Hz)
    void process (float* L, float* R, int n)
    {
        if (paramsDirty) computeParams();
        const double stepRatio = chipRate / outRate, g = gain * 0.02;
        if (! anyActive() && silentRun > rs.numTaps() + 2)
        {
            // callado y con el filtro vacío: la salida es cero; solo avanzan los relojes internos
            for (int i = 0; i < n; ++i)
            {
                acc += stepRatio;
                while (acc >= 1.0) { acc -= 1.0; egc = (egc + 1) & 0x3FFFFFFF; prevL = prevR = curL = curR = 0; }
                L[i] = 0.0f; R[i] = 0.0f;
            }
            return;
        }
        for (int i = 0; i < n; ++i)
        {
            acc += stepRatio;
            while (acc >= 1.0)
            {
                acc -= 1.0;
                prevL = curL; prevR = curR;
                chipSample (curL, curR);
                rs.push (curL, curR);
                if (std::abs (curL) > 0 || std::abs (curR) > 0) silentRun = 0; else ++silentRun;
            }
            const double f = acc;
            double l, r;
            if (interp == 0) { l = prevL + (curL - prevL) * f; r = prevR + (curR - prevR) * f; }
            else rs.out (f, l, r);
            L[i] = (float) std::tanh (l * g);
            R[i] = (float) std::tanh (r * g);
        }
    }

private:
    struct Op { int st = 0; int p = 0; int env = 511; };
    struct Chan { std::array<Op, 2> op; int q1 = 0, q2 = 0; double q1f = 0, q2f = 0; };
    struct OpP { int step = 0, half = 0, tl = 0, ar = 0, dr = 0, rr = 0, sl = 0; };
    struct ChP { std::array<OpP, 2> o; int fb = 0, l = 0, r = 0; bool active = false; };

    std::array<uint8_t, 256> regs {};
    std::array<Chan, 8> chans {};
    std::array<ChP, 8> P {};
    bool paramsDirty = true;
    Resampler rs;
    double designedFor = -1;
    double outRate = 48000.0, acc = 0, prevL = 0, prevR = 0, curL = 0, curR = 0;
    uint32_t egc = 0;
    int silentRun = 0;   // muestras seguidas del chip a cero

    static inline std::array<uint16_t, 256> LOGSIN {}, EXPT {};
    static inline bool tablesReady = false;
    static constexpr int MULX2[16] = { 1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30 };
    static constexpr int PAT_LO[4][8] = { {0,1,0,1,0,1,0,1}, {0,1,0,1,1,1,0,1}, {0,1,1,1,0,1,1,1}, {0,1,1,1,1,1,1,1} };
    static constexpr int PAT_HI[4][8] = { {1,1,1,1,1,1,1,1}, {1,1,1,2,1,1,1,2}, {1,2,1,2,1,2,1,2}, {1,2,2,2,1,2,2,2} };

    static double jsRound (double x) { return std::floor (x + 0.5); }   // Math.round de JavaScript

    static void initTables()
    {
        if (tablesReady) return;
        const double PI = 3.14159265358979323846;
        for (int i = 0; i < 256; ++i)
        {
            LOGSIN[(size_t) i] = (uint16_t) jsRound (-std::log2 (std::sin ((i + 0.5) * PI / 512.0)) * 256.0);
            EXPT[(size_t) i]   = (uint16_t) jsRound ((std::pow (2.0, i / 256.0) - 1.0) * 1024.0);
        }
        tablesReady = true;
    }

    static int egInc (int rate, uint32_t c)
    {
        if (rate < 4) return 0;
        const int sh = rate >> 2;
        if (sh < 12)
        {
            const uint32_t per = 1u << (12 - sh);
            if (c & (per - 1)) return 0;
            return PAT_LO[rate & 3][(c >> (12 - sh)) & 7];
        }
        if (rate >= 60) return 8;
        return PAT_HI[rate & 3][c & 7] << (sh - 12);
    }

    static int opOut (int ph10, int att)
    {
        int i = ph10 & 255;
        if (ph10 & 256) i = 255 - i;
        const int v = LOGSIN[(size_t) i] + att;
        if (v >= 4096) return 0;
        const int o = (EXPT[(size_t) ((~v) & 255)] + 1024) >> (v >> 8);
        return (ph10 & 512) ? -o : o;
    }

    static double opClean (double ph10f, int att)
    {
        return std::sin (ph10f * (3.14159265358979323846 / 512.0)) * 2047.0 * std::pow (2.0, -att / 256.0);
    }

    OpP paramsOp (int c, int o) const
    {
        const int g1 = regs[(size_t) adr (1, c, o)], g2 = regs[(size_t) adr (2, c, o)];
        const int g3 = regs[(size_t) adr (3, c, o)], g4 = regs[(size_t) adr (4, c, o)];
        const int fo = (o == 0 && (regs[(size_t) (0x70 + c)] >> 7)) ? 1 : o;
        const int f5 = regs[(size_t) adr (5, c, fo)], f6 = regs[(size_t) adr (6, c, fo)];
        const int block = f5 >> 5, fnum = ((f5 & 31) << 4) | (f6 >> 4);
        const int rks = (g1 & 16) ? ((block << 1) | (fnum >> 8)) : (block >> 1);
        auto rate = [rks] (int Rv) { return Rv ? std::min (63, Rv * 4 + rks) : 0; };
        const int slN = g4 >> 4;
        OpP p;
        p.step = ((fnum << block) * MULX2[g1 & 15]) >> 1;
        p.half = (g1 >> 5) & 1;
        p.tl   = (int) jsRound (g2 * dbStep / 6.0206 * 256.0);
        p.ar = rate (g3 >> 4); p.dr = rate (g3 & 15); p.rr = rate (g4 & 15);
        p.sl = slN == 15 ? 496 : slN << 4;
        return p;
    }

    void computeParams()
    {
        for (int c = 0; c < 8; ++c)
        {
            auto& ch = chans[(size_t) c];
            auto& e = P[(size_t) c];
            e.active = ch.op[0].st || ch.op[1].st;
            if (! e.active) continue;
            const int r70 = regs[(size_t) (0x70 + c)];
            e.o[0] = paramsOp (c, 0); e.o[1] = paramsOp (c, 1);
            e.fb = r70 & 7; e.l = (r70 >> 6) & 1; e.r = (r70 >> 5) & 1;
        }
        paramsDirty = false;
    }

    void env (Op& o, const OpP& p) const
    {
        switch (o.st)
        {
            case 1:
            {
                if (p.ar >= 62) { o.env = 0; o.st = 2; break; }
                const int inc = egInc (p.ar, egc);
                if (inc) o.env += ((~o.env) * inc) >> 4;
                if (o.env <= 0) { o.env = 0; o.st = 2; }
                break;
            }
            case 2: o.env += egInc (p.dr, egc); if (o.env >= p.sl) { o.env = p.sl; o.st = 3; } break;
            case 3: o.env = std::min (511, o.env + egInc (p.rr, egc)); break;
            case 4: o.env += egInc (p.rr, egc); if (o.env >= 511) { o.env = 511; o.st = 0; } break;
            default: break;
        }
    }

    void chipSample (double& outL, double& outR)
    {
        egc = (egc + 1) & 0x3FFFFFFF;
        int sl = 0, sr = 0; double slf = 0, srf = 0;
        const double ds = depth / 8.0;
        for (int c = 0; c < 8; ++c)
        {
            auto& ch = chans[(size_t) c];
            auto& o0 = ch.op[0]; auto& o1 = ch.op[1];
            if (! o0.st && ! o1.st) continue;
            const auto& e = P[(size_t) c];
            if (! e.active) continue;          // se activó dentro de este bloque: entra en el siguiente
            const auto& p0 = e.o[0]; const auto& p1 = e.o[1];
            env (o0, p0); env (o1, p1);
            if (grain == 0)
            {
                const double fb = e.fb ? (ch.q1f + ch.q2f) / (double) (1 << (8 - e.fb)) : 0.0;
                const double ph0 = o0.p / 512.0 + fb;
                double m = opClean (ph0, p0.tl + (o0.env << 3));
                if (p0.half && std::fmod (std::fmod (ph0, 1024.0) + 1024.0, 1024.0) >= 512.0) m = 0;
                ch.q2f = ch.q1f; ch.q1f = m;
                const double ph1 = o1.p / 512.0 + m * ds;
                double out = opClean (ph1, p1.tl + (o1.env << 3));
                if (p1.half && std::fmod (std::fmod (ph1, 1024.0) + 1024.0, 1024.0) >= 512.0) out = 0;
                if (e.l) slf += out;
                if (e.r) srf += out;
            }
            else
            {
                const int fb = e.fb ? (ch.q1 + ch.q2) >> (8 - e.fb) : 0;
                const int ph0 = ((o0.p >> 9) + fb) & 1023;
                const int m = (p0.half && (ph0 & 512)) ? 0 : opOut (ph0, p0.tl + (o0.env << 3));
                ch.q2 = ch.q1; ch.q1 = m;
                const int ph1 = ((o1.p >> 9) + (int) jsRound (m * ds)) & 1023;
                int out = (p1.half && (ph1 & 512)) ? 0 : opOut (ph1, p1.tl + (o1.env << 3));
                if (grain == 2) out = (out >> 3) << 3;
                if (e.l) sl += out;
                if (e.r) sr += out;
            }
            o0.p = (o0.p + p0.step) & 0x7FFFF;
            o1.p = (o1.p + p1.step) & 0x7FFFF;
            if (o1.st == 0 || (o1.st == 4 && o1.env >= 510)) { o0.st = 0; o1.st = 0; }
        }
        outL = (sl + slf) / 2048.0;
        outR = (sr + srf) / 2048.0;
    }
};

} // namespace ym
