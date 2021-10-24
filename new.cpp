#include "new.h"
#include "stdint.h"

void *operator new(size_t, void *p) { return p; }
void *operator new[](size_t, void *p) { return p; }
void  operator delete  (void *, void *) { };
void  operator delete[](void *, void *) { };
