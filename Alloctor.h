#ifndef  _ALLOCTOR_H_
#define _ALLOCTOR_H_

void* operator new(size_t nsize);
void operator delete(void* p);

void* operator new[](size_t nsize);
void operator delete[](void* p);
void* mem_malloc(size_t nsize);
void mem_free(void* p);
#endif // ! _ALLOC_H_
