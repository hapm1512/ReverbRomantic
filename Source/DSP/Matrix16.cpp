#include "Matrix16.h"

namespace
{
constexpr float hadamardNormalization = 0.25f; // 1 / sqrt (16)
constexpr std::array<std::size_t, Matrix16::size> permutation
{
    0, 5, 10, 15, 3, 8, 13, 2,
    7, 12, 1, 6, 11, 4, 9, 14
};
constexpr std::array<float, Matrix16::size> signs
{
     1.0f, -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,  1.0f,
     1.0f,  1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  1.0f
};
}

void Matrix16::householder (Vector& values) noexcept
{
    float sum = 0.0f;
    for (const auto value : values)
        sum += value;

    const float reflection = sum * 0.125f; // 2 / 16
    for (auto& value : values)
        value -= reflection;
}

void Matrix16::hadamard (Vector& values) noexcept
{
    for (std::size_t width = 1; width < size; width <<= 1u)
    {
        const std::size_t block = width << 1u;
        for (std::size_t base = 0; base < size; base += block)
        {
            for (std::size_t lane = 0; lane < width; ++lane)
            {
                const float a = values[base + lane];
                const float b = values[base + lane + width];
                values[base + lane] = a + b;
                values[base + lane + width] = a - b;
            }
        }
    }

    for (auto& value : values)
        value *= hadamardNormalization;
}

void Matrix16::orthogonal (Vector& values) noexcept
{
    Vector permuted {};
    for (std::size_t i = 0; i < size; ++i)
        permuted[i] = values[permutation[i]] * signs[i];

    hadamard (permuted);
    householder (permuted);

    for (std::size_t i = 0; i < size; ++i)
        values[permutation[i]] = permuted[i] * signs[i];
}
