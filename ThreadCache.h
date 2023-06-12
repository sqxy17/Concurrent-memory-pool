#pragma once
//ǰ�ԣ��̻߳����ǹ�ϣͰ�ṹ��ÿ��Ͱ��һ����Ͱλ��ӳ���С���ڴ����������������ÿ���̶߳�����һ��thread cache��������ÿ���߳��������ȡ������ͷŶ�����������

//���������������кõ�С���ڴ档ʹ�����������Ŀռ��ʱ����ͷɾ��С���ڴ���ս���������ʱ��ͷ��
//�����ڴ����ʵ����Ϊ�˽���̶����ȴ�С�ı����������ڴ�ػ���Ϊ�˽����ͬ��С�����ĳ���
//����Ҳ����˵�����ڴ���кܶ�������������ͬ��С�Ľ�����������

//�Ͷ����ڴ����Щ���ƣ���������ڴ���һ����Χ������С��8�ֽڸ�8�ֽڣ�8~16�ֽڵĸ�16,16~32�ֽڵĸ�32.......
//���ǻ����һ���Ŀռ��˷ѣ�������ֻ��Ҫ5���ֽڵĿռ䣬���Ǹ���8���ֽڵģ���Զ��ճ���3���ֽڣ������������ֽھͽ�������Ƭ
//��ʵ�������������ڴ�أ�Ҳ���ǹ�ϣ��ӳ��




#include"Common.h"

class ThreadCache//�����ǵ������̻߳���
{
public:
	//------------------------------------------------��ܵ�������Ҫ����ɺ���:������ͷ��ڴ�-------------------------------------
	void* Allocate(size_t size);

	void Deallocate(void* ptr, size_t size);

	//�����������̻߳�����������ͷſռ䣬�����������Ŀռ䳬���˻���ʣ��Ŀռ䲻�㣬��������Ļ����ȡ����
	void* FetchFromCentralCache(size_t index, size_t size);

	// ��������ʱ�������ڴ�ص����Ļ���
	void ListTooLong(FreeList& list, size_t size);

private:
	FreeList _freeLists[NFREELIST];//�� ���͵�����
};


static _declspec(thread) ThreadCache* pTLSThreadCache=nullptr; // �����߸��������˱���Ϊ�߳��ڲ�ʹ�ã�ÿ���̶߳���copyһ�ݸ�������
//ps1:_declspec(thread)���������ı�������ÿ���߳��ж������һ��
//ps2:_declspec(thread)��MSVC��Visual C++���ṩ���﷨��չ����������һ���������ֲ߳̾��洢���ڶ��̳߳����У�ÿ���̶߳���һ���Լ��������ڴ�ռ䣬�洢���߳���������ݣ���Ϊ�ֲ߳̾��洢����
// �������ĺô��ǿ��Ա����߳�֮������ݾ����ͳ�ͻ������˳����Ч�ʺͰ�ȫ�ԡ�
// 
//ps3:����δ����У�ThreadCache* pTLSThreadCache��һ��ָ�����͵ı�������ָ��һ����ΪThreadCache����Ķ���
//����ʹ����_declspec(thread)�﷨�����ÿ���̶߳��ᴴ��һ��ThreadCache���󣬲��Ҹö���ֻ���ڵ�ǰ�߳��б����ʡ�ͨ�����ַ�ʽ�����Ա�����̳߳����ж�ͬһThreadCache����ľ����ͳ�ͻ���Ӷ�����˳����Ч�ʺͲ������ܡ�


