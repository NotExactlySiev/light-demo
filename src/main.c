#include <registers.h>
#include <gpucmd.h>
//#include "math.h"
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

    draw_model(pb, &ball, ball_mat);
    printf("%d\n", rc);
    for (;;);
}
