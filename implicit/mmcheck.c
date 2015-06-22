#include "mmcheck.h"
#include "mm.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/**
 * 检查所有block的头和脚的size和标志位是否一致
 */
int check_blocks(char *heap_listp)
{
    if (NULL == heap_listp)
        return 0;

    char *bp = heap_listp;
    size_t size;
    size_t alloc;
    while (1) {
        size = GET_SIZE(HDRP(bp));
        alloc = GET_ALLOC(HDRP(bp));
        /* 结尾块 */
        if (0 == size) {
            if (1 != alloc)
                return 0;
            break;
        }

        if (size != GET_SIZE(FTRP(bp)) || alloc != GET_ALLOC(FTRP(bp)))
            return 0;

        bp = NEXT_BLKP(bp);
    }

    return 1;
}


/**
 * 遍历整个implicit free list，检查已分配的大小是否
 * 等于alloc_size
 */
int check_allocted(char *heap_listp, size_t alloc_size)
{
    if (NULL == heap_listp)
        return 0;

    size_t total_alloc = 0;
    for (; GET_SIZE(HDRP(heap_listp)) > 0; heap_listp = NEXT_BLKP(heap_listp)) {
        if (GET_ALLOC(HDRP(heap_listp))) {
            total_alloc += GET_SIZE(HDRP(heap_listp));
        }
    }

    return total_alloc == alloc_size;
}

/**
 * 检查所有自定义的宏全部正确
 */
void check_macros()
{
    void *heap = malloc(10 * WSIZE);
    char *bp = heap;

    bp += 1 * WSIZE;

    assert ((char*)heap == HDRP(bp));
    assert (0x11 == PACK(16, 1));

    PUT(HDRP(bp), PACK(16, 1));

    assert (0x11 == GET(HDRP(bp)));
    assert (16 == GET_SIZE(HDRP(bp)));
    assert (1 == GET_ALLOC(HDRP(bp)));
    assert ((char*)heap + 3 * WSIZE == FTRP(bp));

    PUT(FTRP(bp), PACK(16, 1));

    assert (GET(HDRP(bp)) == GET(FTRP(bp)));
    assert ((char*)heap + 5 * WSIZE == NEXT_BLKP(bp));

    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(24, 0));
    PUT(FTRP(bp), PACK(24, 0));

    assert (GET(HDRP(bp)) == GET(FTRP(bp)));
    assert (24 == GET_SIZE(HDRP(bp)));
    assert (24 == GET_SIZE(FTRP(bp)));
    assert ((char*)heap + 1 * WSIZE == PREV_BLKP(bp));

    free(heap);
    printf("check_macros: passed\n");
}

