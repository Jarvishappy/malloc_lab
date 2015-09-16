/*
 * mm-naive.c - The implicit free list malloc package.
 *
 * 内存块结构图:
 *                         31               0
 *                          .---------------. _
 *                          | block size|00A|  \
 *    block pointer ------> .---------------.   |
 *                          |    payload    |   |
 *                          .---------------. block size
 *                          |    padding    |   |
 *                          .---------------.   |
 *                          | block size|00A|   .
 *                          .---------------._ /
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "StrikeW",
    /* First member's full name */
    "Siyuan",
    /* First member's email address */
    "wangsiy3@sysu.mail3.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of 8 */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

static char* heap_listp; /* 指向序言块 */

/**
 * malloc时寻找一个匹配的空闲块，first fit策略
 */
static void *find_fit(size_t size);

/**
 * 放置分配块
 */
static void place(void *bp, size_t asize);

static void *extend_heap(size_t words);
/**
 * 合并空闲块, 立即合并
 */
static void *coalesce(void *bp);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* 初始化heap */
    mem_init();
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*) -1) {
        fprintf(stderr, "initialize memory allocator fail\n");
        return -1;
    }

    PUT(heap_listp, 0); /* 第一个块不使用，仅置为0 */
    PUT(heap_listp + WSIZE, PACK(2 * WSIZE, 1));  /* header for 序言块  */
    PUT(heap_listp + 2 * WSIZE, PACK(2 * WSIZE, 1)); /* footer for 序言块 */
    PUT(heap_listp + 3 * WSIZE, PACK(0, 1));     /* 结语块(0,1) */

    heap_listp += 2 * WSIZE;  /* 序言块的payload为0, 所以block pointer指向的就是footer */


    if (!extend_heap(CHUNKSIZE / WSIZE))
        return -1;

    return 0;
}

/*
 * mm_malloc - Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if (heap_listp == 0)
        mm_init();

    if (size == 0)
        return NULL;


    /* actual size = payload + header + footer + padding */
    size_t asize = ALIGN(size + DSIZE);
    void *bp;
    size_t expandsize;
    if ((bp = find_fit(asize)) != NULL) {
    } else {
        /* cannot find fit block in free list, ask kernel for more vm */
        expandsize = MAX(asize, CHUNKSIZE);
        if ((bp = extend_heap(expandsize / WSIZE)) == NULL)
            return NULL;
    }

    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    char *bp = ptr;
    size_t size;
    if (heap_listp < bp && bp < (char*)mem_heap_hi()) {
        if (GET_ALLOC(HDRP(bp))) {
            size = GET_SIZE(HDRP(bp));
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));
            coalesce(bp);
        }
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 * 重新调整之前malloc分配的block的大小
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);
    else if (0 == size) {
        mm_free(ptr);
        return NULL;
    }

    void *new_bp = NULL;
    size_t old_size = GET_SIZE(HDRP(ptr));
    size_t new_size = ALIGN(size);
    size_t frag_size;
    /* if ALIGN(size) > old_size */
    if (new_size > old_size) {

        /* 这个IF中的逻辑感觉多余了，只需要通过malloc()找到一个更大的空闲块就行了 */
        if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) &&
                GET_SIZE(HDRP(NEXT_BLKP(ptr))) >= (new_size - old_size)) {
            /* next block is large enough, ptr not need to change */
            frag_size = GET_SIZE(HDRP(NEXT_BLKP(ptr))) - (new_size - old_size);
            if (frag_size >= MIN_BLK) {
                PUT(HDRP(ptr), PACK(new_size, 1));
                PUT(FTRP(ptr), PACK(new_size, 1));

                PUT(HDRP(NEXT_BLKP(ptr)), PACK(frag_size, 0));
                PUT(FTRP(NEXT_BLKP(ptr)), PACK(frag_size, 0));
            } else {
                new_size = old_size + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
                PUT(HDRP(ptr), PACK(new_size, 1));
                PUT(FTRP(ptr), PACK(new_size, 1));
            }

            return ptr;
        } else {
            /* next block isn't large enough, ptr need to be pointed to a large region */
            if ((new_bp = mm_malloc(new_size)) != NULL) {
                PUT(HDRP(new_bp), PACK(new_size, 1));
                PUT(FTRP(new_bp), PACK(new_size, 1));

                /* copy payload from old block to new block */
                memcpy(new_bp, ptr, GET_SIZE(HDRP(ptr)) - DSIZE);

                return new_bp;
            }

        }
    } else if (new_size < old_size) {
        /* if ALIGN(size) < old_size, check if need to split */
        if (old_size - new_size >= MIN_BLK) {
            PUT(HDRP(ptr), PACK(new_size, 1));
            PUT(FTRP(ptr), PACK(new_size, 1));
            /* split a new free block */
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(old_size - new_size, 0));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(old_size - new_size, 0));
        }

        return ptr;
    }

    return ptr;
}

/**
 * 向kernel申请更多的vm来扩展heap，并把新申请到的vm初始化成free block
 */
static void *extend_heap(size_t words)
{
    size_t size = (words % 2 == 0) ? words * WSIZE : (words + 1) * WSIZE;
    void *bp;

    if ((bp = mem_sbrk(size)) == (void*) -1) {
        return NULL;
    } else {
        /* 初始化free block */
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* 新的结尾块 */
        return coalesce(bp);
    }
}

/**
 * 把左右两边的空闲块合并成一个大的空闲块,
 * 一共有四种情况
 *
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC((char*)bp - DSIZE);
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* 左右两侧的Block都已经分配 */
    if (prev_alloc && next_alloc ) {
    }
    else if (!prev_alloc && next_alloc ) {
        /* 左侧的Block未分配 */
        size += GET_SIZE((char*)bp - DSIZE);
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        /* prev block的footer并没被修改，因此PREV_BLKP能正确获取到prev block的地址 */
        bp = PREV_BLKP(bp);

    } else if (prev_alloc && !next_alloc ) {
        /* 右侧的Block未分配 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        PUT(HDRP(bp), PACK(size, 0));

    } else {
        /* 左右两侧的Block都未分配 */
        size += GET_SIZE((char*)bp - DSIZE) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

        /* prev block的footer并没被修改，因此PREV_BLKP能正确获取到prev block的地址 */
        bp = PREV_BLKP(bp);
    }

    return bp;
}

/**
 * 在free list中寻找block size大于等于size的block
 */
static void *find_fit(size_t size)
{
    char *bp;

    /* 遍历到结尾块，结尾块的size为0 */
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0 ; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= size)
            return bp;
    }

    return NULL;
}

/**
 * 将请求块放置在空闲块的起始位置，当剩余部分大于等于最小块的大小时进行分割操作
 *
 * @param bp 指向free list中一个size大于asize字节的block
 */
static void place(void *bp, size_t asize)
{
    size_t bsize = GET_SIZE(HDRP(bp));
    size_t diff_size = bsize - asize;

    if (diff_size >= MIN_BLK) {
        /* 请求块防止在空闲块的起始位置 */
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1)); /* 此处FTRP的GET_SIZE取到的是上面的asize */

        /* 多余的部分进行分割 */
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(diff_size, 0));
        PUT(FTRP(bp), PACK(diff_size, 0));

    } else {
        PUT(HDRP(bp), PACK(bsize, 1));
        PUT(FTRP(bp), PACK(bsize, 1));
    }
}



