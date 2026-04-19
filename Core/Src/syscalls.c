/**
 * Minimal newlib syscalls: _sbrk for heap (nano libc links sbrkr.o).
 * Heap grows upward from _end; must stay below _estack minus margin (stack grows down).
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

extern char _end;
extern char _estack;

#ifndef SYSCALLS_SBRK_STACK_MARGIN
#define SYSCALLS_SBRK_STACK_MARGIN 512u
#endif

void *_sbrk(ptrdiff_t incr)
{
    static uint8_t *heap_ptr;

    if (heap_ptr == NULL)
        heap_ptr = (uint8_t *)&_end;

    if (incr < 0)
    {
        const size_t shrink = (size_t)(-incr);
        if (shrink > (size_t)(heap_ptr - (uint8_t *)&_end))
        {
            errno = EINVAL;
            return (void *)-1;
        }
        heap_ptr -= shrink;
        return (void *)heap_ptr;
    }

    {
        const size_t grow = (size_t)incr;
        uint8_t *const new_top = heap_ptr + grow;

        if ((uintptr_t)new_top > (uintptr_t)&_estack - SYSCALLS_SBRK_STACK_MARGIN)
        {
            errno = ENOMEM;
            return (void *)-1;
        }

        {
            void *const prev = (void *)heap_ptr;
            heap_ptr = new_top;
            return prev;
        }
    }
}
