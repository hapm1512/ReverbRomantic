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

inline void butterfly (float& a, float& b) noexcept
{
    const float sum = a + b;
    const float difference = a - b;
    a = sum;
    b = difference;
}
}

void Matrix16::householder (Vector& values) noexcept
{
    // Balanced reduction improves instruction-level parallelism.
    const float s0 = values[0] + values[1] + values[2] + values[3];
    const float s1 = values[4] + values[5] + values[6] + values[7];
    const float s2 = values[8] + values[9] + values[10] + values[11];
    const float s3 = values[12] + values[13] + values[14] + values[15];
    const float reflection = (s0 + s1 + s2 + s3) * 0.125f;

    for (auto& value : values)
        value -= reflection;
}

void Matrix16::hadamard (Vector& v) noexcept
{
    // Fully unrolled 16-point Walsh-Hadamard transform.
    butterfly (v[0], v[1]);   butterfly (v[2], v[3]);
    butterfly (v[4], v[5]);   butterfly (v[6], v[7]);
    butterfly (v[8], v[9]);   butterfly (v[10], v[11]);
    butterfly (v[12], v[13]); butterfly (v[14], v[15]);

    butterfly (v[0], v[2]);   butterfly (v[1], v[3]);
    butterfly (v[4], v[6]);   butterfly (v[5], v[7]);
    butterfly (v[8], v[10]);  butterfly (v[9], v[11]);
    butterfly (v[12], v[14]); butterfly (v[13], v[15]);

    butterfly (v[0], v[4]);   butterfly (v[1], v[5]);
    butterfly (v[2], v[6]);   butterfly (v[3], v[7]);
    butterfly (v[8], v[12]);  butterfly (v[9], v[13]);
    butterfly (v[10], v[14]); butterfly (v[11], v[15]);

    butterfly (v[0], v[8]);   butterfly (v[1], v[9]);
    butterfly (v[2], v[10]);  butterfly (v[3], v[11]);
    butterfly (v[4], v[12]);  butterfly (v[5], v[13]);
    butterfly (v[6], v[14]);  butterfly (v[7], v[15]);

    for (auto& value : v)
        value *= hadamardNormalization;
}

void Matrix16::orthogonal (Vector& values) noexcept
{
    alignas (64) Vector permuted {};
    for (std::size_t i = 0; i < size; ++i)
        permuted[i] = values[permutation[i]] * signs[i];

    hadamard (permuted);
    householder (permuted);

    for (std::size_t i = 0; i < size; ++i)
        values[permutation[i]] = permuted[i] * signs[i];
}
