// ProgramServer4.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include "ProgramServer4.h"
#include "function.h"

//全局变量
select_queue_manage_list queue_list_table[SOCK_QUEUE_NUM];
uint8_t queue_status_table[SOCK_QUEUE_NUM] = { 0 };//已经置1的，置0在进程中进行
WSAData wsa;



int main(int argc, char* argv[])
{
	SOCKET s, newsock;
	struct sockaddr_in ser_addr, remote_addr;
	int len;
	uint8_t queue_status_pos=0;

	DWORD dwThreadId, dwThrdParam;
	HANDLE hThread;
	proc_param param,*ptr;
		
	WSAStartup(0x101, &wsa);
	ptr = &param;
	s = socket(AF_INET, SOCK_STREAM, 0);
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	ser_addr.sin_port = htons(0x1234);
	bind(s, (sockaddr*)&ser_addr, sizeof(ser_addr));
	unsigned long arg = 1;
	listen(s, 5);

	while (1) {
	start:
		len = sizeof(remote_addr);
		newsock = accept(s, (sockaddr*)&remote_addr, &len);
		if (newsock == -1) {
			break;
		}
		ioctlsocket(newsock, FIONBIO, &arg);
		printf("accept a connection , socket = %d\n",newsock);
		param.newsock = newsock;
		param.queue_index = queue_status_pos;
		dwThrdParam = (DWORD)&param;

		//选一个进程和一个槽位
		while(queue_status_table[queue_status_pos] == QUEUE_FULL)
		{
			queue_status_pos++;
			if (queue_status_pos >= SOCK_QUEUE_NUM)
			{
				char buf[] = "Server busy, try later\n";
				printf("check old thread");
				queue_status_pos = 0;
				send(newsock, buf, sizeof(buf), 0);
				closesocket(newsock);
				goto start;
			}
		}

		if (queue_list_table[queue_status_pos].queue_proc_set == PROC_OFF)
		{
			hThread = CreateThread(
				NULL,                        // no security attributes 
				0,                           // use default stack size  
				server_proc,                  // thread function 
				&param,                // argument to thread function 
				0,                           // use default creation flags 
				&dwThreadId);                // returns the thread identifier 
				queue_list_table[queue_status_pos].proc_index = dwThreadId;
				queue_list_table[queue_status_pos].queue_proc_set = PROC_ON;
		}
		//将套接字加入某个队列
		printf("# NEWSOCK!!!======================== SOCKET:%d ====== IN QUEUE:%d ========================\n", newsock, queue_status_pos);
		queue_list_table[queue_status_pos].sock_queue[queue_list_table[queue_status_pos].sock_num]=newsock;
		queue_list_table[queue_status_pos].sock_num++;
		if (queue_list_table[queue_status_pos].sock_num >= SOCK_QUEUE_SIZE)
		{
			queue_list_table[queue_status_pos].queue_status = QUEUE_FULL;
			queue_status_table[queue_status_pos] = QUEUE_FULL;
		}

	}
	closesocket(s);
	WSACleanup();
	return 0;
}
