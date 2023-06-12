#pragma once

#include"Common.h"
#include"ObjectPool.h"//Ϊ�˽�new�滻���Լ�ʵ�ֵ�����ռ�ĺ���
#include"PageMap.h"
//ҳ������Ҫ����Ϊ�����Ļ���û��span֮���ҵ���Ӧ��span
//����ҳ�����оͲ���ʹ��Ͱ����,ʹ��Ͱ���Ļ���Ӱ�쵽������ڴ�ķ�����ϲ�,���ǻ��ǻ�����
class PageCache
{
public:

	static PageCache* GetInstance()
	{
		return &_sInst;
	}


	// ��ȡ�Ӷ���span��ӳ��
	Span* MapObjectToSpan(void* obj);

	// �ͷſ���span�ص�Pagecache�����ϲ����ڵ�span
	//ֻ�����������������Ǹ������Ὠ��id��span��ӳ��,Ҳ����д
	//������,д֮ǰ����ǰ���ÿռ�,д���ݵĹ�����,Ҳ���ᶯ�ṹ
	//��д�Ƿ����,�߳�1��һ��λ�ö�д��ʱ��,�߳�2�����ܶ����λ�ý��ж�д
	void ReleaseSpanToPageCache(Span* span);


	//���Ļ�����span�Ҿ������Ļ����,�������Ļ���û��span������,��Ҫ��ҳ����������,���������ҳ��������span�ĺ���
	//��ȡkҳ��span
	Span* NewSpan(size_t k);
	std::mutex _pageMtx;//����������ֺ����Ļ���Ĳ�һ������Ϊ������


private:
	SpanList _spanLists[NPAGES];//128��Ͱ


	ObjectPool<Span> _spanPool;

	//std::map<PAGE_ID, Span*> _idSpanMap;
	TCMalloc_PageMap2<32 - PAGE_SHIFT> _idSpanMap;




	//PageCacheͬ��ҲҪ���óɵ���ģʽ
	PageCache()
	{}


	PageCache(const PageCache&) = delete;//c++11���﷨



	static PageCache _sInst;
};


//ϸ�ڽ���:
//ps1:����ҳ�����еĴ��4ҳ��span��������,������ֱ�������ȥ����4ҳ�Ŀռ�,���ǲ鿴�����ıȵ�ǰspan���������û�п��õ�.
//ps1_1����ʹ��һ���ܴ��12ҳ��span,�г�3��4ҳ��,Ȼ���ٹҵ��ܴ��4ҳ��span������


//1.�����ʼ��ʱ��,PageCache��һ��span��û�й�
//�������¾�����ϵͳ������һ��128ҳ��span,Ȼ��ҵ�ҳ�����,ÿ��span�ܴ��128ҳ��span������
//������Ļ�����Ҫһ��2k��span,��ô�Ͱ�128��������2kȻ��ʣ�µĹҵ�126��span��,�г�����2k�͹ҵ������2k��span����
//���ҳ����ὲ2k��span���ظ����Ļ�����


//2.�����Ļ�����ҳ��������ռ�,����ҳ���������������ռ������ҳ������Ҵ�span��ʱ��,���Խ�centralcache��Ͱ���⿪��,�������Ļ�����ʵ���ͬһ��Ͱ



