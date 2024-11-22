#include <stdint.h>
#include <registers.h>
#include <gpucmd.h>
#include "kernel.h"
#include "gpu.h"

// layers should be in the header
#define PRIM_NLAYERS    256
#define PRIM_SIZE       (1 << 16)

struct PrimBuf {
    uint32_t data[PRIM_SIZE];
    uint32_t layers[PRIM_NLAYERS];
    uint32_t *next;
};

volatile int frame = 0;
static PrimBuf primbufs[2];
static int curr = 0;
static int vsync_event = 0;


int enable_vblank_event(void* handler)
{
    int event;
    EnterCriticalSection();
    event = OpenEvent(RCntCNT3, EvSpINT, EvMdINTR, handler);    
    enable_timer_irq(3);
    EnableEvent(event);
    ExitCriticalSection();
    return event;
}

void disable_vblank_event(int event)
{
    EnterCriticalSection();
    disable_timer_irq(3);
    CloseEvent(event);
    ExitCriticalSection();
}

void vsync_handler(void)
{
    frame++;
}

static void clear_buffer(PrimBuf *pb)
{
    pb->next = &pb->data[0];
    for (int i = 1; i < PRIM_NLAYERS; i++) {
        pb->layers[i] = gp0_tag(0, &pb->layers[i-1]);
    }
    pb->layers[0] = gp0_endTag(0);
}

static void send_prims(uint32_t *data)
{
    while (DMA_CHCR(DMA_GPU) & DMA_CHCR_ENABLE);
    DMA_MADR(DMA_GPU) = (uint32_t) data;
    DMA_CHCR(DMA_GPU) = DMA_CHCR_WRITE | DMA_CHCR_MODE_LIST | DMA_CHCR_ENABLE;
}

// TODO: bring all the initialization stuff here from main
PrimBuf *gpu_init(void)
{
    GPU_GP1 = gp1_resetGPU();
    GPU_GP1 = gp1_dmaRequestMode(GP1_DREQ_GP0_WRITE);
    GPU_GP1 = gp1_dispBlank(false);
    GPU_GP1 = gp1_fbMode(GP1_HRES_256, GP1_VRES_256, GP1_MODE_NTSC, false, GP1_COLOR_16BPP);
    GPU_GP0 = gp0_fbMask(false, false);
    GPU_GP0 = gp0_fbOffset1(0, 0);
    GPU_GP0 = gp0_fbOffset2(1023, 511);
    GPU_GP0 = gp0_fbOrigin(SCREEN_W / 2, SCREEN_H / 2);
    gpu_sync();

    DMA_DPCR |= DMA_DPCR_ENABLE << (DMA_GPU * 4);
    curr = 0;
    clear_buffer(&primbufs[curr]);
    vsync_event = enable_vblank_event(vsync_handler);
    frame = 0;
    return &primbufs[curr];
}

// TODO: rename to gpu_alloc?
uint32_t *next_prim(PrimBuf *pb, int len, int layer)
{
    if (layer > PRIM_NLAYERS-1) {
        printf("layer clamped: %d\n", layer);
        layer = PRIM_NLAYERS-1;
    } else if (layer < 0) {
        printf("layer clamped: %d\n", layer);
        layer = 0;
    }
    uint32_t *prim = pb->next;
    prim[0] = gp0_tag(len, (void*) pb->layers[layer]);
    pb->layers[layer] = gp0_tag(0, prim);
    pb->next += len + 1;
    if (((uintptr_t) pb->next) >= ((uintptr_t) &pb->data[PRIM_SIZE])) {
        printf("too many primitives!\n");
    }
    return &prim[1];
}

// TODO: rename to gpu_swap
PrimBuf *swap_buffer(void)
{
    static int last_rendered = 0;

    uint32_t *prim;
    PrimBuf *pb = &primbufs[curr];
    
    int bufferX = curr * SCREEN_W;
    int bufferY = 0;
    prim = next_prim(pb, 4, PRIM_NLAYERS-1);
    prim[0] = gp0_texpage(0, true, false);
	prim[1] = gp0_fbOffset1(bufferX, bufferY);
	prim[2] = gp0_fbOffset2(bufferX + SCREEN_W - 1, bufferY + SCREEN_H - 2);
	prim[3] = gp0_fbOrigin(bufferX + SCREEN_W / 2, bufferY + SCREEN_H / 2);

    gpu_sync();    
    while (last_rendered == frame);
    last_rendered = frame;

    // add the block fill last, right before dma, so it's drawn first
    prim = next_prim(pb, 3, PRIM_NLAYERS-1);
    prim[0] = gp0_rgb(0, 0, 0) | gp0_vramFill();
    prim[1] = gp0_xy((curr) * SCREEN_W, 0);
    prim[2] = gp0_xy(SCREEN_W, SCREEN_H);
    send_prims(&pb->layers[PRIM_NLAYERS-1]);
    curr = 1 - curr;
    pb = &primbufs[curr];
    GPU_GP1 = gp1_fbOffset(curr * SCREEN_W, 0); 
    clear_buffer(&primbufs[curr]);
    return &primbufs[curr];
}

void gpu_sync(void)
{
    while (!(GPU_GP1 & GP1_STAT_CMD_READY));
}
