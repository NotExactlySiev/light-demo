#include <stdint.h>
#include <gte.h>

#define PREC     (12)
#define ONE      (1 << PREC)

// TODO: unionize these

typedef struct {
    int16_t v;  // don't directly access
} fx;

typedef struct {
    fx x, y, z;
} Vec;

typedef struct {
    int x, y, z;
} Veci;

typedef struct {
    float x, y, z;
} Vecf;

// has the correct bit representation for GTE. can type pun to GTEMatrix
typedef struct {
    fx m[3][3];
    fx _pad;
} Mat;


float qtof(int);
int ftoq(float);

fx ftofx(float v);
float fxtof(fx v);
fx itofx(int i);
int fxtoi(fx v);
fx fx_add(fx a, fx b);
fx fx_sub(fx a, fx b);
fx fx_mul(fx a, fx b);
fx fx_div(fx a, fx b);
Vec vec_add(Vec a, Vec b);
Vec vec_sub(Vec a, Vec b);
Vec vec_scale(Vec v, fx s);
fx vec_dot(Vec a, Vec b);
GTEVector16 vec_gte(Vec v);

