#include"PageCache.h"

PageCache PageCache::_sInst;


//获取一个k页的span
Span* PageCache::NewSpan(size_t k)
{
	//先在PageCache中查看对应的桶有没有空余的,但是第二页没有可以往后找,而不是直接向堆申请
	assert(k > 0);

	if (k > NPAGES - 1)//大于128页的内存直接向堆上申请
	{
		void* ptr = SystemAlloc(k);
		Span* span = _spanPool.New(); //new Span 定长内存池的作用就是要替代new
		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->_n = k;

		_idSpanMap.set(span->_pageId, span);
		return span;
	}


	//还有不能在这里加上桶锁,因为如果在这里加上锁,会在遍历桶查找大span的时候进行频繁地加锁解锁,效率降低
	//先检查第k个桶里面有没有span
	if (!_spanLists[k].Empty())
	{
		Span* kSpan= _spanLists[k].PopFront();//popfront是将span从这个地方解掉
		for (PAGE_ID i = 0; i < kSpan->_n; ++i)
		{
			_idSpanMap.set(kSpan->_pageId + i, kSpan);
		}
		return kSpan;
	}

	//代码到达这里就说明这个桶空的,然后检查后面的桶里面有没有span,如果有,可以把后面大的span进行切分
	for (size_t i = k + 1; i < NPAGES; ++i)
	{
		if (!_spanLists[i].Empty())//不为空就可以开始切,一般是切成一个k页的span,和一个n-k页的span,并且k页的span返回给centralcache,n-k页的span挂到n-k桶中去  切的时候头切尾切都一样
		{
			Span* nSpan = _spanLists[i].PopFront();//这个是切剩下的那部分span

			Span* kSpan = _spanPool.New();//new Span;//这个是切出来返回给CentralCache的span
			
			//这里采用了头切
			//k页的span返回
			//nSpan再挂到对应映射的位置
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanLists[nSpan->_n].PushFront(nSpan);
			//存储nSpan的首尾页号跟nSpan映射,方便pagecache回收内存的时候进行合并查找
			_idSpanMap.set(nSpan->_pageId, nSpan);
			_idSpanMap.set(nSpan->_pageId + nSpan->_n - 1, nSpan);

			//建立id和span的映射,方便centralcache回收小块内存的时候查找相应的span
			for (PAGE_ID i = 0; i < kSpan->_n; ++i)
			{
				_idSpanMap.set(kSpan->_pageId + i,kSpan);
			}

			return kSpan;
		}
	}


	//代码走到这里,就说明PageCache中没有大页的span供切分(也有可能是程序刚开始运行,程序刚开始运行的时候PageCache中同样是没有span的)
	//这时就需要向堆上申请一个128页的span
	Span* bigSpan = _spanPool.New();//new Span;
	void* ptr =SystemAlloc(NPAGES - 1);//返回的指针需要返回页号
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;//算出页号多少
	bigSpan->_n=NPAGES-1;

	_spanLists[bigSpan->_n].PushFront(bigSpan);

	return NewSpan(k);
}




// 获取从对象到span的映射
Span* PageCache::MapObjectToSpan(void* obj)//中心缓存向页缓存中申请内存的时候的对象，对应的页号是多少
{
	PAGE_ID id = ((PAGE_ID)obj >> PAGE_SHIFT);

	auto ret = (Span*)_idSpanMap.get(id);
	assert(ret != nullptr);
	return ret;
}





void PageCache::ReleaseSpanToPageCache(Span* span)
{
	if (span->_n > NPAGES - 1)
	{
		//大于128页的直接还给堆
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		_spanPool.Delete(span);//delete span;
		return;
	}
	//开始处理外碎片,对span的前后的页,尝试进行合并,缓解内存碎片的问题

	//向前合并
	// 对span前后的页，尝试进行合并，缓解内存碎片问题
	while (1)
	{
		PAGE_ID prevId = span->_pageId - 1;
		auto ret = (Span*)_idSpanMap.get(prevId);
		if (ret == nullptr)
		{
			break;
		}

		// 前面相邻页的span在使用，不合并了
		Span* prevSpan =ret;
		if (prevSpan->_isUse == true)
		{
			break;
		}

		// 合并出超过128页的span没办法管理，不合并了
		if (prevSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}

		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		_spanLists[prevSpan->_n].Erase(prevSpan);
		_spanPool.Delete(prevSpan);//delete prevSpan;
	}


	//向后合并
	while (1)
	{
		PAGE_ID nextId = span->_pageId + span->_n;
		auto ret = (Span*)_idSpanMap.get(nextId);
		if (ret == nullptr)
		{
			break;
		}

		Span* nextSpan = ret;
		if (nextSpan->_isUse == true)
		{
			break;
		}

		if (nextSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}

		span->_n += nextSpan->_n;

		_spanLists[nextSpan->_n].Erase(nextSpan);
		_spanPool.Delete(nextSpan);//delete nextSpan;
	}

	_spanLists[span->_n].PushFront(span);
	span->_isUse = false;
	_idSpanMap.set(span->_pageId, span);
	_idSpanMap.set(span->_pageId + span->_n - 1, span);
}




















