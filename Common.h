#pragma once
//这个头文件用来存放的事都需要的公共头文件之类的

#include<iostream>
#include<vector>
#include<unordered_map>
#include<algorithm>//2_TC.cpp中min函数的头文件

#include<map>


#include<assert.h>
#include<time.h>


#include<thread>//创建和管理线程的类的库函数
#include<mutex>//包含着互斥锁(本质上还是一个类),用来解决多个线程访问共同资源的同步问题,当一个线程获取了 std::mutex 对象的锁时，其他线程不能同时获取该锁，只能等待该线程释放锁。


using std::cout;
using std::endl;

#ifdef _WIN32
	#include<windows.h>
#else
//...
#endif

static const size_t MAX_BYTES = 256 * 1024;//256kb       1024字节，也就是1024b，是1kb  1024byte
static const size_t NFREELIST = 208;//这个数字也就是说在线程缓存中，一共有208个桶
static const size_t NPAGES = 129;//页缓存中最大存放的最大span内存块就是128个,因为要空出来一个空位置不存放桶,方便计算下标
static const size_t PAGE_SHIFT = 13;//


typedef size_t PAGE_ID;//WIN32是windows下的32位程序,能编出来的最大页size_t能显式




inline static void* SystemAlloc(size_t kpage)//内联函数
{
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);//一页是 8k   kpage*8*l024

	if (ptr == nullptr)
		throw std::bad_alloc();//没申请出来内存就报错
	return ptr;
}




inline static void SystemFree(void* ptr)//将申请的内存还给堆的函数
{
	VirtualFree(ptr, 0, MEM_RELEASE);
}




//这个函数就是专门用来取传入的对象的头4个字节
static void*& NextObj(void* obj)
{
	return *(void**)obj;//这就取到了头上的4个字节或者8个字节
}


//-----------------------------------------------------------------自由链表的设计------------------------------------------------------
class FreeList//这个类是用来管理切分好的小对象的自由链表
{
public:
	//---------------自由链表中的头插
	void Push(void* obj)//回收的对象插入进自由链表
	{
		assert(obj);//链表为空可以插入，但是插入的东西不能是空的
		//插入使用的是头插
		NextObj(obj) = _freeList;//用二级指针强转，然后再进行解引用，是因为不知道是多少位的电脑，这样无论是4字节还是8字节都能用这个指针保存地址
		_freeList = obj;

		++_size;
	}


	//-----------自由链表的地址区间插入
	void PushRange(void* start, void* end,size_t n)//range:范围   参数n是插入的范围中有多少个数
	{
		NextObj(end) = _freeList;
		_freeList = start;
		_size += n;
	}


	//删除一段范围的接口
	void PopRange(void*& start, void*& end, size_t n)
	{
		assert(n <= _size);
		start =_freeList;
		end = start;

		for (size_t i = 0;i < n - 1; ++i)
		{
			end = NextObj(end);
		}

		_freeList = NextObj(end);
		NextObj(end) = nullptr;
		_size -= n;
	}



	//提供一个访问_size的接口
	size_t Size()
	{
		return _size;
	}



	//---------------自由链表中的头删
	void* Pop()//弹出一个对象大小的空间,供申请的使用
	{
		assert(_freeList);//删除的前提是这个链表存在空间可供使用
		//头删
		void* obj = _freeList;
		_freeList = NextObj(obj);

		--_size;
		return obj;
	}


	//自由链表的判空操作
	bool Empty()//返回真就是空表
	{
		return _freeList == nullptr;//
	}

	size_t& MaxSize()
	{
		return _maxSize;
	}

private:
	void* _freeList = nullptr;
	size_t _maxSize = 1;
	size_t _size=0;//这个变量用来保存数据的个数
};






//------------------------------------------------------进程中的哈希桶的设计-------------------------------------------------------------
//计算对象大小的对齐映射规则
//如果按照8字节一个桶，那么一个线程中是256kb=256*1024b，也就需要32768个哈希桶，太多了，所以映射规则肯定是要改进的

// 整体控制在最多10%左右的内碎片浪费                --这个最多比如申请了129个字节，但是实际申请了129+15     然后多出来15用不到   15/(129+15)约等于0.1....
 // [1,128] 8byte对齐       freelist[0,16)     16个桶                  -----也就是说在1-128b（这里指的是128字节）这个范围，每8字节一个桶
 // [128+1,1024] 16byte对齐   freelist[16,72)       56个桶           -----129字节-1kb       每16字节一个桶
 // [1024+1,8*1024] 128byte对齐   freelist[72,128)       56个桶
 // [8*1024+1,64*1024] 1024byte对齐     freelist[128,184)     
 // [64*1024+1,256*1024] 8*1024byte对齐   freelist[184,208)

//这样会大大减少桶的数量
class SizeClass
{
public:

	//这个函数是计算实际申请的内存                     -----这个是普通版本的实现
	//size_t _RoundUp(size_t size,size_t alignNum)//AlignNum：对齐数
	//{
	//	size_t alignSize;//这个变量是实际上申请的空间大小
	//	if (size % 8 != 0)//模上8刚好等于0的，就说明刚好能够整除
	//	{
	//		alignSize = (size / alignNum + 1) * alignNum;//向上对齐
	//	}
	//	else
	//	{
	//		alignSize = size;
	//	}
	//}


	//这个是大佬的版本
	//上面的所有对齐数都是2的次方
	static inline size_t _RoundUp(size_t bytes, size_t alignNum)//bytes是申请的字节数   alignNum是对齐数
	{
		//也就是说这一步的操作会将所有给出的bytes向上进行对齐
		return (((bytes)+alignNum - 1) & ~(alignNum - 1));//将（对齐数-1取反），与上（申请的字节数+对齐数-1），就是实际申请的空间，同样会向上进行对齐
	}
	//包括下面的那有一个，相对于四则运算，位运算符的效率比四则运算高


	//上下这两个函数因为在类的内部封装，但又是独立调用的，所以要用静态的内联函数来形容
	static inline size_t RoundUp(size_t size)//这个函数的作用就是你给我一个size，我就算出来你对齐以后所占用的实际内存是多少
	{
		if (size <= 128)
		{
			return _RoundUp(size, 8);
		}
		else if (size <= 1024)
		{
			return _RoundUp(size, 16);
		}
		else if (size <= 8 * 1024)
		{
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024)
		{
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024)
		{
			return _RoundUp(size, 8*1024);
		}
		else
		{
			return _RoundUp(size, 1 << PAGE_SHIFT);
		}
	}



	//----------------------------------------用于计算有多少桶的函数------------------
	//有多少桶就代表每个区间有多少链表
	//这个函数主要是用于计算映射的是哪一个自由链表桶
	//计算桶下标的函数还是自己实现一个，参考大佬一个
	
	//自己实现的函数
	//static inline size_t _Index(size_t bytes,size_t alignNum)//参数还是需要对齐数
	//{
	//	if (bytes%alignNum==0)//模上对齐数如果是0的话，index就是bytes直接除以对齐数，然后减1
	//	{
	//		return bytes / alignNum - 1;
	//	}
	//	else
	//	{
	//		return bytes / alignNum;//如果不能整除对齐数，那么可以直接除以
	//	}
	//}




	static inline size_t _Index(size_t bytes, size_t align_shift)//大佬版的思想
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}


	// 计算映射的哪一个自由链表桶
	static inline size_t Index(size_t bytes)
	{
		assert(bytes <= MAX_BYTES);//申请的字节数必须小于这个线程缓存中的最大字节数
		// 每个区间有多少个链
		static int group_array[4] = { 16, 56, 56, 56 };
		if (bytes <= 128) {
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 8 * 1024) {
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes - 8 * 1024, 10) + group_array[2] + group_array[1]
				+ group_array[0];
		}
		else if (bytes <= 256 * 1024) {
			return _Index(bytes - 64 * 1024, 13) + group_array[3] +
				group_array[2] + group_array[1] + group_array[0];
		}
		else {
			assert(false);
		}
		return -1;
	}



	//-------------一次ThreadCache 从中心缓存中获取多少个对象
	 static size_t NumMoveSize(size_t size)//参数就是线程缓存中变量需要的单个对象大小,但是不能要一次给一次,要一次给多个相同大小的内存块
	{
		 assert(size>0);//不能是0
		// [2, 512]，一次批量移动多少个对象的(慢启动)上限值
		// 小对象一次批量上限高
		// 小对象一次批量上限低
		int num = MAX_BYTES / size;//超过512个的就按照512,没有超过的按照原数,大于等于128kb的给俩
		if (num < 2)
			num = 2;
		if (num > 512)
			num = 512;
		return num;
	}





	 //--------------计算一次向系统获取几个页
	  // 单个对象 8byte
	  // ......
	  //单个对象 256KB
	 static size_t NumMovePage(size_t size)
	 {
		 size_t num = NumMoveSize(size);
		 size_t npage = num * size;//这个实际上是算出来的字节数

		 npage >>= PAGE_SHIFT;//一页是8k的话,也就是2的13次方字节,除以8k,就是右移13位
		 if (npage == 0)
			 npage = 1;//也就是说中心缓存哪怕申请的空间很小,最少也得分过去一页让中心缓存使用

		 return npage;
	 }

};

 


//span不但central cache要使用，后面的page cache也要使用，所以定义到这里
//这个结构就是用来管理多个连续页大块内存跨度结构
struct Span
{
	//页号是什么，通俗的来说就是进程地址中一个个8k大小的空间，一般情况下进程地址大小是4g，大概能编出来2的19次方页
	//但是在64位系统下  2的64次方除以 8k，这就多了
	PAGE_ID _pageId=0;//大块内存的起始页的页号
	size_t _n=0;//页的数量

	Span* _next=nullptr;//span链表是一个带头循环的双向链表
	Span* _prev=nullptr;

	size_t _objSize = 0;//切好的小对象的大小,单位是字节

	size_t _useCount=0;//被切好的小块内存被分配给ThreadCache的计数,被使用了就++,还回来了就--
	//如果中心缓存中的usecount=0,说明切分给线程缓存的的小块内存都回来了.则中心缓存将这个span还给页缓存
	//页缓存通过页号,查看前后相邻的页是否空闲,是的话就合并,合并出更大的页,解决内存碎片问题

	void* _freeList=nullptr;//切好的小块内存的自由链表

	bool _isUse=false;//是否在被使用,默认是false
};






//spanList的定义   这个是一个带头的双向的循环链表
class SpanList
{
public:
	//必须要用构造函数完成初始化
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}




	Span* PopFront()
	{
		Span* front = _head->_next;
		Erase(front);
		return front;
	}




	void Insert(Span* pos, Span* newSpan)
	{
		assert(pos && newSpan);
		
		Span* prev = pos->_prev;
		prev->_next = newSpan;
		newSpan->_prev = prev;
		newSpan->_next = pos;
		pos->_prev = newSpan;
	}


	void Erase(Span* pos)//注意删除的时候不能删除头结点
	{
		assert(pos);
		assert(pos != _head);



		Span* prev = pos->_prev;
		Span* next = pos->_next;

		prev->_next = next;
		next->_prev = prev;
	}


	//提供了begin和end函数就支持遍历了
	Span* Begin()//要记得span是带头的双向循环链表
	{
		return _head->_next;
	}


	Span* End()
	{
		return _head;
	}



	bool Empty()//查看保存对应页数的桶是不是空
	{
		return _head->_next == _head;//返回真就是空
	}







	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}

private:
	Span* _head;

public:
	std::mutex _mtx;//访问几号桶的时候,就得把锁挂上,访问同一个桶,就会出现竞争   ---桶锁
	//当一个线程调用mutex::lock()获取互斥锁的所有权,其余线程就不能再调用mutex相关的函数
};














