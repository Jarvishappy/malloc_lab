/**
 * Demo comparison of void* pointer
 * K&R中关于pointer arithmetic的论述：
 *
 *
 */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
    int *a = malloc(10 * sizeof(int));
    void *p = a + 1;
    void *q = a + 3;

    /* compare two void* pointer */
    printf("p=%p, q=%p\n", p, q);
    if (p < q) {
        printf("p less than q\n");
        printf("q - p = %ld\n", q - p);
    } else {
        printf("p greater or equal to q\n");
    }

    free(a);
    return 0;
}


