#include <registers.h>
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

void draw_model(PrimBuf *pb, Model *m, Mat mat)
{
    //draw_vector(pb, (Vec3) { ONE/16, 0, 0 }, (Vec3) { ONE/32, 0, 0 });
    // wait, if we're updating the light matrix per model anyway...

/*
    update_llm(&l, 1, mat, pb);
    Mat mvp = mat_multiply(projection, mat);
    Vec3 proj[m->nverts];
    transform_ortho(proj, m->verts, m->nverts, &mvp);
    for (int i = 0; i < m->nfaces; i++) {
        uint16_t *face = (*m->faces)[i];
        Vec3 v0 = proj[face[0]];
        Vec3 v1 = proj[face[1]];
        Vec3 v2 = proj[face[2]];
        Vec3 n[3] = {
            m->normals[face[0]],
            m->normals[face[1]],
            m->normals[face[2]],
        };
        uint32_t color[3] = {0};
        normal_to_color(color, n);
        draw_triangle(pb, v0, v1, v2,
            color[0],
            color[1],
            color[2]
        );
    }
*/
}

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

int _start()
{
    printf("Hi!\n");

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
    Vec verts[m.nverts];
    Vec normals[m.nverts];
    uint16_t faces[m.nfaces][3];
    m.verts = verts;
    m.normals = normals;
    m.faces = faces;

    int rc = model_read_data(&m, data);

    PrimBuf *pb = gpu_init();
    for (;;) {
        printf("Frame %d\n", frame);        
        //draw_model(pb, &m, NULL);
        pb = swap_buffer();
    }

    printf("%d\n", rc);
    for (;;);
}
