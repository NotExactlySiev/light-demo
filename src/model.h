#pragma once
#include "math.h"

typedef struct {
    uint16_t nverts;
    uint16_t nfaces;
    Vec *verts;
    Vec *normals;
    uint16_t (*faces)[][3];
} Model;

void *model_new_ply(Model *m, void *data);
int   model_read_data(Model *m, void *data, fx scale);
