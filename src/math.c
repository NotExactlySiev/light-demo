#include <stdint.h>
//#include <math.h>
#include "math.h"

// do we need a non-static version of this? yes
/*
fx ftofx(float v)
{
    return (fx) { ftoq(v) };
    //return (fx) { roundf(v * ONE) };
}
*/

fx ftofx(const float v)
{
    return (fx) { ONE*v };
}

float fxtof(fx v)
{
    return qtof(v.v);
}

fx itofx(int i)
{
    return (fx) { i * ONE };
}

int fxtoi(fx v)
{
    return v.v / ONE;
}

fx fx_add(fx a, fx b)
{
    return (fx) { a.v + b.v };
}

fx fx_sub(fx a, fx b)
{
    return (fx) { a.v - b.v };
}

fx fx_mul(fx a, fx b)
{
    return (fx) { a.v * b.v / ONE };
}

fx fx_div(fx a, fx b)
{
    return (fx) { (a.v * ONE) / b.v };
}

Vec vec_add(Vec a, Vec b)
{
    return (Vec) {
        .x = fx_add(a.x, b.x),
        .y = fx_add(a.y, b.y),
        .z = fx_add(a.z, b.z),
    };
}

Vec vec_sub(Vec a, Vec b)
{
    return (Vec) {
        .x = fx_sub(a.x, b.x),
        .y = fx_sub(a.y, b.y),
        .z = fx_sub(a.z, b.z),
    };
}

Vec vec_scale(Vec v, fx s)
{
    return (Vec) {
        .x = fx_mul(v.x, s),
        .y = fx_mul(v.y, s),
        .z = fx_mul(v.z, s),
    };
}

fx vec_dot(Vec a, Vec b)
{
    // ugly. can fx_add be made variadic?
    return fx_add(fx_mul(a.x, b.x),
           fx_add(fx_mul(a.y, b.y),
                  fx_mul(a.z, b.z)));
}

GTEVector16 vec_gte(Vec v)
{
    return (GTEVector16) { v.x.v, v.y.v, v.z.v };
}

GTEMatrix *mat_gte(Mat *m)
{
    return (GTEMatrix *) m;
}
