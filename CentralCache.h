#pragma once
#include"Common.h"


//span�ǹ�����ҳΪ��λ�Ĵ�����ڴ�
//������ϣͰ��ÿ��λ������ҵĶ���Span�������ӵ�����
//spam������Ƴ�˫����ˣ�Ϊ�˷���ĳ��spanȫ���������ˣ������Ż���page cache
//centralcacheҲ��208��Ͱ




class CentralCache
{
public:

	// �����Ļ����ȡһ�������Ķ����thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);//fetch:����     n�Ƕ��ٸ�   bye_size��ÿ���Ĵ�С



	 // ��SpanList����page cache��ȡһ��span
	Span* GetOneSpan(SpanList& list, size_t size);


	// ��centralcache��������С�ڴ����¹ҵ����Ļ����span����
	void ReleaseListToSpans(void* start, size_t byte_size);


	//�ṩһ�����еĺ���
	static CentralCache* GetInstance()//�����ͳɹ�����Ƴ���һ������ģʽ
	{
		return &_sInst;
	}

private:
	SpanList _spanLists[NFREELIST];//Ҳ���������������Ͱ��������һ����,Ҳ����˵��thread cache���Ǽ���Ͱ,��central cache��Ҳ�Ǽ���Ͱ

private:
	//����ģʽ�����ñ��˹����������͹���˽��
	CentralCache()//��ϣͰ�ı����Զ�������
	{}

	//Ϊ�˷�ֹ�õ�����ȥ��������,ҲҪ�ѿ����������
	CentralCache(const CentralCache&) = delete;//c++11��ɾ�������﷨,����Ϊ�˷�ֹ���õ�CentralCache�Ķ���ȥ��������,����ֱ��ɾ���������캯��

	static CentralCache _sInst;//��ͷ�ļ��ж��庯����ʵ��,���ܻᱻ��ΰ�����ɴ���
};



//ϸ�ڽ���:
//ps1:ÿ���̶߳��ж����ThreadCache,����ÿ�����̶�ֻ����һ��centralcache
//ps2:������˭��centralcache��ʱ��,���ʺϽ�centralcache��Ƴɵ���ģʽ
//ps3:����� Central Cache ʱ���������Ϊ����ģʽ����Ҫԭ����ȷ������ϵͳ��ֻ����һ�� Central Cache ��ʵ���������������¼���ô���
//ps4:��֤��ȫ����Դ��ͳһ����Central Cache ��һ�����ڻ������ݵ���Ҫ�����������ֶ��ʵ��������ѵõ�ͳһ�Ļ������ݣ�����ɻ��ҺͲ���Ҫ�Ĵ���
//ps5:������ϵͳ���������û��ʹ�õ���ģʽ����ôÿ����Ҫ���� Central Cache ��ʱ����Ҫ����һ���µ�ʵ��������������ϵͳ�Ŀ���



//����ģʽ:
//ps1:����ģʽ��Ҫ����ҪĿ�ľ��Ǳ�֤������Ӧ�ó�����ֻ�ܴ���һ�����ʵ��,�����ṩȫ�ַ������ʵ���ķ���
//ps2:��������ģʽͨ����Ҫ�Լ�������������������
//ps3:�˴��ĵ���ģʽ�Ƕ���ģʽ






