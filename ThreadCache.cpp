#include"ThreadCache.h"
#include"CentralCache.h"//



//������������̻߳����ڲ���Ա������ʵ��
//����ռ��С   ������������ֽ���
//���̻߳����У�һ����208��Ͱ
void* ThreadCache::Allocate(size_t size)//�������size������1,5,8,12.....�ȵȶ��п��ܣ�������ָ���С����ָ���С����
{
	//�˴��Ĺ�ϣͰ��ӳ�������size���ֽ����ĸ���Χ��Ҳ����˵Ͱ��һ����Χ��
	assert(size <= MAX_BYTES);//Ҳ����˵���̵������������ڴ���256kb
	size_t alignSize = SizeClass::RoundUp(size);//alignSize��ʵ��������ڴ�,Ҳ�������϶���
	size_t index = SizeClass::Index(size);//index�������Ϊ��ϣ����±꣬Ҳ���ǵ�ǰ����Ŀռ�Ĵ�С�����ĸ�Ͱ����

	if (!_freeLists[index].Empty())//Thread cache ������ԭ�����ÿ���̻߳�ȡ���Ǹ��Ե�tls����
	{
		return _freeLists[index].Pop();
	}
	else//Ҳ����˵�����Ӧ������������û�У��͵�����һ�������    central cache
	{
		return FetchFromCentralCache(index, alignSize);//���������ֻ�ܸ������Ժ��ʵ�ʳ���
	}
}




// �ͷŶ���ʱ���������ʱ�������ڴ�ص����Ļ���
void ThreadCache::ListTooLong(FreeList& list, size_t size)//���ڴ沢��һ��ȫ������ȥ
{
	void* start = nullptr;
	void* end = nullptr;
	list.PopRange(start, end, list.MaxSize());//�ó���һ�����ڴ�׼������ȥ

	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}




//��Ҫ���յĿռ�       ptr�ǻ��տռ�ĵ�ַ��size�ǻ��տռ�Ĵ�С
void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(size <= MAX_BYTES);
	assert(ptr);

	//�Ҷ�Ӧ����������Ͱ����������ȥ
	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(ptr);

	//�������ȴ���һ������������ڴ�ĳ��ȾͿ�ʼ����һ��list��centralcache                   ----�˴��Ǽ򻯰�,û��ϸ�ڿ����̻߳�����ܴ�С�볬�����ٷ����ڴ�
	if (_freeLists[index].Size() >= _freeLists[index].MaxSize())
	{
		// �ͷŶ���ʱ���������ʱ�������ڴ�ص����Ļ���
		ListTooLong(_freeLists[index], size);
	}

}




void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)//�����������ThreadCache��û��û�湩����,�ͻ�������������centralcache����
{
	//С�ֽڵ�����ռ�,���Զ��һЩ�ڴ��,���ֽڵ�����ռ�,�����ٸ�һЩ�ڴ��      -----����õ�������ʼ�����㷨
	//batch:����
	//�������һ��������,��һ��Ҫ��һ��,�ڶ���Ҫ������,������Ҫ������.....  ������಻���512��
	//1.Ҳ����˵�ʼ������CentralCacheҪ̫��,���Ҫ���ڴ��������������ò���
	//2.sizeԽ��,һ����CentralCacheҪ��batchNum��ԽС
	//3.���sizeԽС,һ����CentralCacheҪ��batchNum��Խ��
	size_t batchNum = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));//��������������Ļ���������ڴ��ĸ���,���̻߳����ʹ��
	if (_freeLists[index].MaxSize() == batchNum)
	{
		_freeLists[index].MaxSize() += 1;//�����������,���Ա��Ϊ345....
	}

	void* start = nullptr;
	void* end = nullptr;//�����������Ļ����е�һ��span�и�ɵ�С�ڴ����׵�ַβ�ص�ַ

	//span����������̻߳��������������,�Ȱ����span��ʣ����ڴ���ж��ٸ�����
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);//actualʵ��,Ҳ�����̻߳���ʵ�������ڴ�������
	assert(actualNum > 0);//���ٵ���1��

	if (actualNum == 1)//���ʵ��ֻ��ȡ��һ��,��һ���ڴ�鱻��ȥʹ����,����
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


