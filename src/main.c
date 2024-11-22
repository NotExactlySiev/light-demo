#include <registers.h>
#include <cop0.h>
#include <gpucmd.h>
#include "kernel.h"
#include "gpu.h"
#include "model.h"
#include "light.h"
#include "object.h"

#define SCREEN_W 256
#define SCREEN_H 240

#define EMBED_BIN(s) \
    extern uint8_t _binary_bin_##s##_start[]; \
    extern uint32_t _binary_bin_##s##_size[];

#define EMBED_MODEL(s)    EMBED_BIN(s##_ply)

EMBED_MODEL(ball)
EMBED_MODEL(cube)
EMBED_MODEL(monke)
EMBED_MODEL(weird)

// TODO: models init function with X() macro

// TODO: move
Mat projection;

void draw_line(PrimBuf *pb, Veci a, Veci b, uint32_t color)
{
    // TODO: actually sort
    uint32_t *prim = next_prim(pb, 3, 255);
    prim[0] = color | gp0_line(false, false);
    prim[1] = gp0_xy(a.x, a.y);
    prim[2] = gp0_xy(b.x, b.y);
}

void draw_point(PrimBuf *pb, Veci pt, uint32_t color)
{
    uint32_t *prim = next_prim(pb, 2, 1);
    prim[0] = color | gp0_rectangle1x1(false, true, false);
    prim[1] = gp0_xy(pt.x, pt.y);
}

void draw_axes(PrimBuf *pb)
{
    Vec verts[4] = {
        [1] = { FX(ONE/4),  FX(0),      FX(0)   },
        { FX(0),    FX(ONE/4),    FX(0)   },
        { FX(0),    FX(0),      FX(ONE/4) },
    };

    Vec proj[4];
    Veci draw[4];

    //if (!in_view(verts[0], &proj[0]))
    //    return;

    transform_vecs(&proj[0], verts, 4, projection);
    for (int i = 0; i < 4; i++) {
        draw[i] = vec_raw(proj[i]);
    }
    draw_line(pb, draw[0], draw[1], gp0_rgb(255, 0, 0));
    draw_line(pb, draw[0], draw[2], gp0_rgb(0, 255, 0));
    draw_line(pb, draw[0], draw[3], gp0_rgb(0, 0, 255));
}

void draw_triangle(PrimBuf *pb, Veci v0, Veci v1, Veci v2, uint32_t col0, uint32_t col1, uint32_t col2)
{
    int z = INT16_MIN;
    if (v0.z > z) z = v0.z;
    if (v1.z > z) z = v1.z;
    if (v2.z > z) z = v2.z;
    z = (v0.z + v1.z + v2.z + z)/4;
    // TODO: don't harcode this transform
    z = (z + 2048) / 16;
    uint32_t *prim = next_prim(pb, 6, z);
    *prim++ = (col0 & 0xFFFFFF) | _gp0_polygon(false, false, true, false, false);
    *prim++ = gp0_xy(v0.x, v0.y);
    *prim++ = col1 & 0xFFFFFF;
    *prim++ = gp0_xy(v1.x, v1.y);
    *prim++ = col2 & 0xFFFFFF;
    *prim++ = gp0_xy(v2.x, v2.y);
}

void shade_verts(uint32_t *out, Vec *normals, int n)
{
    // TODO: NO_GTE mode
    int i = 0;
    for (i = 0; i < n % 3; i++) {
        gte_loadVec(0, normals[i]);
        gte_command(GTE_CMD_NCS | GTE_SF);
        gte_storeDataReg(GTE_RGB2, 0, &out[i]);
    }
    
    for (; i < n; i += 3) {
        gte_loadVec(0, normals[i + 0]);
        gte_loadVec(1, normals[i + 1]);
        gte_loadVec(2, normals[i + 2]);
        gte_command(GTE_CMD_NCT | GTE_SF);
        gte_storeDataReg(GTE_RGB0, 0, &out[i]);
        gte_storeDataReg(GTE_RGB1, 4, &out[i]);
        gte_storeDataReg(GTE_RGB2, 8, &out[i]);
    }
}

void draw_object(PrimBuf *pb, Object *obj, Light *lights)
{
    Mat mat = mat_rotate_y(obj->angle_y);
    mat.t = obj->pos;
    
    Mat mvp = mat_mul(projection, mat);
    Model *m = obj->model;

    Vec proj[m->nverts];
    uint32_t colors[m->nverts];

    // TODO: there might be a way of only shading the vertices that we need
    transform_vecs(proj, m->verts, m->nverts, mvp);
    update_llm(lights, obj);
    shade_verts(colors, m->normals, m->nverts);

    for (int i = 0; i < m->nfaces; i++) {
        uint16_t *face = (*m->faces)[i];
        Veci v0 = vec_raw(proj[face[0]]);
        Veci v1 = vec_raw(proj[face[1]]);
        Veci v2 = vec_raw(proj[face[2]]);
        gte_setDataReg(GTE_SXY0, *(uint32_t *) &proj[face[0]]);
        gte_setDataReg(GTE_SXY1, *(uint32_t *) &proj[face[1]]);
        gte_setDataReg(GTE_SXY2, *(uint32_t *) &proj[face[2]]);
        gte_command(GTE_CMD_NCLIP);
        if ((int32_t) gte_getDataReg(GTE_MAC0) >= 0) continue;
        draw_triangle(pb, v0, v1, v2,
            colors[face[0]],
            colors[face[1]],
            colors[face[2]]
        );
    }
}

static void gte_init(void) {
    uint32_t sr = cop0_getReg(COP0_SR);
    cop0_setReg(COP0_SR, sr | COP0_SR_CU2);
    //gte_setControlReg(GTE_H, SCREEN_W/2);
}

uint8_t *arena = (uint8_t *) 0x80100000;

void *alloc(size_t amt)
{
    amt = (amt + 3) & ~3;
    printf("allocating %ld\n", amt);
    void *p = arena;
    arena += amt;
    return p;
}

Model *load_model(uint8_t *data, fx scale)
{
    Model *m = alloc(sizeof(*m));
    void *p = model_new_ply(m, data);

    m->verts = alloc(sizeof(Vec) * m->nverts);
    m->normals = alloc(sizeof(Vec) * m->nverts);
    m->faces = alloc(sizeof(uint16_t) * m->nfaces * 3);

    model_read_data(m, p, scale);
    return m;
}


int _start()
{
    gte_init();

    Model *cube_model  = load_model(_binary_bin_cube_ply_start, FX(ONE/16));
    Model *ball_model  = load_model(_binary_bin_ball_ply_start, FX(ONE/13));
    Model *lamp_model  = load_model(_binary_bin_ball_ply_start, FX(ONE/64));
    Model *monke_model = load_model(_binary_bin_monke_ply_start, FX(ONE/12));
    Model *weird_model = load_model(_binary_bin_weird_ply_start, FX(ONE/12));

    // orthographic for now. only scale to screen
    projection = mat_mul(mat_scale(FX(ONE/10), FX(ONE/10), FX(ONE)),
                 mat_mul(mat_rotate_x(FX(ONE/12)),
                         mat_rotate_y(FX(ONE/16))));

    // TODO: put this stuff in a scene struct
    Light lights[3] = {
        {
            .kind = LIGHT_POINT,
            .vec = { FX(0), FX(-1500), FX(0) },
            .ambient = { FX(200), FX(200), FX(150) },
            .diffuse = { FX(2150), FX(2100), FX(2000) },
            .k1 = fx32_div(FX32(ONE), FX32(600)),
        },
        {
            .kind = LIGHT_POINT,
            .vec = { FX(0), FX(-1500), FX(0) },
            .ambient = { FX(40), FX(40), FX(40) },
            .diffuse = { FX(0), FX(2000), FX(0) },
            .k1 = fx32_div(FX32(ONE), FX32(600)),
        },
        { .kind = LIGHT_NONE },
    };
    Material material1 = {
        .ambient = { FX(ONE/14), FX(ONE/14), FX(ONE/14) },
        .diffuse = { FX(ONE), FX(ONE), FX(ONE) },
    };

    // TODO: lamp object. have a way of automatically linking this to the light
    // and have the emissive thing be shared by its light object
    Material lamp_material = {
        .emissive = { FX(3500), FX(3500), FX(3300) },
    };

    Object cube = {
        .pos = { FX(700), FX(0), FX(0) },
        .model = cube_model,
        .material = &material1,
    };

    Object lamp = {
        .pos = { FX(0), FX(-1000), FX(0) },
        .model = lamp_model,
        .material = &lamp_material,
    };

    Object ball1 = {
        .pos = { FX(700), FX(0), FX(500) },
        .model = ball_model,
        .material = &material1,
    };

    Object weird = {
        .pos = { FX(-600), FX(0), FX(0) },
        .model = weird_model,
        .material = &material1,
    };

    fx t = FX(0);
    fx angle = FX(0);
    PrimBuf *pb = gpu_init();
    for (;;) {
        printf("Frame %d\n", frame);

        Vec lamp_pos = (Vec) { fx_mul(fx_sin(t), FX(ONE/3)), FX(-800), FX(0) };
        lights[0].vec = lamp_pos;
        lamp.pos = lamp_pos;
        weird.angle_y = fx_add(fx_mul(angle, FX(ONE/3)), FX(-ONE/8));

        // lamp can't be lit by the light inside itself. because the distance is zero.
        // so we only light it by the other lights :D
        Light no_lights[3] = { [1] = lights[1], [2] = lights[2] };
        draw_object(pb, &lamp, no_lights);

        draw_object(pb, &ball1, lights);
        draw_object(pb, &weird, lights);

        draw_axes(pb);
        pb = swap_buffer();
        
        angle = fx_add(angle, FX(21));
        t = fx_add(t, FX(30));
    }
}
