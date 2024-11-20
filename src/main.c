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

EMBED_MODEL(model)

// TODO: models init function with X() macro

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

void draw_triangle(PrimBuf *pb, Veci v0, Veci v1, Veci v2, uint32_t col0, uint32_t col1, uint32_t col2)
{
    int z = 0;
    if (v0.z > z) z = v0.z;
    if (v1.z > z) z = v1.z;
    if (v2.z > z) z = v2.z;
    // TODO: don't harcode this transform
    z = (z + 400) / 32;
    //int z = (v0.z + v1.z + v2.z) / 3;
    uint32_t *prim = next_prim(pb, 6, z);
    *prim++ = (col0 & 0xFFFFFF) | _gp0_polygon(false, false, true, false, false);
    *prim++ = gp0_xy(v0.x, v0.y);
    *prim++ = col1 & 0xFFFFFF;
    *prim++ = gp0_xy(v1.x, v1.y);
    *prim++ = col2 & 0xFFFFFF;
    *prim++ = gp0_xy(v2.x, v2.y);
}

// TODO: move
Mat projection;

void normal_to_color(uint32_t *out, Vec *n)
{
    // TODO: do this once per vertex
    // TODO: have the lights in a light structure. then read into
    // gte matrix and transpose the colors
    // doing one light for now

    gte_setV0(n[0].x.v, n[0].y.v, n[0].z.v);
  //  gte_loadV0(&n[0]);
  //  gte_loadV1(&n[1]);
  //  gte_loadV2(&n[2]);
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
    GTEMatrix llm = {0};
/*
    if (n < 3) {
         // only updating some of the lights
         // TODO

    }
    model.t = vec_zero();

*/
    //Vec lv = vec_scale(vec3_normalize(lights[0].dir), lights[0].power);
    Vec lv = lights[0].dir;
    //Vec llv = mat_vec_multiply(lv, mat);
    Vec llv = lv;

    //draw_vector(pb, VEC3_ZERO, llv);
    //VPRINT(llv);
    llm.values[0][0] = llv.x.v;
    llm.values[0][1] = llv.y.v;
    llm.values[0][2] = llv.z.v;

    //llm = mat_multiply(llm, model);
    // TODO: transform by model
    gte_loadLightMatrix(&llm);
}


void draw_model(PrimBuf *pb, Model *m, Mat mat)
{
    //draw_vector(pb, (Vec3) { ONE/16, 0, 0 }, (Vec3) { ONE/32, 0, 0 });
    // wait, if we're updating the light matrix per model anyway...
gte_loadLightColorMatrix(&(GTEMatrix){
            {{	0,	0,	0 },
             {	600,	0,	0 },
             {	0,	0,	0 }}
        //          ^       ^       ^
        //        Light1  Light2  Light3
        });

        gte_setControlReg(GTE_RBK, 0.1*ONE);
        gte_setControlReg(GTE_GBK, 0.1*ONE);
        gte_setControlReg(GTE_BBK, 0.2*ONE);

Light l = {
    .dir = { FX(-ONE), FX(ONE), 0 },
    .power = 2.0*ONE,
};
    update_llm(&l, 1, mat, pb);
    Mat mvp = mat_mul(projection, mat);
    Vec proj[m->nverts];
    //transform_ortho(proj, m->verts, m->nverts, &mvp);
    transform_vecs(proj, m->verts, m->nverts, mvp);

    for (int i = 0; i < m->nfaces; i++) {
        //printf("%d\n", v.z.v + 400);
        uint16_t *face = (*m->faces)[i];
        Veci v0 = vec_raw(proj[face[0]]);
        Veci v1 = vec_raw(proj[face[1]]);
        Veci v2 = vec_raw(proj[face[2]]);
        Vec n[3] = {
            m->normals[face[0]],
            m->normals[face[1]],
            m->normals[face[2]],
        };
        uint32_t color[3] = { 255 };
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

    // TODO: probably sync here

    GPU_GP0 = gp0_rgb(255, 0, 0) | _gp0_polygon(false, false, true, false, false);
    GPU_GP0 = gp0_xy(0, -40);
    GPU_GP0 = gp0_rgb(0, 255, 0);
    GPU_GP0 = gp0_xy(-40, 40);
    GPU_GP0 = gp0_rgb(0, 0, 255);
    GPU_GP0 = gp0_xy(40, 40);

    Model m;
    void *data = model_new_ply(&m, _binary_bin_model_ply_start);

    // allocate the space
    // TODO: allocate in an arena
    Vec verts[m.nverts];
    Vec normals[m.nverts];
    uint16_t faces[m.nfaces][3];
    m.verts = verts;
    m.normals = normals;
    m.faces = faces;

    int rc = model_read_data(&m, data);

    projection = mat_mul(
        mat_scale(FX(ONE/6), FX(ONE/6), FX(ONE)),
        mat_rotate_x(FX(ONE/12))
    );
    fx angle = { 0 };
    PrimBuf *pb = gpu_init();
    for (;;) {
        printf("Frame %d\n", frame);
        Mat mat = mat_rotate_y(angle);
        angle = fx_add(angle, FX(1));
        draw_model(pb, &m, mat);
        pb = swap_buffer();
    }
}
