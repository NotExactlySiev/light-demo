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

// TODO: models init function with X() macro

// TODO: move
Mat projection;

void draw_line(PrimBuf *pb, Veci a, Veci b, uint32_t color)
{
    // TODO: actually sort
    uint32_t *prim = next_prim(pb, 3, 63);
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
    z = (z + 2048) / 64;
    uint32_t *prim = next_prim(pb, 6, z);
    *prim++ = (col0 & 0xFFFFFF) | _gp0_polygon(false, false, true, false, false);
    *prim++ = gp0_xy(v0.x, v0.y);
    *prim++ = col1 & 0xFFFFFF;
    *prim++ = gp0_xy(v1.x, v1.y);
    *prim++ = col2 & 0xFFFFFF;
    *prim++ = gp0_xy(v2.x, v2.y);
}

void normal_to_color(uint32_t *out, Vec *n)
{
    // TODO: NO_GTE mode
    // TODO: do this once per vertex
    // TODO: have the lights in a light structure. then read into
    // gte matrix and transpose the colors
    // doing one light for now

    gte_setV0(n[0].x.v, n[0].y.v, n[0].z.v);
    gte_setV1(n[1].x.v, n[1].y.v, n[1].z.v);
    gte_setV2(n[2].x.v, n[2].y.v, n[2].z.v);
    gte_command(GTE_CMD_NCT | GTE_SF);
    out[0] = 0xFFFFFF & gte_getDataReg(GTE_RGB0);
    out[1] = 0xFFFFFF & gte_getDataReg(GTE_RGB1);
    out[2] = 0xFFFFFF & gte_getDataReg(GTE_RGB2);
}


void draw_object(PrimBuf *pb, Object *obj, Light *lights)
{
    Mat mat = mat_rotate_y(obj->angle_y);
    mat.t = obj->pos;

    update_llm(lights, obj);
    Mat mvp = mat_mul(projection, mat);

    Model *m = obj->model;
    Vec proj[m->nverts];
    transform_vecs(proj, m->verts, m->nverts, mvp);

    for (int i = 0; i < m->nfaces; i++) {
        uint16_t *face = (*m->faces)[i];
        Veci v0 = vec_raw(proj[face[0]]);
        Veci v1 = vec_raw(proj[face[1]]);
        Veci v2 = vec_raw(proj[face[2]]);
        Vec n[3] = {
            m->normals[face[0]],
            m->normals[face[1]],
            m->normals[face[2]],
        };
        uint32_t color[3] = { 255, 255, 255 };
        normal_to_color(color, n);
        draw_triangle(pb, v0, v1, v2,
            color[0],
            color[1],
            color[2]
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

    // TODO: move these to gpu_init
    Model *cube_model  = load_model(_binary_bin_cube_ply_start, FX(ONE/16));
    Model *ball_model  = load_model(_binary_bin_ball_ply_start, FX(ONE/13));
    Model *monke_model = load_model(_binary_bin_monke_ply_start, FX(ONE/12));
    // orthographic for now. only scale to screen
    projection = mat_mul(mat_scale(FX(ONE/10), FX(ONE/10), FX(ONE)),
                 mat_mul(mat_rotate_x(FX(ONE/12)),
                         mat_rotate_y(FX(ONE/16))));

    Material material1 = {
        .ambient = { FX(ONE), FX(ONE), FX(ONE) },
        .diffuse = { FX(ONE), FX(ONE/2), FX(ONE/2) },
    };

    Material material2 = {
        .ambient = { FX(ONE), FX(ONE), FX(ONE) },
        .diffuse = { FX(ONE), FX(ONE), FX(ONE) },
    };

    Object cube = {
        .pos = { FX(800), FX(0), FX(200) },
        .model = cube_model,
        .material = &material1,
    };
    Object ball1 = {
        .pos = { FX(-650), FX(0), FX(200) },
        .model = ball_model,
        .material = &material1,
    };
    Object ball2 = {
        .pos = { FX(650), FX(0), FX(400) },
        .model = ball_model,
        .material = &material2,
    };
    Object monke = {
        .pos = { FX(0), FX(0), FX(-700) },
        .model = monke_model,
        .material = &material2,
    };

    // TODO: put this stuff in a scene struct
    Light lights[3] = {
        {
            .kind = LIGHT_POINT,
            .vec = { FX(0), FX(0), FX(0) },
            .ambient = { FX(100), FX(0), FX(0) },
            .diffuse = { FX(1000), FX(900), FX(900) },
            .k1 = fx32_div(FX32(ONE), FX32(600)),
        },
        { .kind = LIGHT_NONE },
        { .kind = LIGHT_NONE },
    };

    fx t = FX(0);
    fx angle = FX(0);
    PrimBuf *pb = gpu_init();
    for (;;) {
        printf("Frame %d\n", frame);

        lights[0].vec.y = fx_mul(fx_sin(t), FX(ONE/3));
        cube.angle_y = angle;
        monke.angle_y = fx_add(fx_mul(angle, FX(ONE/3)), FX(-ONE/8));

        //draw_object(pb, &cube, lights);
        draw_object(pb, &ball1, lights);
        draw_object(pb, &ball2, lights);
        draw_object(pb, &monke, lights);
        draw_axes(pb);

        pb = swap_buffer();
        
        angle = fx_add(angle, FX(21));
        t = fx_add(t, FX(30));
    }
}
