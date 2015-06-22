/**
 * Heap consistency checker
 *
 */
#ifndef MMCHECK_H
#define MMCHECK_H
#include <sys/types.h>

/**
 * 检查所有block的头和脚的size和标志位是否一致
 */
int check_blocks(char *heap_listp);

/**
 * 遍历整个implicit free list，检查已分配的大小是否
 * 等于alloc_size
 */
int check_allocted(char *heap_listp, size_t alloc_size);

void check_macros();


#endif
