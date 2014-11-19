#include <windows.h>
#include <stdio.h>
#include <math.h>

//把一条正弦曲线0~2pi 之间的弧度等分200份抽样，计算每个点的振幅
//然后每隔300ms设置下一个抽样点,并让cpu工作对应振幅时间
const int samplingCount = 200; //抽样点数目
const double pi = 3.1415926;
const int totalAmplitude = 300; //每个抽样点对应时间片
const double delta = 2.0/samplingCount;  //抽样弧度的增量

int busySpan[samplingCount];//每个抽样点对应的busy时间
int idleSpan[samplingCount];//每个抽样点对应的idle时间

//一个线程调用MakeUsageSin，并把该线程绑定到一个cpu，那么该cpu呈现正弦曲线
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

//如果cpuindex < 0 则所有cpu都显示正弦曲线
//否则只有第 cpuindex个cpu显示正弦曲线
//cpuindex 从 0 开始计数
void CpuSin(int cpuIndex)
{
	//计算 busySpan 和 idleSpan
	double radian = 0;
	int amplitude = totalAmplitude / 2;
	for (int i = 0; i < samplingCount; i++)
	{
		busySpan[i] = (DWORD)(amplitude + sin(pi*radian)*amplitude);
		idleSpan[i] = totalAmplitude - busySpan[i];
		radian += delta;
	}

	//获取系统cup数量
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
			SetThreadAffinityMask(threads[i], 1<<i);//线程指定在某个cpu运行
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