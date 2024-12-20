#pragma once
#include <stdint.h>
#include <gte.h>

//#define NO_GTE

#define PREC     (12)
#define ONE      (1 << PREC)

// TODO: unionize these

typedef struct {
    int16_t v;  // don't directly access
} fx;

typedef struct {
    int32_t v;  // don't directly access
} fx32;

typedef struct {
    fx x, y, z;       // not compatible with GTEVector16 (unless not packed?)
} Vec;

typedef struct {
    fx32 x, y, z;     // this one is actually compatible with GTEVector32
} Vec32;

// NOTE: Vec not aligned to (4) can't be uploaded using lwc. also the 4th element won't be 0.
// is it faster to convert to GTEVector16 and lwc, or just mtc the one I already have?

typedef struct {
    int x, y, z;
} Veci;

typedef struct {
    float x, y, z;
} Vecf;

// has the correct bit representation for GTE. can type pun to GTEMatrix
typedef union {
    struct {
        fx m[3][3];
        fx _pad;
        Vec t;
    };
    GTEMatrix gte;
    Vec v[3];
} Mat;


float qtof(int);
int ftoq(float);

fx FX(int16_t);
fx32 FX32(int32_t);
fx ftofx(float v);
float fxtof(fx v);
fx itofx(int i);
int fxtoi(fx v);
fx fx_add(fx a, fx b);
fx fx_sub(fx a, fx b);
fx fx_mul(fx a, fx b);
fx fx_div(fx a, fx b);
fx fx_neg(fx v);
fx fx_sin(fx v);
fx fx_cos(fx v);
Vec vec_add(Vec a, Vec b);
Vec vec_sub(Vec a, Vec b);
Vec vec_scale(Vec v, fx s);
Vec vec_mul(Vec a, Vec b);
fx vec_dot(Vec a, Vec b);
fx vec_mag2(Vec a);
Vec vec_normalize_fake(Vec a);
Veci vec_toi(Vec);
Veci vec_raw(Vec);
GTEVector16 *vec_gte(Vec *v);
Mat mat_id(void);
Mat mat_scale(fx x, fx y, fx z);
Mat mat_transpose(Mat);
Mat mat_rotate_x(fx);
Mat mat_rotate_y(fx);
Mat mat_rotate_z(fx);
Mat mat_mul(Mat a, Mat b);
Vec mat_vec_multiply(Vec, Mat);
void transform_vecs(Vec *out, Vec *in, unsigned int n, Mat m);

fx fx_from32(fx32);
fx32 fx_to32(fx);
fx32 fx32_add(fx32, fx32);
fx32 fx32_sub(fx32, fx32);
fx32 fx32_mul(fx32, fx32);
fx32 fx32_div(fx32, fx32);
Vec32 vec_to32(Vec);
fx32 vec32_dot(Vec32, Vec32);

void gte_loadVec(int i, Vec v);

void mat_print(Mat);

