// a few allocations in multiples of 4 bytes checked for alignment
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include "myHeap.h"

int main() {
    assert(myInit(4096) == 0);
    int* ptr[4];

    ptr[0] = (int*) myAlloc(4);
    dispMem();
    ptr[1] = (int*) myAlloc(8);
    dispMem();
    ptr[2] = (int*) myAlloc(16);
    dispMem();
    ptr[3] = (int*) myAlloc(24);
    dispMem();

    for (int i = 0; i < 4; i++) {
        assert(ptr[i] != NULL);
    }

    for (int i = 0; i < 4; i++) {
        assert(((int)ptr[i]) % 8 == 0);
    }

    exit(0);
}
