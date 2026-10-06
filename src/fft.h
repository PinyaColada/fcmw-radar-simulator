#pragma once
#include "config.h"
namespace radar {
// In-place, unnormalized forward radix-2 FFT: X[k] = sum x[n] exp(-j*2pi*k*n/N).
void fft(std::vector<Complex>& data);
double hann(int n, int size);
}
