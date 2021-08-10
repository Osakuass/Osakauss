#include <libs/stdlib.h>
#include <kernel/log.h>
#include <kernel/mem/phys.h>

#define FRAMES_COUNT 0x800
#define BLOCK_SIZE 0x1000

static struct frame frames;

static u8 frames_buffer[0x800] = {0};

extern int end;

u64 physmem_base = (u64) &end;
block blocks_;

static void
emptyBlock(u32 addr){
    u32 block = (addr - physmem_base) / 0x1000;
    for (int x = frames.blocks[block].start; x < frames.blocks[block].end; x++){
        frames.frames[x] = 0xff;
    }
    frames.blocks[block].start = 0;
    frames.blocks[block].end = 0;
}
static void
fillBlock(block block){
    for (int x = block.start; x < block.end; x++){
        frames.frames[x] = 1;
    }
    frames.blocks[block.amount].start = block.start;
    frames.blocks[block.amount].end = block.end;
}
static block FindEmptyBlocks(u32 size){
    block empty = {0,0, .flag = 1};
    u32 selectedFramesSizes = 0;
    bool CurrentlyFine = false;
    u32 FramesSelected = 0;
    u32 rotations = 0;
    block blockList = {.start = -1, .end = 0, .realSize =0};
    for (int x = 0; x < frames.frameCount; x++){
        if (x == FRAMES_COUNT){
                return empty;
        }
        if (selectedFramesSizes < size){

            if (frames.frames[x] != 0xff){
                CurrentlyFine = true;
                FramesSelected++;
                selectedFramesSizes += BLOCK_SIZE;
                if (blockList.start == -1){
                    blockList.start = x;
                }
            }
            else{
                if (CurrentlyFine == true){
                    if (frames.frames[x-1] == 0xff){
                        frames.frames[x-1] = 0;

                        if (FramesSelected !=  0){
                            blockList.start = 0;
                        }
                    }
                    selectedFramesSizes -= BLOCK_SIZE;
                    CurrentlyFine = false;
                }
            }
        }
        else{
            break;
        }
        rotations++;
    }
    if (rotations == FRAMES_COUNT){
        return empty;
    }
    blockList.end = rotations;
    blockList.amount = FramesSelected;
    blockList.realSize = selectedFramesSizes;
    return blockList;
}
extern void
physmem_init(){
    memset(frames_buffer,0,sizeof(char));
    frames.frameCount = FRAMES_COUNT;
    frames.frames = frames_buffer;
    frames.blocks = &blocks_;
    if (physmem_base & 0xfff) physmem_base += 0x1000;
	physmem_base &= 0xfffff000;
}
extern u64
physmem_alloc(u64 size){
    block block =  FindEmptyBlocks(size);
    if ( block.flag == 1 || block.amount == 0 || block.start == -1){
        return NULL;
    }
    else{
        tracef("> Allocating [%d] blocks\n", block.amount)
        tracef("> Allocating [%d] bytes\n", block.realSize);
        tracef("> Start block offset: [%d]\n", block.start);
        tracef("> End block offset: [%d]\n", block.end)
        u64 addr = physmem_base + block.amount * 0x1000;
        tracef("> Allocated space addr [%d]\n", addr);
        fillBlock(block);
        return addr;
    }
}
extern void
physmem_free(u64 addr){
    tracef("> Freeing address [%d]\n", addr);
    emptyBlock(addr);
}
