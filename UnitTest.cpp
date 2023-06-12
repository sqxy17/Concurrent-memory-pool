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



void TestThreadCache1()//�����߳���û���Լ��Ŀռ�
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
	printf("δ�Ż�ǰ1000000�������ͷ�ʱ�䣺%u", end - begin);
}





//int main()
//{
//	double* ptr = (double*)ConcurrentAlloc(1);
//	*ptr = 9.2;
//	cout << *ptr << endl;
//	cout << sizeof(*ptr) << endl;
//}

//ps��֪ʶ��Ĳ���
/*std::thread::join()���������ڵȴ�һ���߳�ִ����ϲ���������Դ�ĺ�����������˵������join()��������ǰ�̣߳�ֱ�������õ��߳�ִ�н���Ϊֹ��
�ڱ����õ��߳̽�����join()�������������Դ���������̱߳��Ϊ�����״̬��

ʹ��join()�������Ա�֤���̵߳ȴ����߳�ִ�н������ټ���ִ�У������������̻߳�δ����ʱ���߳���ǰ�˳������⡣
���⣬���������join()��detach()��������std::thread��������ʱ�����std::terminate()���������³��������

��Ҫע����ǣ�����̻߳�δִ�н����͵�����join()��������ǰ�߳̽�һֱ������ֱ�������õ��߳�ִ�н���Ϊֹ��
��ˣ��ڵ���join()����ʱ��Ҫȷ�������õ��߳��Ѿ�ִ����ϣ�������ܻ������ڵ�������ǰ�̡߳�*/