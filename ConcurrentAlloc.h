#pragma once
//这个头文件是为了处理静态的tls问题
//实际当中申请内存，不可能让每个线程自己去获取thread cache的对象，还得再封装一层
//concurrent：并发的意思

#include"Common.h"
#include"ThreadCache.h"
#include"PageCache.h"
#include"ObjectPool.h"

//不给成静态的，在多次包含时，链接属性可能会冲突
static void* ConcurrentAlloc(size_t size)//在tcmallc中，这里就直接叫做tcmalloc了              申请的是个函数
{
	if (size > MAX_BYTES)//申请的内存超过了256kb,会有两种情况
	{
		size_t alignSize = SizeClass::RoundUp(size);
		size_t kpage = alignSize >> PAGE_SHIFT;

		PageCache::GetInstance()->_pageMtx.lock();
		Span* span=PageCache::GetInstance()->NewSpan(kpage);
		span->_objSize = size;
		PageCache::GetInstance()->_pageMtx.unlock();

		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		return ptr;
	}
	else
	{
		//通过tls，每个线程无锁的获取自己专属的ThreadCache对象
		if (pTLSThreadCache == nullptr)//如果没有子线程，那就new出来一个子线程
		{
			static ObjectPool<ThreadCache> tcPool;
			pTLSThreadCache = tcPool.New();//new ThreadCache;//没有
	
		}


		return pTLSThreadCache->Allocate(size);//在这申请内存的都是小于256kb的
	}
}



static void ConcurrentFree(void* ptr)//这个接口就是用来释放这个地址的          ----给我任意一块内存都能算出页号
{
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);//用内存块的地址能算出页号
	size_t size = span->_objSize;
	if (size > MAX_BYTES)//也就是释放的时候超过256kb可能是还给中心缓存,可能是还给堆
	{
		PageCache::GetInstance()->_pageMtx.lock();
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		PageCache::GetInstance()->_pageMtx.unlock();

	}
	else
	{
		assert(pTLSThreadCache);//走到这里的时候，每个线程肯定是有自己的tls的
		pTLSThreadCache->Deallocate(ptr, size);
	}
}

















