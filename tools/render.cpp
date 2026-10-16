// Renderiza un log de escrituras (t_ms dir dato por línea) con el motor C++, igual que render_html.js
#include "../Source/Chip.h"
#include <cstdio>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
int main (int argc, char** argv)
{
    FILE* f = std::fopen (argv[1], "r");
    std::vector<double> T; std::vector<int> A, D; double t; int a, d;
    while (std::fscanf (f, "%lf %d %d", &t, &a, &d) == 3) { T.push_back (t); A.push_back (a); D.push_back (d); }
    const double sr = 44100;
    ym::Chip chip (sr); chip.interp = 0; chip.depth = std::atof (argc > 3 ? argv[3] : "16");
    const double t0 = T[0], tEnd = T.back() + 1500;
    const long N = (long) std::ceil ((tEnd - t0) / 1000 * sr);
    std::vector<float> L (N), R (N);
    long pos = 0; size_t i = 0;
    while (pos < N)
    {
        const double tms = t0 + pos / sr * 1000;
        while (i < T.size() && T[i] <= tms) { chip.write (A[i], D[i]); ++i; }
        long next = i < T.size() ? (long) std::ceil ((T[i] - t0) / 1000 * sr) : N;
        next = std::max (pos + 1, std::min (N, next));
        chip.process (L.data() + pos, R.data() + pos, (int) (next - pos)); pos = next;
    }
    FILE* o = std::fopen (argv[2], "wb");
    for (long k = 0; k < N; ++k) { std::fwrite (&L[k], 4, 1, o); std::fwrite (&R[k], 4, 1, o); }
    std::fclose (o); std::printf ("N %ld\n", N);
}
