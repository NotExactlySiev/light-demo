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

typedef enum {
    LIGHT_NONE,
    LIGHT_SUN,
    LIGHT_POINT,
} LightKind;

typedef struct {
    Vec vec;  // direction or position
    Vec color;
    fx power;
    LightKind kind;
} Light;

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


void draw_model(PrimBuf *pb, Model *m, fx angle_y, Vec pos)
{
    // calculate the local matrix (and it's rotational inverse) right here.
    //draw_vector(pb, (Vec3) { ONE/16, 0, 0 }, (Vec3) { ONE/32, 0, 0 });
    // wait, if we're updating the light matrix per model anyway...
    gte_loadLightColorMatrix(&(GTEMatrix){
        {{	1600,	0,	0 },
         {	1600,	600,	0 },
         {	1600,	0,	0 }}
    //          ^       ^       ^
    //        Light1  Light2  Light3
    });

    Mat mat = mat_rotate_y(angle_y);
    mat.t = pos;

    Mat rmat = mat_rotate_y(fx_neg(angle_y));

    gte_setControlReg(GTE_RBK, 0.1*ONE);
    gte_setControlReg(GTE_GBK, 0.1*ONE);
    gte_setControlReg(GTE_BBK, 0.2*ONE);

    // point light position

    static fx t = {0};
    t = fx_add(t, FX(13));
    fx y = fx_mul(fx_sin(t), FX(ONE/3));
    // this comes from outside
    Light lights[3] = {
        {
            .kind = LIGHT_POINT,
            .vec = { FX(0), y, FX(0) },
            .power = FX(600),
        },
        {
            .kind = LIGHT_SUN, 
            .vec = { FX(ONE), FX(-ONE), 0 },
            //.power = FX(ONE/8),
        },
        {
            .kind = LIGHT_NONE,
        },
    };

    Vec lv[3] = {0};
    for (int i = 0; i < 3; i++) {
        Light *l = &lights[i];
        if (l->kind == LIGHT_POINT) {
            // mag2 should probably return int32
            // this should be done at the object routine (it receives the lights that need updating for it)
            // 32-bit calculation
            // TODO: use proper fx32 types
            Vec d = vec_sub(l->vec, pos);
            int32_t factor = (l->power.v * ONE) / ((d.x.v * d.x.v + d.y.v * d.y.v + d.z.v * d.z.v) / ONE);
            lv[i] = vec_scale(d, FX(factor));
        } else if (l->kind == LIGHT_SUN) {
            // TODO: normalize and use power
            lv[i] = l->vec;
            //lv[i] = vec_scale(vec_normalize_fake(lights[0].dir), lights[0].power);
        }
    }

    update_llm(lv, rmat);
    Mat mvp = mat_mul(projection, mat);

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

    Model *cube  = load_model(_binary_bin_cube_ply_start);
    Model *ball  = load_model(_binary_bin_ball_ply_start);
    Model *monke = load_model(_binary_bin_monke_ply_start);

    // orthographic for now. only scale to screen
    projection = mat_mul(mat_scale(FX(ONE/10), FX(ONE/10), FX(ONE)),
                 mat_mul(mat_rotate_x(FX(ONE/12)),
                         mat_rotate_y(FX(ONE/16))));
    fx angle = FX(0);
    PrimBuf *pb = gpu_init();
    for (;;) {
        printf("Frame %d\n", frame);
        angle = fx_add(angle, FX(21));

        Vec pos; 
        pos = (Vec) { FX(800), FX(0), FX(200) };
        draw_model(pb, cube, angle, pos);

        pos = (Vec) { FX(-700), FX(0), FX(-300) };
        //draw_model(pb, cube, angle, pos);

        pos = (Vec) { FX(-100), FX(0), FX(550) };
        draw_model(pb, ball, FX(0), pos);

        pos = (Vec) { FX(-800), FX(0), FX(-300) };
        draw_model(pb, monke, fx_add(fx_mul(angle, FX(ONE/3)), FX(-ONE/8)), pos);

        draw_axes(pb);
        pb = swap_buffer();
    }
}
