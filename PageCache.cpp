#include"PageCache.h"

PageCache PageCache::_sInst;


//��ȡһ��kҳ��span
Span* PageCache::NewSpan(size_t k)
{
	//����PageCache�в鿴��Ӧ��Ͱ��û�п����,���ǵڶ�ҳû�п���������,������ֱ���������
	assert(k > 0);

	if (k > NPAGES - 1)//����128ҳ���ڴ�ֱ�����������
	{
		void* ptr = SystemAlloc(k);
		Span* span = _spanPool.New(); //new Span �����ڴ�ص����þ���Ҫ���new
		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->_n = k;

		_idSpanMap.set(span->_pageId, span);
		return span;
	}


	//���в������������Ͱ��,��Ϊ��������������,���ڱ���Ͱ���Ҵ�span��ʱ�����Ƶ���ؼ�������,Ч�ʽ���
	//�ȼ���k��Ͱ������û��span
	if (!_spanLists[k].Empty())
	{
		Span* kSpan= _spanLists[k].PopFront();//popfront�ǽ�span������ط����
		for (PAGE_ID i = 0; i < kSpan->_n; ++i)
		{
			_idSpanMap.set(kSpan->_pageId + i, kSpan);
		}
		return kSpan;
	}

	//���뵽�������˵�����Ͱ�յ�,Ȼ��������Ͱ������û��span,�����,���԰Ѻ�����span�����з�
	for (size_t i = k + 1; i < NPAGES; ++i)
	{
		if (!_spanLists[i].Empty())//��Ϊ�վͿ��Կ�ʼ��,һ�����г�һ��kҳ��span,��һ��n-kҳ��span,����kҳ��span���ظ�centralcache,n-kҳ��span�ҵ�n-kͰ��ȥ  �е�ʱ��ͷ��β�ж�һ��
		{
			Span* nSpan = _spanLists[i].PopFront();//�������ʣ�µ��ǲ���span

			Span* kSpan = _spanPool.New();//new Span;//������г������ظ�CentralCache��span
			
			//���������ͷ��
			//kҳ��span����
			//nSpan�ٹҵ���Ӧӳ���λ��
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanLists[nSpan->_n].PushFront(nSpan);
			//�洢nSpan����βҳ�Ÿ�nSpanӳ��,����pagecache�����ڴ��ʱ����кϲ�����
			_idSpanMap.set(nSpan->_pageId, nSpan);
			_idSpanMap.set(nSpan->_pageId + nSpan->_n - 1, nSpan);

			//����id��span��ӳ��,����centralcache����С���ڴ��ʱ�������Ӧ��span
			for (PAGE_ID i = 0; i < kSpan->_n; ++i)
			{
				_idSpanMap.set(kSpan->_pageId + i,kSpan);
			}

			return kSpan;
		}
	}


	//�����ߵ�����,��˵��PageCache��û�д�ҳ��span���з�(Ҳ�п����ǳ���տ�ʼ����,����տ�ʼ���е�ʱ��PageCache��ͬ����û��span��)
	//��ʱ����Ҫ���������һ��128ҳ��span
	Span* bigSpan = _spanPool.New();//new Span;
	void* ptr =SystemAlloc(NPAGES - 1);//���ص�ָ����Ҫ����ҳ��
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;//���ҳ�Ŷ���
	bigSpan->_n=NPAGES-1;

	_spanLists[bigSpan->_n].PushFront(bigSpan);

	return NewSpan(k);
}




// ��ȡ�Ӷ���span��ӳ��
Span* PageCache::MapObjectToSpan(void* obj)//���Ļ�����ҳ�����������ڴ��ʱ��Ķ��󣬶�Ӧ��ҳ���Ƕ���
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
		//����128ҳ��ֱ�ӻ�����
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		_spanPool.Delete(span);//delete span;
		return;
	}
	//��ʼ��������Ƭ,��span��ǰ���ҳ,���Խ��кϲ�,�����ڴ���Ƭ������

	//��ǰ�ϲ�
	// ��spanǰ���ҳ�����Խ��кϲ��������ڴ���Ƭ����
	while (1)
	{
		PAGE_ID prevId = span->_pageId - 1;
		auto ret = (Span*)_idSpanMap.get(prevId);
		if (ret == nullptr)
		{
			break;
		}

		// ǰ������ҳ��span��ʹ�ã����ϲ���
		Span* prevSpan =ret;
		if (prevSpan->_isUse == true)
		{
			break;
		}

		// �ϲ�������128ҳ��spanû�취�������ϲ���
		if (prevSpan->_n + span->_n > NPAGES - 1)
		{
			break;
		}

		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		_spanLists[prevSpan->_n].Erase(prevSpan);
		_spanPool.Delete(prevSpan);//delete prevSpan;
	}


	//���ϲ�
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




















