#include "math.h"
#include "light.h"

static Vec calculate_light_vector(Light l, Vec pos)
{
    switch(l.kind) {
    case LIGHT_POINT:
        // mag2 should probably return int32
        // this should be done at the object routine (it receives the lights that need updating for it)
        // 32-bit calculation
        // TODO: use proper fx32 types
        Vec d = vec_sub(l.vec, pos);
        int32_t factor = (l.power.v * ONE) / ((d.x.v * d.x.v + d.y.v * d.y.v + d.z.v * d.z.v) / ONE);
        return vec_scale(d, FX(factor));
        
    case LIGHT_SUN:
        // TODO: normalize and use power
        //lv[i] = vec_scale(vec_normalize_fake(lights[0].dir), lights[0].power);
        return l.vec;
    case LIGHT_NONE:
        return (Vec) {0};
    }
}

// calculate light vectors for object
void calculate_lights(Vec *out, Light *lights, int n, Vec pos)
{
    for (int i = 0; i < n; i++) {
        out[i] = calculate_light_vector(lights[i], pos);
    }
}

// model matrix. should NOT scale
void update_llm(Vec *lights, Mat mat)
{
    Mat lm = {
        .v = { lights[0], lights[1], lights[2] }
    };

    // only use the rotation (shouldn't scale either)
    mat.t = (Vec) {0};
    // this is functionally the same as this:
    // Mat llm = mat_transpose(mat_mul(mat, mat_transpose(lm)));
    // because (AB^)^ == BA^
    Mat llm = mat_mul(lm, mat_transpose(mat));
    gte_loadLightMatrix(&llm.gte);
}

