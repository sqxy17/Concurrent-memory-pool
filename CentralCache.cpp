#include"CentralCache.h"
#include"PageCache.h"

//单例模式的饿汉模式,是单例模式的实际方式之一,在这种模式下,单例对象在类加载时就被创建出来
//在饿汉模式中，单例对象在类加载时就被创建出来，而不是在第一次使用时才进行创建。因此，它在多线程环境下是线程安全的，不需要考虑竞争条件。
//具体实现方法通常是在类的定义中直接创建一个静态的、私有的单例对象，然后提供一个静态的公共方法来访问该对象。
//在饿汉模式中，单例对象的初始化是在程序启动时就完成的
CentralCache CentralCache::_sInst;




//找CentralCache要批量的内存的函数
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	//第一步先算要的是哪个桶里面的,但是CentralCache中对应的桶也有可能是空的,是空的或者不够,还得想下一层缓存中申请空间
	size_t index = SizeClass::Index(size);

	//因为有可能多个线程都在向中心缓存中的同一个桶中申请内存,那么就先把锁给加上
	_spanLists[index]._mtx.lock();//这就是对某个桶加了锁

	Span* span = GetOneSpan(_spanLists[index],size);
	assert(span);//假设要到了,要到就不能是空的
	assert(span->_freeList);

	//从这里开始直到函数结束,主要作用就是从span中获取batchNum个对象
	//如果不够batchNum个,有多少拿多少
	start = span->_freeList;
	end = start;

	size_t i = 0;
	size_t actualNum = 1;//实际获取了多少个
	while( i < batchNum-1 && NextObj(end)!=nullptr)//也就是说当sapn剩余的不够申请的大小的情况下,有多少给多少
	{
		end = NextObj(end);
		++i;
		++actualNum;//每获取一个就++一下
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	span->_useCount += actualNum;//释放的流程有用
	_spanLists[index]._mtx.unlock();//加了锁一定要注意解锁,否则就会造成死锁的现象
	return actualNum;
}




//获取一个非空的span,可能是从central chche的自由链表中获取
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//查看当前的spanlist中是不是还有没分配对象的span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_freeList != nullptr)
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}


	//先把centralcache的桶锁解开,如果其他线程释放内存也能进行,不会造成阻塞
	list._mtx.unlock();

	//运行到此处说明没有空闲的span了,只能找下一层的PageCache要      ----需要调用函数
	PageCache::GetInstance()->_pageMtx.lock();
	Span* span=PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	span->_isUse = true;
	span->_objSize = size;
	PageCache::GetInstance()->_pageMtx.unlock();


	//对获取span进行切分,这个地方不用加锁,其他线程访问不到这个span

	//在页缓存拿到span以后,除了切给自己的,还要将多的挂在页缓存的spanlist中
	//计算span的大块内存的起始地址和大块内存的大小(字节数)
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	//把大块内存切成自由链表链接起来
	//1.先切下来一块去做头,方便尾插2
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;

	int i = 1;
	while (start < end)
	{
		++i;//用来观测
		NextObj(tail) = start;
		tail = NextObj(tail);//这个也就相当于tail=start
		start += size;
	}
	NextObj(tail) = nullptr;
	

	//切好span以后,要将span挂到桶里面的时候再加锁
	list._mtx.lock();
	list.PushFront(span);

	return span;
}





// 将一定数量的对象释放到span跨度
//但是
void CentralCache::ReleaseListToSpans(void* start, size_t size)//给的size就是要算属于哪个桶
{
	size_t index = SizeClass::Index(size);
	_spanLists[index]._mtx.lock();
	while (start)
	{
		void* next = NextObj(start);

		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);
		NextObj(start) = span->_freeList;
		span->_freeList = start;
		span->_useCount--;
		if (span->_useCount == 0)//此时就说明切分出去的小块内存回来了,然后这个span就能返回给PageCache,条件符合PageCache就可以进行页的合并
		{
			_spanLists[index].Erase(span);
			span->_freeList = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;

			//释放span给PageCache的时候,使用pagecache的锁就可以了
			//这时把桶锁解掉就行
			_spanLists[index]._mtx.unlock();

			PageCache::GetInstance()->_pageMtx.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);//页能合并,就是因为原本就是大块的span切分出去的,还回去的时候还涉及到锁的问题
			PageCache::GetInstance()->_pageMtx.unlock();

			_spanLists[index]._mtx.lock();
		}

		start = next;
	}
	_spanLists[index]._mtx.unlock();
}























