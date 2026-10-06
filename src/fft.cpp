#include "fft.h"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace radar {
double hann(int n, int size) {
    return size <= 1 ? 1.0 : 0.5 - 0.5 * std::cos(2.0 * pi * n / (size - 1));
}
void fft(std::vector<Complex>& data) {
    const auto n = data.size();
    if (n == 0 || (n & (n - 1)) != 0) throw std::invalid_argument("FFT size must be a power of two");
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        auto bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(data[i], data[j]);
    }
    for (std::size_t length = 2; length <= n; length <<= 1) {
        const Complex step = std::polar(1.0, -2.0 * pi / static_cast<double>(length));
        for (std::size_t start = 0; start < n; start += length) {
            Complex rotation(1, 0);
            for (std::size_t j = 0; j < length / 2; ++j) {
                const Complex even = data[start + j];
                const Complex odd = rotation * data[start + j + length / 2];
                data[start + j] = even + odd;
                data[start + j + length / 2] = even - odd;
                rotation *= step;
            }
        }
    }
    // HLS: each unscaled butterfly can grow by one bit; choose stage scaling
    // and wider accumulators before replacing double with a fixed-point type.
}
}
