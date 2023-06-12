#include "ObjectPool.h"
#include "ConcurrentAlloc.h"
#include<vector>
#include "ThreadCache.h"
#include"CentralCache.h"

void Alloc1()
{
	for (size_t i = 0; i < 5; ++i)
	{
		void* ptr = ConcurrentAlloc(6);
	}
}

void Alloc2()
{
	for (size_t i = 0; i < 5; ++i)
	{
		void* ptr = ConcurrentAlloc(7);
	}
}

void TLSTest()
{
	std::thread t1(Alloc1);
	t1.join();

	std::thread t2(Alloc2);
	t2.join();
}


void TestConcurrentAlloc()
{
	int* p = (int*)ConcurrentAlloc(sizeof(int));
	*p = 18;
	cout << *p << endl;
}


void TestConcurrentFree()
{
	int* p = (int*)ConcurrentAlloc(sizeof(int));
	*p = 18;
	ConcurrentFree(p);
	cout << *p << endl;
}



void TestThreadCache1()//测试线程有没有自己的空间
{
		if (pTLSThreadCache == nullptr)
		{
			pTLSThreadCache = new ThreadCache;
		}

		size_t size = 16;
		void* obj1 = pTLSThreadCache->Allocate(size);
		void* obj2 = pTLSThreadCache->Allocate(size);
		pTLSThreadCache->Deallocate(obj1, size);
		pTLSThreadCache->Deallocate(obj2, size);

		cout << "TestThreadCache finished successfully!" << endl;
	
}


void TestThreadCache2()
{
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache;
	}
	void* obj = pTLSThreadCache->Allocate(16);
	pTLSThreadCache->Deallocate(obj, 16);

}




void TestThreadCache3()
{
	std::vector<void*> v1;
	for (int i = 0; i < 5; i++)
	{
		v1.push_back(ConcurrentAlloc(4));
	}
	for (int i = 0; i < 5; i++)
	{
		ConcurrentFree(v1[i]);
	}
}





void TestCentralCache1()
{
	std::vector<void*> v1;
	for (int i = 0; i < 2; i++)
	{
		v1.push_back(ConcurrentAlloc(4));
	}

	for (int i = 0; i < 2; i++)
	{
		ConcurrentFree(v1[i]);
	}
}





void TestPageCache1()
{
	ConcurrentAlloc(4);
}




void TestPageCache2()
{
	void* ptr1 = SystemAlloc(129 * 1024);
}




void TestPageCache3()
{
	std::vector<void*> v1;
	for (int i = 0; i < 10000; i++)
	{
		v1.push_back(ConcurrentAlloc(8));
	}

	for (int i = 0; i < 10000; i++)
	{
		ConcurrentFree(v1[i]);
		if (i == 9998)
		{
			int x = 0;
		}
	}
}







void TestPageCache()
{
	std::vector<void*> v1;
	size_t begin = clock();
	for (int i = 0; i < 1000000; i++)
	{
		v1.push_back(ConcurrentAlloc(100));
	}
	for (int i = 0; i < 1000000; i++)
	{
		ConcurrentFree(v1[i]);
	}
	size_t end = clock();
	printf("未优化前1000000次申请释放时间：%u", end - begin);
}





//int main()
//{
//	double* ptr = (double*)ConcurrentAlloc(1);
//	*ptr = 9.2;
//	cout << *ptr << endl;
//	cout << sizeof(*ptr) << endl;
//}

//ps：知识点的补充
/*std::thread::join()函数是用于等待一个线程执行完毕并回收其资源的函数。具体来说，调用join()会阻塞当前线程，直到被调用的线程执行结束为止。
在被调用的线程结束后，join()函数会回收其资源，并将该线程标记为已完成状态。

使用join()函数可以保证主线程等待子线程执行结束后再继续执行，避免了在子线程还未结束时主线程提前退出的问题。
另外，如果不调用join()或detach()函数，当std::thread对象被销毁时会调用std::terminate()函数，导致程序崩溃。

需要注意的是，如果线程还未执行结束就调用了join()函数，则当前线程将一直阻塞，直到被调用的线程执行结束为止。
因此，在调用join()函数时，要确保被调用的线程已经执行完毕，否则可能会无限期地阻塞当前线程。*/