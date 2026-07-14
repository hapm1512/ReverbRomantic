#include "DelayLine.h"
#include <algorithm>
#include <cmath>
void RomanticDelayLine::prepare(double sr,double maxSeconds){buffer.assign((size_t)std::max(2,(int)std::ceil(sr*maxSeconds)+2),0.0f);reset();}
void RomanticDelayLine::reset(){std::fill(buffer.begin(),buffer.end(),0.0f);writePosition=0;}
void RomanticDelayLine::setDelaySamples(float v) noexcept { if(buffer.size()>2) delaySamples=std::clamp(v,1.0f,(float)buffer.size()-2.0f); }
float RomanticDelayLine::process(float x) noexcept { if(buffer.empty()) return x; buffer[(size_t)writePosition]=x; float rp=(float)writePosition-delaySamples; while(rp<0)rp+=(float)buffer.size(); int a=(int)rp,b=(a+1)%(int)buffer.size(); float f=rp-(float)a; float y=buffer[(size_t)a]+f*(buffer[(size_t)b]-buffer[(size_t)a]); writePosition=(writePosition+1)%(int)buffer.size(); return y; }
