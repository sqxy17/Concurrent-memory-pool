#pragma once

#include"Common.h"
#include"ObjectPool.h"//为了将new替换成自己实现的申请空间的函数
#include"PageMap.h"
//页缓存主要就是为了中心缓存没有span之后找到相应的span
//但是页缓存中就不能使用桶锁了,使用桶锁的话会影响到后面的内存的分裂与合并,但是还是会用锁
class PageCache
{
public:

	static PageCache* GetInstance()
	{
		return &_sInst;
	}


	// 获取从对象到span的映射
	Span* MapObjectToSpan(void* obj);

	// 释放空闲span回到Pagecache，并合并相邻的span
	//只有这个函数和下面的那个函数会建立id和span的映射,也就是写
	//基数树,写之前会提前开好空间,写数据的过程中,也不会动结构
	//读写是分离的,线程1对一个位置读写的时候,线程2不可能对这个位置进行读写
	void ReleaseSpanToPageCache(Span* span);


	//中心缓存有span我就用中心缓存的,但是中心缓存没有span可用了,就要找页缓存申请了,这个就是找页缓存申请span的函数
	//获取k页的span
	Span* NewSpan(size_t k);
	std::mutex _pageMtx;//这个锁的名字和中心缓存的不一样就是为了区分


private:
	SpanList _spanLists[NPAGES];//128个桶


	ObjectPool<Span> _spanPool;

	//std::map<PAGE_ID, Span*> _idSpanMap;
	TCMalloc_PageMap2<32 - PAGE_SHIFT> _idSpanMap;




	//PageCache同样也要设置成单例模式
	PageCache()
	{}


	PageCache(const PageCache&) = delete;//c++11的语法



	static PageCache _sInst;
};


//细节讲解:
//ps1:假设页缓存中的存放4页的span链表不存在,并不是直接向堆上去申请4页的空间,而是查看其他的比当前span大的链表有没有可用的.
//ps1_1比如使用一个能存放12页的span,切成3个4页的,然后再挂到能存放4页的span链表上


//1.所以最开始的时候,PageCache中一个span都没有挂
//这个情况下就是向系统上申请一个128页的span,然后挂到页缓存的,每个span能存放128页的span链表上
//如果中心缓存需要一个2k的span,那么就把128的切下来2k然后剩下的挂到126的span上,切出来的2k就挂到上面的2k的span上面
//最后页缓存会讲2k的span返回给中心缓存用


//2.在中心缓存向页缓存申请空间,并且页缓存在向堆上申请空间或者在页缓存查找大span的时候,可以将centralcache的桶锁解开了,就算中心缓存访问到了同一个桶



