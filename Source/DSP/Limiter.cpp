#include "Limiter.h"
void Limiter::process (float& left, float& right) const noexcept
{
    left = std::tanh (left * 1.15f) / std::tanh (1.15f);
    right = std::tanh (right * 1.15f) / std::tanh (1.15f);
}
