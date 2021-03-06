#include "vram.h"
#include <gpu.h>
#include <dma.hpp>
#include <io.h>
#include "log.h"

void beginCpuToVramTransfer(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    CPU2VRAM buf = {0};
    setcode(&buf, 0xA0); // CPU -> VRAM
    setlen(&buf, 3);
    setXY0(&buf, x, y);
    setWH(&buf, w, h);
    DrawPrim(&buf);
}

void copyToVram(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t* data) {
    LOG("copyToVram(x=%3d, y=%3d, w=%3d, h=%3d, data=0x%08x)... ", x, y, w, h, data);
    
    beginCpuToVramTransfer(x, y, w, h);
    
    volatile uint32_t *GP0 = (uint32_t*)0x1f801810;
    for (int y = 0; y<h; y++) {
        for (int x = 0; x<w; x++) {
            *GP0 = *data++;
        }   
    }

    LOG("ok\n");
}

void copyToVramDma(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t* data) {
    LOG("copyToVramDma(x=%3d, y=%3d, w=%3d, h=%3d, data=0x%08x)... ", x, y, w, h, data);
    beginCpuToVramTransfer(x, y, w, h);
    
    using namespace DMA;
    auto addr = MADDR((uint32_t)data);
    auto block = BCR::mode1(0x10, w * h / 0x10 / 2);
    auto control = CHCR::VRAMwrite();

    writeGP1(4, 2); // DMA Direction - CPU -> VRAM
    masterEnable(Channel::GPU, true);
    waitForChannel(Channel::GPU);
    
    write32(CH_BASE_ADDR    + 0x10 * (int)DMA::Channel::GPU, addr._reg);
    write32(CH_BLOCK_ADDR   + 0x10 * (int)DMA::Channel::GPU, block._reg);
    write32(CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::GPU, control._reg);

    waitForChannel(Channel::GPU);

    LOG("ok\n");
}
