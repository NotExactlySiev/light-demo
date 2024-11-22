#include <stdint.h>
#include <gte.h>
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

fx FX(const int16_t v)
{
    return (fx) { v };
}

fx ftofx(float v)
{
    return (fx) { ftoq(v) };
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

fx fx_neg(fx x)
{
    return (fx) { -x.v };
}

fx fx_one(void)
{
    return (fx) { ONE };
}

fx fx_zero(void)
{
    return (fx) { 0 };
}

#define qN_l	10
#define qN_h	15
#define qA		12
#define B		19900
#define	C		3516

static inline int _isin(int qN, int x) {
    int c, y;
    c  = x << (30 - qN);			// Semi-circle info into carry.
    x -= 1 << qN;					// sine -> cosine calc

    x <<= (31 - qN);				// Mask with PI
    x >>= (31 - qN);				// Note: SIGNED shift! (to qN)
    x  *= x;
    x >>= (2 * qN - 14);			// x=x^2 To Q14

    y = B - (x * C >> 14);			// B - x^2*C
    y = (1 << qA) - (x * y >> 16);	// A - x^2*(B-x^2*C)

    return (c >= 0) ? y : (-y);
}

fx fx_sin(fx x) {
    return (fx) { _isin(qN_l, x.v) };
}

fx fx_cos(fx x) {
	return (fx) { _isin(qN_l, x.v + (1 << qN_l)) };
}

/*
int hisin(int x) {
	return _isin(qN_h, x);
}

int hicos(int x) {
	return _isin(qN_h, x + (1 << qN_h));
}
*/

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

fx vec_mag2(Vec a)
{
    return vec_dot(a, a);
}

Vec vec_normalize_fake(Vec a)
{
    return vec_scale(a, fx_div(FX(ONE), vec_mag2(a)));
}

// TODO: make this a union too?
GTEVector16 *vec_gte(Vec *v)
{
    //return (GTEVector16) { v.x.v, v.y.v, v.z.v };
    return (GTEVector16 *) v;
}

Vec mat_vec_mul(Mat m, Vec v)
{
    return vec_add((Vec) {
        .x = vec_dot(m.v[0], v),
        .y = vec_dot(m.v[1], v),
        .z = vec_dot(m.v[2], v),
    }, m.t);
}


Veci vec_toi(Vec v)
{
    return (Veci) {
        .x = fxtoi(v.x),
        .y = fxtoi(v.y),
        .z = fxtoi(v.z),
    };
}

// careful!
Veci vec_raw(Vec v)
{
    return (Veci) { v.x.v, v.y.v, v.z.v };
}

fx fx_from32(fx32 x)
{
    return (fx) { x.v };
}

fx32 fx_to32(fx x)
{
    return (fx32) { x.v };
}

fx32 fx32_add(fx32 x, fx32 y)
{
    return (fx32) { x.v + y.v };
}

fx32 fx32_mul(fx32 x, fx32 y)
{
    return (fx32) { x.v * y.v / ONE };
}

fx32 fx32_div(fx32 x, fx32 y)
{
    return (fx32) { (x.v * ONE) / y.v };
}

Vec32 vec_to32(Vec v)
{
    return (Vec32) { fx_to32(v.x), fx_to32(v.y), fx_to32(v.z) };
}

fx32 vec32_dot(Vec32 a, Vec32 b)
{
    // ((a*b)*c) != (a*b*c) in fixed point. divide at the end loses less precision.
    /*return fx32_add(fx32_mul(a.x, b.x),
           fx32_add(fx32_mul(a.y, b.y),
                    fx32_mul(a.z, b.z)));*/
    return (fx32) { (a.x.v * b.x.v + a.y.v * b.y.v + a.z.v * b.z.v) / ONE };
}


Mat mat_id(void)
{
    return (Mat) {{{
        { fx_one(), fx_zero(), fx_zero() },
        { fx_zero(), fx_one(), fx_zero() }, 
        { fx_zero(), fx_zero(), fx_one() },
    }}};
}

Mat mat_transpose(Mat m)
{
    return (Mat) {{{
        { m.m[0][0], m.m[1][0], m.m[2][0] },
        { m.m[0][1], m.m[1][1], m.m[2][1] },
        { m.m[0][2], m.m[1][2], m.m[2][2] },
    }}};
}

Mat mat_rotate_x(fx angle)
{
    fx c = fx_cos(angle);
    fx s = fx_sin(angle);

    return (Mat) {{{
        { fx_one(),  fx_zero(), fx_zero() },
        { fx_zero(), c,         fx_neg(s) }, 
        { fx_zero(), s,         c         },
    }}};
}

Mat mat_rotate_y(fx angle)
{
    fx c = fx_cos(angle);
    fx s = fx_sin(angle);

    return (Mat) {{{
        { c,         fx_zero(), s         }, 
        { fx_zero(), fx_one(),  fx_zero() },
        { fx_neg(s), fx_zero(), c         },
    }}};
}

Mat mat_rotate_z(fx angle)
{
    fx c = fx_cos(angle);
    fx s = fx_sin(angle);

    return (Mat) {{{
        { c,         fx_neg(s), fx_zero() }, 
        { s,         c,         fx_zero() },
        { fx_zero(), fx_zero(), fx_one()  },
    }}};
}

Mat mat_scale(fx x, fx y, fx z)
{
    return (Mat) {{{
        { x        , fx_zero(), fx_zero() },
        { fx_zero(), y        , fx_zero() }, 
        { fx_zero(), fx_zero(), z         },
    }}};
}

Vec mat_vec_multiply(Vec v, Mat m)
{
    return (Vec) {
        .x = vec_dot(m.v[0], v),
        .y = vec_dot(m.v[1], v),
        .z = vec_dot(m.v[2], v),
    };
}

// return AB, that is, A happens after B
Mat mat_mul(Mat a, Mat b)
{
    Mat bt = mat_transpose(b);
    //return (Mat) {
    Mat ret = {
       .v = {
           mat_vec_multiply(a.v[0], bt),
           mat_vec_multiply(a.v[1], bt),
           mat_vec_multiply(a.v[2], bt),
       },

       // can't do this for some reason :/ 
       //.t = vec_add(a.t, mat_vec_multiply(b.t, a));
    };
    ret.t = vec_add(a.t, mat_vec_multiply(b.t, a));
    return ret;
}

static void gte_load_matrix(Mat m)
{
    gte_loadRotationMatrix(&m.gte);
    gte_setControlReg(GTE_TRX, m.t.x.v);
    gte_setControlReg(GTE_TRY, m.t.y.v);
    gte_setControlReg(GTE_TRZ, m.t.z.v);
}

void transform_vecs(Vec *out, Vec *in, unsigned int n, Mat m)
{
#ifdef NO_GTE
    for (int i = 0; i < n; i++) {
        out[i] = mat_vec_multiply(in[i], m);
    }
#else
    gte_load_matrix(m);
    for (int i = 0; i < n; i++) {
        //gte_loadV0(vec_gte(&in[i]));
        gte_setV0(in[i].x.v, in[i].y.v, in[i].z.v);
        gte_command(GTE_CMD_MVMVA | GTE_SF | GTE_MX_RT | GTE_V_V0 | GTE_CV_TR);
        // dunno if there's a better way to do this
        //gte_storeDataReg(GTE_IR1, 0, &out[i].gte);
        out[i].x.v = gte_getDataReg(GTE_IR1);
        out[i].y.v = gte_getDataReg(GTE_IR2);
        out[i].z.v = gte_getDataReg(GTE_IR3);
        //gte_storeDataReg(GTE_IR3, 4, &out[i].gte);
    }
#endif
}

// lol
#define DEC(x)  (((((x) * 10)/8 * 10)/8 * 10)/ 8 * 10)/ 8

void printf(const char*, ...);

void fx_print(fx x)
{
    int16_t v = x.v;
    if (v >= 0)
        printf(" %d.%04d", v >> 12, DEC(v & (ONE - 1)));
    else
        printf("-%d.%04d", (-v) >> 12, DEC((-v) & (ONE - 1)));

    printf(" [0x%04X] ", v & 0xFFFF);
}

void vec_print(Vec v)
{
    fx_print(v.x);
    fx_print(v.y);
    fx_print(v.z);
    printf("\n");
}

void mat_print(Mat m)
{
    printf("Matrix:\n");
    vec_print(m.v[0]);
    vec_print(m.v[1]);
    vec_print(m.v[2]);
    vec_print(m.t);
    printf("\n");
}
