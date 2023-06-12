#pragma once
#include"Common.h"


//span是管理以页为单位的大块内内存
//这个类哈希桶的每个位置下面挂的都是Span对象链接的链表
//spam链表被设计成双向的了，为了方便某个span全部还回来了，紧接着还给page cache
//centralcache也是208个桶




class CentralCache
{
public:

	// 从中心缓存获取一定数量的对象给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);//fetch:拿来     n是多少个   bye_size是每个的大小



	 // 从SpanList或者page cache获取一个span
	Span* GetOneSpan(SpanList& list, size_t size);


	// 从centralcache还回来的小内存重新挂到中心缓存的span上面
	void ReleaseListToSpans(void* start, size_t byte_size);


	//提供一个共有的函数
	static CentralCache* GetInstance()//这样就成功的设计出了一个单例模式
	{
		return &_sInst;
	}

private:
	SpanList _spanLists[NFREELIST];//也就是这两个缓存的桶的数量是一样的,也就是说在thread cache中是几号桶,在central cache中也是几号桶

private:
	//单例模式不想让别人构造这个对象就构成私有
	CentralCache()//哈希桶的表是自定义类型
	{}

	//为了防止拿到对象去拷贝构造,也要把拷贝构造封死
	CentralCache(const CentralCache&) = delete;//c++11的删除函数语法,就是为了防止有拿到CentralCache的对象去拷贝构造,所以直接删除拷贝构造函数

	static CentralCache _sInst;//在头文件中定义函数的实现,可能会被多次包含造成错误
};



//细节讲解:
//ps1:每个线程都有独享的ThreadCache,但是每个进程都只能有一个centralcache
//ps2:所以在谁急centralcache的时候,就适合将centralcache设计成单例模式
//ps3:在设计 Central Cache 时，将其设计为单例模式的主要原因是确保整个系统中只存在一个 Central Cache 的实例。这样做有以下几点好处：
//ps4:保证了全局资源的统一管理：Central Cache 是一个用于缓存数据的重要组件，如果出现多个实例，则很难得到统一的缓存数据，会造成混乱和不必要的错误。
//ps5:降低了系统开销：如果没有使用单例模式，那么每次需要访问 Central Cache 的时候都需要创建一个新的实例，这样会增加系统的开销



//单例模式:
//ps1:单例模式主要的主要目的就是保证在整个应用程序中只能存在一个类的实例,并且提供全局访问这个实例的方法
//ps2:但是这种模式通常需要自己来管理对象的生命周期
//ps3:此处的单例模式是饿汉模式






