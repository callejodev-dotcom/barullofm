// Toca una secuencia (seq.txt: t_ms tipo valor) con el firmware C++ y guarda escrituras y audio
#include "../Source/Synth.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
static FILE* wlog; static long curSample; static double SR = 48000;
static void hook (void*, int a, int d) { std::fprintf (wlog, "%.4f %d %d\n", curSample * 1000.0 / SR, a, d); }
int main (int argc, char** argv)
{
    struct E { double t; std::string ty; int a; }; std::vector<E> seq;
    FILE* f = std::fopen (argv[1], "r"); double t; char ty[16]; int a;
    while (std::fscanf (f, "%lf %15s %d", &t, ty, &a) == 3) seq.push_back ({ t, ty, a });
    static ym::Synth s; s.prepare (SR);
    wlog = std::fopen (argv[2], "w"); s.writeHook = hook;
    const long N = (long) ((seq.back().t + 10) / 1000 * SR);
    std::vector<float> L (N), R (N); size_t k = 0; long pos = 0;
    while (pos < N)
    {
        curSample = pos;
        while (k < seq.size() && seq[k].t / 1000 * SR <= pos)
        {
            auto& e = seq[k++];
            if (e.ty == "prog") s.setVoice (e.a);
            else if (e.ty == "on") s.midiNoteOn (e.a);
            else if (e.ty == "off") s.midiNoteOff (e.a);
            else if (e.ty == "cc64") s.midiSustain (e.a >= 64);
        }
        long next = k < seq.size() ? (long) std::ceil (seq[k].t / 1000 * SR) : N;
        next = std::max (pos + 1, std::min (N, next));
        // render en trozos pequeños para que el registro de tiempos sea fino
        while (pos < next) { int n = (int) std::min<long> (32, next - pos); curSample = pos; s.render (&L[(size_t) pos], &R[(size_t) pos], n); pos += n; }
    }
    std::fclose (wlog);
    if (argc > 3) { FILE* o = std::fopen (argv[3], "wb"); for (long i = 0; i < N; ++i) { std::fwrite (&L[(size_t) i], 4, 1, o); std::fwrite (&R[(size_t) i], 4, 1, o); } std::fclose (o); }
    std::printf ("ok %ld\n", N);
}
