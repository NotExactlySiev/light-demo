#include "math.h"
#include "object.h"
#include "light.h"

static fx get_attenuation(Light l, Vec pos)
{
    Vec d = vec_sub(l.vec, pos);
    Vec32 dd = vec_to32(d);
    // TODO: no k0 and k2 for now. those need a square root :P
    // attenuation is actually 1/(k0 + k1*d + k2*d^2), but we do another
    // division by d to normalize the direction vector upfront.
    // this means we could replace the stored k1 by 1/k1 for higher precision.
    // (if 0 <= k1 <= 1, that is)
    //fx32 d1 = {0};
    fx32 d2 = vec32_dot(dd, dd);
    //fx32 d3 = {0};
    fx32 att = fx32_div(FX32(ONE), fx32_mul(l.k1, d2));
    return fx_from32(att);
}

// light values calculated for a single point (object or vertex)
typedef struct {
    Vec lv;
    Vec lc;
    Vec bk;
} LightVectors;

static LightVectors calculate_light_vector(Light l, Object *obj)
{
    LightVectors vecs = {0};

    switch(l.kind) {
    case LIGHT_POINT:
        // this needs full 32-bit precision.
        Vec d = vec_sub(l.vec, obj->pos);
        /*
        Vec32 dd = vec_to32(d);
        fx32 factor = fx32_div(fx_to32(l.power), vec32_dot(dd, dd));
        return vec_scale(d, fx_from32(factor));*/

        fx factor = get_attenuation(l, obj->pos);
        Vec ambient = vec_scale(vec_mul(obj->material->ambient, l.ambient), factor);
        Vec diffuse = vec_mul(obj->material->diffuse, l.diffuse);

        return (LightVectors) {
            .lv = vec_scale(d, factor),
            .lc = diffuse,
            .bk = ambient,
        };
        
    case LIGHT_SUN:
        // TODO: normalize and use power
        //lv[i] = vec_scale(vec_normalize_fake(lights[0].dir), lights[0].power);
        return (LightVectors) {
            .lv = l.vec,
        };
    
    case LIGHT_NONE:
    default:
        return (LightVectors) {0};
    }
}

void update_llm(Light *lights, Object *obj)
{
    Mat lm = {0};
    Mat lc = {0};
    Vec bk = {0};

    for (int i = 0; i < 3; i++) {
        LightVectors vecs = calculate_light_vector(lights[i], obj);
        lm.v[i] = vecs.lv;
        lc.v[i] = vecs.lc;
        bk = vec_add(bk, vecs.bk);
    }


    // local transform matrix. should NOT scale or translate. rotation only
    Mat rot = mat_rotate_y(fx_neg(obj->angle_y));
    // the line below is functionally the same as this:
    // lm = mat_transpose(mat_mul(mat, mat_transpose(lm)));
    // because (AB^)^ == BA^
    lm = mat_mul(lm, mat_transpose(rot));

    // the colors are in columns, not rows
    lc = mat_transpose(lc);

    gte_setControlReg(GTE_RBK, bk.x.v);
    gte_setControlReg(GTE_GBK, bk.y.v);
    gte_setControlReg(GTE_BBK, bk.z.v);
    gte_loadLightColorMatrix(&lc.gte);
    gte_loadLightMatrix(&lm.gte);
}

