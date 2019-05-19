#include<iostream>
#include"Client.h"
#include<thread>
#include<chrono>
#include<stdio.h>
#include<atomic>
#include"TimeStamp.h"

#define CLIENT_COUNT 1000
bool g_bRun = true;
void cmdThread()
{
	while (true)
	{
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (strcmp(cmdBuf, "exit") == 0)
		{
			g_bRun = false;
			printf("Client exit.\n");
			break;
		}
		else
		{
			printf("Invalid input, please re-enter.\n");
		}
	}
}

const int client_count = CLIENT_COUNT;//客户端数量
Client* client[client_count];//客户端数组
const int thread_count = 4;//发送线程的数量
std::atomic_int sendCount(0);
std::atomic_int readyCount(0);

void recvThread(int begin, int end)//1-4，四个线程
{
	while (g_bRun)
	{
		for (int i = begin; i < end; ++i)
		{
			client[i]->onRun();
		}
	}
}

void sendThread(int id)//1-4，四个线程
{
	printf("thread<%d>,start\n", id);
	int c = client_count / thread_count;
	int begin = (id - 1) * c;
	int end = id * c;
	for (int i = begin; i < end; ++i)
	{
		client[i] = new Client();
	}

	for (int i = begin; i < end; ++i)
	{
		client[i]->connectToServer(IP, PORT);
	}

	printf("thread<%d>,Connect<begin=%d, end=%d>\n", id, begin, end);

	//等待其他线程准备好发送
	++readyCount;
	while (readyCount < thread_count)
	{
		std::chrono::milliseconds t(10);//3000毫秒
		std::this_thread::sleep_for(t);
	}

	std::thread t1(recvThread, begin, end);
	t1.detach();

	netmsg_Login login[10];//提高发送频率，每次发送十个消息包
	for (int i = 0; i < 10; ++i)
	{
		strcpy(login[i].userName, "kzj");
		strcpy(login[i].passWord, "12345");
	}


	const int nLen = sizeof(login);
	while (g_bRun)
	{
		for (int i = begin; i < end; ++i)
		{
			if (client[i]->sendData(login, nLen) != SOCKET_ERROR)
			{
				++sendCount;//发送的数量
			}
		}
	}

	for (int i = begin; i < end; ++i)
	{
		client[i]->closeClient();
		delete client[i];
	}

	printf("thread<%d>,exit\n", id);
}

int main()
{
	//输入线程
	std::thread cmd_t(cmdThread);
	cmd_t.detach();

	//启动发送线程
	for (int i = 0; i < thread_count; ++i)
	{
		std::thread t(sendThread, i + 1);//传递的是线程的编号
		t.detach();
	}

	CellTimeStamp tTime;

	while (g_bRun)
	{
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			//!"std::atomic<int>::atomic(const std::atomic<int>&)": 尝试引用已删除的函数
			//!将类 "std::atomic<int>" 作为可变参数函数的参数的非标准用法
			/*printf("thread<%d>,clients<%d>,time<%lf>,send<%d>\n",
				thread_count, client_count, t, sendCount);*/

			printf("thread<%d>,clients<%d>,time<%lf>,send<%d>\n",
				thread_count, client_count, t, (int)(sendCount.load() / t));

			sendCount = 0;
			tTime.update();
		}

#ifdef _WIN32
		Sleep(1);
#else
		sleep(1);
#endif // _WIN32
	}

	std::cout << "EXIT...." << std::endl;
	return 0;
}