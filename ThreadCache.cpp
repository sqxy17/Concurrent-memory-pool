#include"ThreadCache.h"
#include"CentralCache.h"//



//这个代码用于线程缓存内部成员函数的实现
//申请空间大小   参数是申请的字节数
//在线程缓存中，一共有208个桶
void* ThreadCache::Allocate(size_t size)//但是这个size可能是1,5,8,12.....等等都有可能，不满足指针大小的向指针大小对齐
{
	//此处的哈希桶的映射规则是size的字节在哪个范围，也就是说桶是一个范围的
	assert(size <= MAX_BYTES);//也就是说进程的最大能申请的内存是256kb
	size_t alignSize = SizeClass::RoundUp(size);//alignSize是实际申请的内存,也就是向上对齐
	size_t index = SizeClass::Index(size);//index可以理解为哈希表的下标，也就是当前申请的空间的大小是在哪个桶里面

	if (!_freeLists[index].Empty())//Thread cache 无锁的原因就是每个线程获取的是各自的tls变量
	{
		return _freeLists[index].Pop();
	}
	else//也就是说这个对应的自由链表中没有，就得往下一层查找了    central cache
	{
		return FetchFromCentralCache(index, alignSize);//这个参数就只能给对齐以后的实际长度
	}
}




// 释放对象时，链表过长时，回收内存回到中心缓存
void ThreadCache::ListTooLong(FreeList& list, size_t size)//还内存并不一定全部还回去
{
	void* start = nullptr;
	void* end = nullptr;
	list.PopRange(start, end, list.MaxSize());//拿出来一部分内存准备还回去

	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}




//想要回收的空间       ptr是回收空间的地址，size是回收空间的大小
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(size <= MAX_BYTES);
	assert(ptr);

	//找对应的自由链表桶，对象插入进去
	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(ptr);

	//当链表长度大于一次批量申请的内存的长度就开始返回一段list给centralcache                   ----此处是简化版,没有细节考虑线程缓存的总大小想超过多少返还内存
	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		// 释放对象时，链表过长时，回收内存回到中心缓存
		ListTooLong(_freeLists[index], size);
	}

}




void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)//这个函数是在ThreadCache中没有没存供申请,就会调用这个函数向centralcache申请
{
	//小字节的申请空间,可以多给一些内存块,大字节的申请空间,可以少给一些内存块      -----这就用到了慢开始调节算法
	//batch:批量
	//下面就是一个慢增长,第一次要给一个,第二次要给两个,第三次要给三个.....  但是最多不会比512大
	//1.也就是说最开始不会向CentralCache要太多,如果要的内存块数量过多可能用不完
	//2.size越大,一次向CentralCache要的batchNum就越小
	//3.如果size越小,一次向CentralCache要的batchNum就越大
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));//这个函数是向中心缓存申请的内存块的个数,供线程缓存的使用
	if (_freeLists[index].MaxSize() == batchNum)
	{
		_freeLists[index].MaxSize() += 1;//如果增长的慢,可以变更为345....
	}

	void* start = nullptr;
	void* end = nullptr;//这两个是中心缓存中的一个span切割成的小内存块的首地址尾地地址

	//span中如果不够线程缓存中申请的数量,先把这个span中剩余的内存块有多少给多少
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);//actual实际,也就是线程缓存实际申请内存块的数量
	assert(actualNum > 0);//至少得有1个

	if (actualNum == 1)//如果实际只获取了一个,那一个内存块被拿去使用了,并且
	{
		assert(start == end);
		return start;
	}
	else
	{
		_freeLists[index].PushRange(NextObj(start), end, actualNum - 1);
		return start;
	}



	return nullptr;
}


