Simple contained heap manager which does alignment

static uint8_t buffer[1000];  // create some buffer to allocate data from
MEM32_SYSTEM mem;             // Memory system instance

/* setup the memory system to allocate the buffer with requested alignment */
mem32_sys_init(&mem, (uint32_t)& buffer[0], (uint32_t)&buffer[1000], 2);

/* allocate 256 bytes from buffer it will be 2 byte aligned */
void* p = mem32_sys_malloc(&mem, 256);

/* free the memory when you are done */
mem32_sys_free(&mem, p);
