/*
#include "common.h"
#include "math.h"
#include "gpucmd.h"
*/
#define SCREEN_W    256
#define SCREEN_H    240

typedef struct PrimBuf PrimBuf;

extern volatile int frame;

PrimBuf *gpu_init(void);
uint32_t *next_prim(PrimBuf *pb, int len, int layer);
PrimBuf *swap_buffer(void);
void gpu_sync(void);
