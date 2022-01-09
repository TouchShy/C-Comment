#include "Alloctor.h"
#include "MemoryMgr.hpp"

void* operator new(size_t nsize)
{
	return MemoryMgr::Instance().alloc(nsize);
}

void operator delete(void* p)
{
	MemoryMgr::Instance().freeMem(p);
}

void* operator new[](size_t nsize)
{
	return MemoryMgr::Instance().alloc(nsize);
}

void operator delete [](void* p)
{
	MemoryMgr::Instance().freeMem(p);
}

void* mem_malloc(size_t nsize)
{
	return MemoryMgr::Instance().alloc(nsize);
}

void mem_free(void* p)
{
	MemoryMgr::Instance().freeMem(p);
}