#pragma once

typedef struct Object Object;

#include "gpu.h"
#include "model.h"
#include "light.h"

struct Object {
    Vec pos;
    fx angle_y;
    Model *model;
    Material *material;
};

// TODO: lights should come from the scene
void draw_object(PrimBuf *pb, Object *obj, Light *lights);
