#include <registers.h>
#include <cop0.h>
#include <gpucmd.h>
//#include "math.h"
#include "gpu.h"
#include "model.h"

#define SCREEN_W 256
#define SCREEN_H 240

// TODO: what do I do with this?
void printf(const char *, ...);

#define EMBED_BIN(s) \
    extern uint8_t _binary_bin_##s##_start[]; \
    extern uint32_t _binary_bin_##s##_size[];

#define EMBED_MODEL(s)    EMBED_BIN(s##_ply)

EMBED_MODEL(ball)
EMBED_MODEL(cube)

// TODO: models init function with X() macro

// TODO: move
Mat projection;

void draw_line(PrimBuf *pb, Veci a, Veci b, uint32_t color)
{
    uint32_t *prim = next_prim(pb, 3, 1);
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

typedef struct {
    Vec dir;  // will get normalized
    fx power;
    Vec color;
} Light;

// model matrix. should NOT scale
void update_llm(Light *lights, int n, Mat mat, PrimBuf *pb)
{
/*
    if (n < 3) {
         // only updating some of the lights
         // TODO

    }
    model.t = vec_zero();
*/
    //Vec lv = vec_scale(vec3_normalize(lights[0].dir), lights[0].power);

    Mat lm = {
        .v = { lights[0].dir, lights[1].dir },
    };

    // this is functionally the same as this:
    // Mat llm = mat_transpose(mat_mul(mat, mat_transpose(lm)));
    // because (AB^)^ == BA^
    Mat llm = mat_mul(lm, mat_transpose(mat));
    gte_loadLightMatrix(&llm.gte);
}


void draw_model(PrimBuf *pb, Model *m, fx angle_y, Vec pos)//Mat mat, Mat rmat)
{
    // calculate the local matrix (and it's rotational inverse) right here.
    //draw_vector(pb, (Vec3) { ONE/16, 0, 0 }, (Vec3) { ONE/32, 0, 0 });
    // wait, if we're updating the light matrix per model anyway...
    gte_loadLightColorMatrix(&(GTEMatrix){
        {{	0,	800,	0 },
         {	700,	0,	0 },
         {	0,	0,	0 }}
    //          ^       ^       ^
    //        Light1  Light2  Light3
    });

    Mat mat = mat_rotate_y(angle_y);
    mat.t = pos;

    Mat rmat = mat_rotate_y(fx_neg(angle_y));

    gte_setControlReg(GTE_RBK, 0.1*ONE);
    gte_setControlReg(GTE_GBK, 0.1*ONE);
    gte_setControlReg(GTE_BBK, 0.2*ONE);

    Light l[2] = {
        {
            .dir = { FX(-ONE), FX(-ONE), 0 },
            .power = 2.0*ONE,
        },

        {
            .dir = { FX(ONE), FX(-ONE), 0 },
            .power = 2.0*ONE,
        },
    };
    update_llm(&l, 2, rmat, pb);
    Mat mvp = mat_mul(projection, mat);

    Vec proj[m->nverts];
    //transform_ortho(proj, m->verts, m->nverts, &mvp);
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

Model *load_model(uint8_t *data)
{
    Model *m = alloc(sizeof(*m));
    void *p = model_new_ply(m, data);

    m->verts = alloc(sizeof(Vec) * m->nverts);
    m->normals = alloc(sizeof(Vec) * m->nverts);
    m->faces = alloc(sizeof(uint16_t) * m->nfaces * 3);

    model_read_data(m, p);
    return m;
}

int _start()
{
    gte_init();

    // TODO: move these to gpu_init
    GPU_GP1 = gp1_resetGPU();
    GPU_GP1 = gp1_dmaRequestMode(GP1_DREQ_GP0_WRITE);
    GPU_GP1 = gp1_dispBlank(false);
    GPU_GP1 = gp1_fbMode(GP1_HRES_256, GP1_VRES_256, GP1_MODE_NTSC, false, GP1_COLOR_16BPP);
    GPU_GP0 = gp0_fbMask(false, false);
    GPU_GP0 = gp0_fbOffset1(0, 0);
    GPU_GP0 = gp0_fbOffset2(1023, 511);
    GPU_GP0 = gp0_fbOrigin(SCREEN_W / 2, SCREEN_H / 2);
    gpu_sync();

    Model *cube = load_model(_binary_bin_cube_ply_start);
    Model *ball = load_model(_binary_bin_ball_ply_start);

    // orthographic for now. only scale to screen
    projection = mat_mul(
        mat_scale(FX(ONE/10), FX(ONE/10), FX(ONE)),
        mat_rotate_x(FX(ONE/12))
    );
    fx angle = FX(0);
    PrimBuf *pb = gpu_init();
    for (;;) {
        printf("Frame %d\n", frame);
        angle = fx_add(angle, FX(3));

        Vec pos = { FX(800), FX(0), FX(200) };
        draw_model(pb, cube, angle, pos);

        pos = (Vec) { FX(0), FX(0), FX(0) };
        draw_model(pb, cube, angle, pos);

        pos = (Vec) { FX(-600), FX(0), FX(-100) };
        draw_model(pb, cube, fx_add(angle, FX(400)), pos);

        pos = (Vec) { FX(0), FX(0), FX(-1000) };
        draw_model(pb, ball, FX(0), pos);
        draw_axes(pb);
        pb = swap_buffer();
    }
}
