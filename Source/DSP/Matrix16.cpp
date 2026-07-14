#include "Matrix16.h"
void Matrix16::hadamard(Vector& v) noexcept { for(int step=1;step<16;step*=2) for(int i=0;i<16;i+=step*2) for(int j=0;j<step;++j){auto a=v[(size_t)(i+j)],b=v[(size_t)(i+j+step)];v[(size_t)(i+j)]=a+b;v[(size_t)(i+j+step)]=a-b;} for(auto& x:v)x*=0.25f; }
