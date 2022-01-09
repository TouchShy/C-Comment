#ifndef _MemoryMgr_H_
#define _MemoryMgr_H_
#include <stdlib.h>
#include <assert.h>
#include <mutex>
#ifdef _DEBUG
	#include<stdio.h>
	#define xPrintf(...) printf( __VA_ARGS__)
#else
	#define xPrintf(...)
#endif // _DEBUG


//内存块(最小块）
#define MAX_MEMORY_SIZE 1024
class MemoryAlloc;
class MemoryBlock
{
public:
	MemoryBlock()
	{

	}
	~MemoryBlock()
	{

	}

public:
	int nID;//内存编号
	int nRef;//引用次数
	MemoryAlloc* pAlloc;//所属内存块
	MemoryBlock* pNext;//下一块位置
	//是否在内存池中
	bool bPool;

private:
	//预留
	char c1;
	char c2;
	char c3;
};

//int nsize = sizeof(MemoryBlock); 32字节
//内存池
class MemoryAlloc
{
public:
	MemoryAlloc()
	{
		_pBuf = nullptr;
		_pHeader = nullptr;
		_nSize = 0;
		_nBlockSize = 0;
	}

	~MemoryAlloc()
	{
		if (_pBuf)
		{
			free(_pBuf);
			_pBuf = nullptr;
		}
	}

	//申请内存
	void* allocMemory(size_t nSize)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_pBuf)
		{
			initMemory();
		}
		MemoryBlock* pReturn = nullptr;
		if (nullptr == _pHeader)
		{
			pReturn = (MemoryBlock*)malloc(nSize+sizeof(MemoryBlock));
			if (pReturn)
			{
				pReturn->bPool = false;
				pReturn->nID = -1;
				pReturn->nRef = 1;
				pReturn->pAlloc = nullptr;
				pReturn->pNext = nullptr;
			}
		}
		else {
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;

			assert(0 == pReturn->nRef);
			pReturn->nRef = 1;
		}
		xPrintf("allocMem:%p,id=%d,size=%llu\n", pReturn, pReturn->nID, nSize);
		return ((char*)pReturn + sizeof(MemoryBlock));
	}

	void freeMemory(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		assert(1 == pBlock->nRef);
		if (pBlock->bPool)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (--pBlock->nRef != 0)
			{
				return;
			}
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else
		{
			if (--pBlock->nRef != 0)
			{
				return;
			}
			free(pBlock);
		}
		return;
	}

	//init
	void initMemory()
	{
		xPrintf("initMemory---->\n");
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;

		//计算内存池的大小
		size_t realSize = _nSize + sizeof(MemoryBlock);
		size_t buffSize = realSize * _nBlockSize;
		//向系统同申请池内存
		_pBuf = (char*)malloc(buffSize);
		//初始化内存池
		_pHeader = (MemoryBlock*)_pBuf;
		if (_pHeader)
		{
			_pHeader->bPool = true;
			_pHeader->nID = 0;
			_pHeader->nRef = 0;
			_pHeader->pAlloc = this;
			_pHeader->pNext = nullptr;
		}

		MemoryBlock* pHeader = _pHeader;
		for (size_t n = 1; n < _nBlockSize; n++)
		{
			MemoryBlock* pTemp = (MemoryBlock*)(_pBuf + (n * realSize));
			if (pTemp && pHeader)
			{
				pTemp->bPool = true;
				pTemp->nID = n;
				pTemp->nRef = 0;
				pTemp->pAlloc = this;
				pTemp->pNext = nullptr;

				pHeader->pNext = pTemp;

				pHeader = pTemp;
			}
		}
	}
protected:
	//内存池地址
	char* _pBuf;
	//头部内存单元(未被引用的内存地址)
	MemoryBlock* _pHeader;

	//内存单元的大小
	size_t _nSize;
	//内存块的个数
	size_t _nBlockSize;

	std::mutex _mutex;

};

//方便初始化
template<size_t nSize,size_t nBlockSize>
class MemoryAlloctor :public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		size_t n = sizeof(void*);
		_nSize = (nSize /n) * n + (nSize % n ? n : 0); //按字节对齐
		_nBlockSize = nBlockSize;
	}
};

//内存池管理工具
class MemoryMgr
{
public:
	static MemoryMgr& Instance()
	{
		static MemoryMgr mgr;
		return mgr;
	}
private:
	MemoryMgr()
	{
		xPrintf("MemoryMgr---->\n");
		init_szAlloc(0, 64, &_mem64);
		init_szAlloc(65, 128, &_mem128);
		init_szAlloc(129, 256, &_mem256);
		init_szAlloc(257, 512, &_mem512);
		init_szAlloc(513, 1024, &_mem1024);
	}

public:
	~MemoryMgr()
	{

	}
	//申请内存
	void* alloc(size_t nSize)
	{
		if (nSize <= MAX_MEMORY_SIZE)
		{
			return _szAlloc[nSize]->allocMemory(nSize);
		}
		else
		{
			MemoryBlock* pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			if (pReturn)
			{
				pReturn->bPool = false;
				pReturn->nID = -1;
				pReturn->nRef = 1;
				pReturn->pAlloc = nullptr;
				pReturn->pNext = nullptr;
				xPrintf("allocMem:%p,id=%d,size=%llu\n",pReturn, pReturn->nID, nSize);
				return (char*)pReturn + sizeof(MemoryBlock);
			}
		}
		return nullptr;
	}

	void freeMem(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		xPrintf("freeMem:%p,id=%d\n", pBlock, pBlock->nID);
		if (pBlock->bPool)
		{
			pBlock->pAlloc->freeMemory(pMem);
		}
		else {
			if (--pBlock->nRef == 0)
			{
				free(pBlock);
			}
		}
	}

	void addRef(void* pMem)
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		if (pBlock)
		{
			++pBlock->nRef;
		}
	}
private:
	//内存池映射数组初始化
	void init_szAlloc(int nbegin,int nEnd, MemoryAlloc* pMem)
	{
		for (int n = nbegin; n <= nEnd; n++)
		{
			_szAlloc[n] = pMem;
		}
	}
private:
	MemoryAlloctor<64,100000> _mem64;
	MemoryAlloctor<128, 100000> _mem128;
	MemoryAlloctor<256, 100000> _mem256;
	MemoryAlloctor<512, 100000> _mem512;
	MemoryAlloctor<1024, 100000> _mem1024;
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE + 1]; //64 + 1方便定位到1-64
};

#endif // !_MemoryMgr_H_
