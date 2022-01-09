#ifndef _CellObjectPool_H_
#define _CellObjectPool_H_
#include <stdlib.h>
#include <assert.h>
#include <mutex>

#ifdef _DEBUG
	#ifndef xPrintf
		#include<stdio.h>
		#define xPrintf(...) printf( __VA_ARGS__)
	#endif
#else
	#ifndef xPrintf
		#define xPrintf(...)
	#endif
#endif // _DEBUG


template<class Type, size_t nPoolSize>
class CellObjectPool
{
public:
	CellObjectPool()
	{
		initPool();
	}

	~CellObjectPool()
	{
		if (_pBuf)
			delete[] _pBuf;
	}

	//申请对象
	void* alloObject(size_t nSize)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		NodeHeader* pReturn = nullptr;
		if (nullptr == _pHeader)
		{
			pReturn = (NodeHeader*)new char[sizeof(Type) + sizeof(NodeHeader)];
			if (pReturn)
			{
				pReturn->bPool = false;
				pReturn->nID = -1;
				pReturn->nRef = 1;
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
		return ((char*)pReturn + sizeof(NodeHeader));
	}

	void freeObject(void* obj)
	{
		NodeHeader* pObject = (NodeHeader*)((char*)obj - sizeof(NodeHeader));
		xPrintf("freeObject:%p,id=%d,size=%llu\n", pReturn, pReturn->nID, nSize);
		assert(1 == pObject->nRef);
		if (pObject->bPool)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			if (--pObject->nRef != 0)
			{
				return;
			}
			pObject->pNext = _pHeader;
			_pHeader = pObject;
		}
		else
		{
			if (--pObject->nRef != 0)
			{
				return;
			}
			delete[] pObject;
		}
		return;
	}
private:
	void initPool()
	{
		assert(nullptr == _pBuf);
		if (_pBuf)
			return;

		size_t realSize = sizeof(Type) + sizeof(NodeHeader);
		size_t nSize = nPoolSize * realSize;
		_pBuf = new char[nSize];

		_pHeader = (NodeHeader*)_pBuf;
		_pHeader->pNext = nullptr;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->bPool = true;

		NodeHeader* pHeader = _pHeader;
		for (size_t n = 1; n < nPoolSize; n++)
		{
			NodeHeader* pTemp = (NodeHeader*)(_pBuf + (n * realSize));
			if (pTemp && pHeader)
			{
				pTemp->bPool = true;
				pTemp->nID = n;
				pTemp->nRef = 0;
				pTemp->pNext = nullptr;

				pHeader->pNext = pTemp;
				pHeader = pTemp;
			}
		}
	}
private:
	class NodeHeader
	{
	public:
		NodeHeader* pNext;//下一块位置
		//是否在内存池中
		int nID;//内存编号
		char nRef;//引用次数
		bool bPool;

	private:
		//预留
		char c1;
		char c2;
	};
protected:
	//对象池地址
	char* _pBuf;
	NodeHeader* _pHeader;
	std::mutex _mutex;

};
template<class Type,size_t nPoolSize>
class CellObjectPoolBase
{
public:
	CellObjectPoolBase()
	{

	}

	~CellObjectPoolBase()
	{

	}

	void* operator new(size_t n)
	{
		return ObjectPool().alloObject(n);
	}

	void operator delete(void* p)
	{
		ObjectPool().freeObject(p);
	}
	
	template<typename ...Args>
	static Type* createObject(Args ... args)
	{
		Type* Obj = new Type(args...);
		return Obj;
	}

	static void destoryObject(Type* obj)
	{
		delete obj;
	}

private:
	typedef CellObjectPool<Type, nPoolSize> ClassTypePool;

	static ClassTypePool& ObjectPool()
	{
		static ClassTypePool obj;
		return obj;
	}
};

#endif // !_CellObjectPool_H_
