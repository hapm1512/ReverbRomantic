#pragma once
#include <array>
struct Matrix16 { using Vector=std::array<float,16>; static void hadamard(Vector&) noexcept; };
