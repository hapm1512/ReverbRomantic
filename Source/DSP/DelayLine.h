#pragma once
#include <vector>
class RomanticDelayLine { public: void prepare(double,double); void reset(); void setDelaySamples(float) noexcept; float process(float) noexcept; private: std::vector<float> buffer; int writePosition=0; float delaySamples=1.0f; };
