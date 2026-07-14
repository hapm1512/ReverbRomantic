#include "Matrix16.h"

void Matrix16::householder (Vector& values) noexcept
{
    float sum = 0.0f;

    for (const auto value : values)
        sum += value;

    // 2 / 16 = 1 / 8.
    const float reflection = sum * 0.125f;

    for (auto& value : values)
        value -= reflection;
}

void Matrix16::hadamard (Vector& values) noexcept
{
    householder (values);
}
