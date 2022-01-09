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


//�ڴ��(��С�飩
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
	int nID;//�ڴ���
	int nRef;//���ô���
	MemoryAlloc* pAlloc;//�����ڴ��
	MemoryBlock* pNext;//��һ��λ��
	//�Ƿ����ڴ����
	bool bPool;

private:
	//Ԥ��
	char c1;
	char c2;
	char c3;
};

//int nsize = sizeof(MemoryBlock); 32�ֽ�
//�ڴ��
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

	//�����ڴ�
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

		//�����ڴ�صĴ�С
		size_t realSize = _nSize + sizeof(MemoryBlock);
		size_t buffSize = realSize * _nBlockSize;
		//��ϵͳͬ������ڴ�
		_pBuf = (char*)malloc(buffSize);
		//��ʼ���ڴ��
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
	//�ڴ�ص�ַ
	char* _pBuf;
	//ͷ���ڴ浥Ԫ(δ�����õ��ڴ��ַ)
	MemoryBlock* _pHeader;

	//�ڴ浥Ԫ�Ĵ�С
	size_t _nSize;
	//�ڴ��ĸ���
	size_t _nBlockSize;

	std::mutex _mutex;

};

//�����ʼ��
template<size_t nSize,size_t nBlockSize>
class MemoryAlloctor :public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		size_t n = sizeof(void*);
		_nSize = (nSize /n) * n + (nSize % n ? n : 0); //���ֽڶ���
		_nBlockSize = nBlockSize;
	}
};

//�ڴ�ع�����
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
	//�����ڴ�
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
	//�ڴ��ӳ�������ʼ��
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
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE + 1]; //64 + 1���㶨λ��1-64
};

#endif // !_MemoryMgr_H_
