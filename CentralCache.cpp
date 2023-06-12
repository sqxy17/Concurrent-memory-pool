#include"CentralCache.h"
#include"PageCache.h"

//����ģʽ�Ķ���ģʽ,�ǵ���ģʽ��ʵ�ʷ�ʽ֮һ,������ģʽ��,���������������ʱ�ͱ���������
//�ڶ���ģʽ�У����������������ʱ�ͱ������������������ڵ�һ��ʹ��ʱ�Ž��д�������ˣ����ڶ��̻߳��������̰߳�ȫ�ģ�����Ҫ���Ǿ���������
//����ʵ�ַ���ͨ��������Ķ�����ֱ�Ӵ���һ����̬�ġ�˽�еĵ�������Ȼ���ṩһ����̬�Ĺ������������ʸö���
//�ڶ���ģʽ�У���������ĳ�ʼ�����ڳ�������ʱ����ɵ�
CentralCache CentralCache::_sInst;




//��CentralCacheҪ�������ڴ�ĺ���
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	//��һ������Ҫ�����ĸ�Ͱ�����,����CentralCache�ж�Ӧ��ͰҲ�п����ǿյ�,�ǿյĻ��߲���,��������һ�㻺��������ռ�
	size_t index = SizeClass::Index(size);

	//��Ϊ�п��ܶ���̶߳��������Ļ����е�ͬһ��Ͱ�������ڴ�,��ô���Ȱ���������
	_spanLists[index]._mtx.lock();//����Ƕ�ĳ��Ͱ������

	Span* span = GetOneSpan(_spanLists[index],size);
	assert(span);//����Ҫ����,Ҫ���Ͳ����ǿյ�
	assert(span->_freeList);

	//�����￪ʼֱ����������,��Ҫ���þ��Ǵ�span�л�ȡbatchNum������
	//�������batchNum��,�ж����ö���
	start = span->_freeList;
	end = start;

	size_t i = 0;
	size_t actualNum = 1;//ʵ�ʻ�ȡ�˶��ٸ�
	while( i < batchNum-1 && NextObj(end)!=nullptr)//Ҳ����˵��sapnʣ��Ĳ�������Ĵ�С�������,�ж��ٸ�����
	{
		end = NextObj(end);
		++i;
		++actualNum;//ÿ��ȡһ����++һ��
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	span->_useCount += actualNum;//�ͷŵ���������
	_spanLists[index]._mtx.unlock();//������һ��Ҫע�����,����ͻ��������������
	return actualNum;
}




//��ȡһ���ǿյ�span,�����Ǵ�central chche�����������л�ȡ
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//�鿴��ǰ��spanlist���ǲ��ǻ���û��������span
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


	//�Ȱ�centralcache��Ͱ���⿪,��������߳��ͷ��ڴ�Ҳ�ܽ���,�����������
	list._mtx.unlock();

	//���е��˴�˵��û�п��е�span��,ֻ������һ���PageCacheҪ      ----��Ҫ���ú���
	PageCache::GetInstance()->_pageMtx.lock();
	Span* span=PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	span->_isUse = true;
	span->_objSize = size;
	PageCache::GetInstance()->_pageMtx.unlock();


	//�Ի�ȡspan�����з�,����ط����ü���,�����̷߳��ʲ������span

	//��ҳ�����õ�span�Ժ�,�����и��Լ���,��Ҫ����Ĺ���ҳ�����spanlist��
	//����span�Ĵ���ڴ����ʼ��ַ�ʹ���ڴ�Ĵ�С(�ֽ���)
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	//�Ѵ���ڴ��г�����������������
	//1.��������һ��ȥ��ͷ,����β��2
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;

	int i = 1;
	while (start < end)
	{
		++i;//�����۲�
		NextObj(tail) = start;
		tail = NextObj(tail);//���Ҳ���൱��tail=start
		start += size;
	}
	NextObj(tail) = nullptr;
	

	//�к�span�Ժ�,Ҫ��span�ҵ�Ͱ�����ʱ���ټ���
	list._mtx.lock();
	list.PushFront(span);

	return span;
}





// ��һ�������Ķ����ͷŵ�span���
//����
void CentralCache::ReleaseListToSpans(void* start, size_t size)//����size����Ҫ�������ĸ�Ͱ
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
		if (span->_useCount == 0)//��ʱ��˵���зֳ�ȥ��С���ڴ������,Ȼ�����span���ܷ��ظ�PageCache,��������PageCache�Ϳ��Խ���ҳ�ĺϲ�
		{
			_spanLists[index].Erase(span);
			span->_freeList = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;

			//�ͷ�span��PageCache��ʱ��,ʹ��pagecache�����Ϳ�����
			//��ʱ��Ͱ���������
			_spanLists[index]._mtx.unlock();

			PageCache::GetInstance()->_pageMtx.lock();
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);//ҳ�ܺϲ�,������Ϊԭ�����Ǵ���span�зֳ�ȥ��,����ȥ��ʱ���漰����������
			PageCache::GetInstance()->_pageMtx.unlock();

			_spanLists[index]._mtx.lock();
		}

		start = next;
	}
	_spanLists[index]._mtx.unlock();
}























