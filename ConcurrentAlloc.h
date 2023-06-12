#pragma once
//���ͷ�ļ���Ϊ�˴���̬��tls����
//ʵ�ʵ��������ڴ棬��������ÿ���߳��Լ�ȥ��ȡthread cache�Ķ��󣬻����ٷ�װһ��
//concurrent����������˼

#include"Common.h"
#include"ThreadCache.h"
#include"PageCache.h"
#include"ObjectPool.h"

//�����ɾ�̬�ģ��ڶ�ΰ���ʱ���������Կ��ܻ��ͻ
static void* ConcurrentAlloc(size_t size)//��tcmallc�У������ֱ�ӽ���tcmalloc��              ������Ǹ�����
{
	if (size > MAX_BYTES)//������ڴ泬����256kb,�����������
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
		//ͨ��tls��ÿ���߳������Ļ�ȡ�Լ�ר����ThreadCache����
		if (pTLSThreadCache == nullptr)//���û�����̣߳��Ǿ�new����һ�����߳�
		{
			static ObjectPool<ThreadCache> tcPool;
			pTLSThreadCache = tcPool.New();//new ThreadCache;//û��
	
		}


		return pTLSThreadCache->Allocate(size);//���������ڴ�Ķ���С��256kb��
	}
}



static void ConcurrentFree(void* ptr)//����ӿھ��������ͷ������ַ��          ----��������һ���ڴ涼�����ҳ��
{
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);//���ڴ��ĵ�ַ�����ҳ��
	size_t size = span->_objSize;
	if (size > MAX_BYTES)//Ҳ�����ͷŵ�ʱ�򳬹�256kb�����ǻ������Ļ���,�����ǻ�����
	{
		PageCache::GetInstance()->_pageMtx.lock();
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		PageCache::GetInstance()->_pageMtx.unlock();

	}
	else
	{
		assert(pTLSThreadCache);//�ߵ������ʱ��ÿ���߳̿϶������Լ���tls��
		pTLSThreadCache->Deallocate(ptr, size);
	}
}

















