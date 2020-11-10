// second allocation is too big to fit
#include <assert.h>
#include <stdlib.h>
#include "myHeap.h"

int main() {
   assert(myInit(4096) == 0);
   dispMem();
   assert(myAlloc(2048) != NULL);
   dispMem();
   assert(myAlloc(2047) == NULL);
   dispMem();
   exit(0);
}
