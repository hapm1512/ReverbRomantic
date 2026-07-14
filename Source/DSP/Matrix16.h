#pragma once

#include <array>
#include <cstddef>

// Orthogonal 16x16 feedback mixer for the FDN.
//
// The Householder transform is:
//     y = x - (2 / N) * sum(x)
//
// For N = 16 the scale is 1 / 8. The transform is energy preserving,
// deterministic and requires no stored 16x16 coefficient matrix.
struct Matrix16
{
    static constexpr std::size_t size = 16;
    using Vector = std::array<float, size>;

    // Preferred API for Epic 3B.1A.
    static void householder (Vector& values) noexcept;

    // Backward-compatible entry point used by the current HybridFDN16 code.
    // It now applies the Householder transform instead of Hadamard mixing.
    static void hadamard (Vector& values) noexcept;
};
