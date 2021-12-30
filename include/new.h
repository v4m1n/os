#pragma once
#include "stddef.h"

void *operator new(size_t, void *p);
void *operator new[](size_t, void *p);
void  operator delete  (void *, void *);
void  operator delete[](void *, void *);

