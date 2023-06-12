#pragma once
//前言：线程缓存是哈希桶结构，每个桶是一个按桶位置映射大小的内存块对象的自由链表。每个线程都会有一个thread cache对象，这样每个线程在这里获取对象和释放对象是无锁的

//自由链表：管理切好的小块内存。使用自由链表的空间的时候是头删，小块内存回收进自由链表时是头插
//定长内存池其实就是为了解决固定长度大小的变量，并发内存池还是为了解决不同大小变量的长度
//所以也就是说并发内存池有很多自由链表，不同大小的结点的自由链表

//和定长内存池有些类似，这个给的内存是一个范围，比如小于8字节给8字节，8~16字节的给16,16~32字节的给32.......
//但是会存在一定的空间浪费，想这种只需要5个字节的空间，但是给了8个字节的，永远会空出来3个字节，这样的三个字节就叫做内碎片
//其实就像多个定长的内存池，也就是哈希的映射




#include"Common.h"

class ThreadCache//这里是单个的线程缓存
{
public:
	//------------------------------------------------框架的两个重要的组成函数:申请和释放内存-------------------------------------
	void* Allocate(size_t size);

	void Deallocate(void* ptr, size_t size);

	//上面两个是线程缓存中申请和释放空间，但是如果申请的空间超过了或者剩余的空间不足，则会向中心缓存获取对象
	void* FetchFromCentralCache(size_t index, size_t size);

	// 链表过长时，回收内存回到中心缓存
	void ListTooLong(FreeList& list, size_t size);

private:
	FreeList _freeLists[NFREELIST];//类 类型的数组
};


static _declspec(thread) ThreadCache* pTLSThreadCache=nullptr; // 这句告诉给编译器此变量为线程内部使用，每个线程都会copy一份给本地用
//ps1:_declspec(thread)创建出来的变量，在每个线程中都会存在一个
//ps2:_declspec(thread)是MSVC（Visual C++）提供的语法扩展，用于声明一个变量是线程局部存储。在多线程程序中，每个线程都有一份自己独立的内存空间，存储该线程所需的数据，称为线程局部存储区。
// 这样做的好处是可以避免线程之间的数据竞争和冲突，提高了程序的效率和安全性。
// 
//ps3:在这段代码中，ThreadCache* pTLSThreadCache是一个指针类型的变量，它指向一个名为ThreadCache的类的对象。
//由于使用了_declspec(thread)语法，因此每个线程都会创建一份ThreadCache对象，并且该对象只能在当前线程中被访问。通过这种方式，可以避免多线程程序中对同一ThreadCache对象的竞争和冲突，从而提高了程序的效率和并发性能。



