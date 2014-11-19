#include <windows.h>
#include <stdio.h>
#include <math.h>

//��һ����������0~2pi ֮��Ļ��ȵȷ�200�ݳ���������ÿ��������
//Ȼ��ÿ��300ms������һ��������,����cpu������Ӧ���ʱ��
const int samplingCount = 200; //��������Ŀ
const double pi = 3.1415926;
const int totalAmplitude = 300; //ÿ���������Ӧʱ��Ƭ
const double delta = 2.0/samplingCount;  //�������ȵ�����

int busySpan[samplingCount];//ÿ���������Ӧ��busyʱ��
int idleSpan[samplingCount];//ÿ���������Ӧ��idleʱ��

//һ���̵߳���MakeUsageSin�����Ѹ��̰߳󶨵�һ��cpu����ô��cpu������������
DWORD WINAPI MakeUsageSin(LPVOID lpParameter)
{
	DWORD startTime = 0;
	for(int j = 0; ; j = (j + 1) % samplingCount)
	{
		startTime = GetTickCount();
		while ((GetTickCount() - startTime) < busySpan[j]);
		Sleep(idleSpan[j]);
	}
}

//���cpuindex < 0 ������cpu����ʾ��������
//����ֻ�е� cpuindex��cpu��ʾ��������
//cpuindex �� 0 ��ʼ����
void CpuSin(int cpuIndex)
{
	//���� busySpan �� idleSpan
	double radian = 0;
	int amplitude = totalAmplitude / 2;
	for (int i = 0; i < samplingCount; i++)
	{
		busySpan[i] = (DWORD)(amplitude + sin(pi*radian)*amplitude);
		idleSpan[i] = totalAmplitude - busySpan[i];
		radian += delta;
	}

	//��ȡϵͳcup����
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	int num_processors = SysInfo.dwNumberOfProcessors;
	if(cpuIndex + 1 > num_processors)
	{
		printf("error: the index of cpu is out of boundary\n");
		printf("cpu number: %d\n", num_processors);
		printf("your index: %d\n", cpuIndex);
		printf("** tip: the index of cpu start from 0 **\n");
		return;
	}

	if(cpuIndex < 0)
	{
		HANDLE* threads = new HANDLE[num_processors];
		for (int i = 0;i < num_processors;i++)
		{
			DWORD mask = 1<<i;
			threads[i] = CreateThread(NULL, 0, MakeUsageSin, &mask, 0, NULL);
			SetThreadAffinityMask(threads[i], 1<<i);//�߳�ָ����ĳ��cpu����
		}
		WaitForMultipleObjects(num_processors, threads, TRUE, INFINITE);
	}
	else
	{
		HANDLE thread;
		DWORD mask = 1;
		thread = CreateThread(NULL, 0, MakeUsageSin, &mask, 0, NULL);
		SetThreadAffinityMask(thread, 1<<cpuIndex);
		WaitForSingleObject(thread,INFINITE);
	}

}
int main()
{

	CpuSin(0);
	return 0;
}