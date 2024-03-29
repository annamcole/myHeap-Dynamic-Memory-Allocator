// many odd sized allocations and interspersed frees
#include <assert.h>
#include <stdlib.h>
#include "myHeap.h"

int main() {
   assert(myInit(4096) == 0);
   void * ptr[9];
   ptr[0] = myAlloc(1);
   ptr[1] = (myAlloc(5));
   ptr[2] = (myAlloc(8));
   ptr[3] = (myAlloc(14));
   dispMem();
   assert(ptr[0] != NULL);
   assert(ptr[1] != NULL);
   assert(ptr[2] != NULL);
   assert(ptr[3] != NULL);
   
   assert(myFree(ptr[1]) == 0);
   dispMem();
   assert(myFree(ptr[0]) == 0);
   dispMem();
   assert(myFree(ptr[3]) == 0);
   dispMem();
   
   ptr[4] = (myAlloc(1));
   ptr[5] = (myAlloc(4));
   dispMem();
   assert(ptr[4] != NULL);
   assert(ptr[5] != NULL);
   assert(myFree(ptr[5]) == 0);
   dispMem();
   
   ptr[6] = (myAlloc(9));
   ptr[7] = (myAlloc(33));
   dispMem();
   assert(ptr[6] != NULL);
   assert(ptr[7] != NULL);
   
   assert(myFree(ptr[4]) == 0);
   dispMem();

   ptr[8] = (myAlloc(55));
   assert(ptr[8] != NULL);
   dispMem();

   assert(myFree(ptr[2]) == 0);
   dispMem();
   assert(myFree(ptr[7]) == 0);
   dispMem();
   assert(myFree(ptr[8]) == 0);
   dispMem();
   assert(myFree(ptr[6]) == 0);
   dispMem();

   exit(0);
}
