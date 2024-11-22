#pragma once

typedef struct Light Light;
typedef struct Material Material;

#include "object.h"

typedef enum {
    LIGHT_NONE,
    LIGHT_SUN,
    LIGHT_POINT,
} LightKind;

struct Light {
    Vec vec;  // direction or position
    LightKind kind;

    Vec ambient;
    Vec diffuse;
    //Vec specular;

    // attenuation factors
    fx32 k0, k1, k2;
    
};

struct Material {
    Vec ambient;
    Vec diffuse;
    //Vec specular;
    //Vec emissive;
    //per vertex?
};

void calculate_lights(Vec *out, Light *lights, int n, Object *obj);
void update_llm(Vec *lights, Mat mat);
