#pragma once

#include <array>
#include <cstddef>

// Fast orthogonal 16x16 feedback mixers.
// Every transform preserves energy and allocates no memory.
struct Matrix16
{
    static constexpr std::size_t size = 16;
    using Vector = std::array<float, size>;

    static void householder (Vector& values) noexcept;
    static void hadamard (Vector& values) noexcept;

    // Sign permutation + normalized Hadamard + Householder.
    // The basis changes remove fixed modal relationships while preserving energy.
    static void orthogonal (Vector& values) noexcept;
};
