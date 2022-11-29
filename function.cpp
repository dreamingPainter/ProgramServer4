#include "function.h"
//多进程+select+扩展select+超出的不能发送的数据用链表解决，子进程自己解决自己

//全局变量
extern select_queue_manage_list queue_list_table[SOCK_QUEUE_NUM];
extern uint8_t queue_status_table[SOCK_QUEUE_NUM];//已经置1的，置0在进程中进行




DWORD __stdcall server_proc(LPVOID lpParam)
{
	fd_set readfds, writefds;
	proc_param *param_ptr = (proc_param*)lpParam;
	SOCKET sock;
	int retval,len;
	timeval timeout;
	select_queue_manage_list* queue_ptr = &queue_list_table[param_ptr->queue_index];
	char recv_array[4096];
	recv_buf *buffer;

	timeout.tv_sec = 0;
	timeout.tv_usec = 1;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	while (1)
	{
		//是否还有需要管理的套接字
		if (queue_ptr->sock_num == 0)
		{
			queue_status_table[queue_ptr->proc_index] = PROC_OFF;
			queue_ptr->sock_num = 0;
			memset(queue_ptr->sock_queue, 0, SOCK_QUEUE_SIZE);
			break;
		}
		//读写队列赋值
		make_fdlist(queue_ptr, &readfds);
		make_fdlist(queue_ptr, &writefds);
		retval = select(0, &readfds, &writefds, NULL, &timeout);
		if (retval == SOCKET_ERROR)
			printf("GetLastError is:%d\n", GetLastError());
		//循环检查套接字队列
		for (int count = 0; count < SOCK_QUEUE_SIZE; count++)
		{
			if (queue_ptr->sock_queue[count] == 0)
				continue;
			sock = queue_ptr->sock_queue[count];
			if (FD_ISSET(sock, &readfds))
			{
				//收了就发
				retval = recv(sock, recv_array, sizeof(recv_array), 0);
				if (retval == 0) {
					closesocket(sock);
					printf("close socket:%d\n", sock);
					queue_ptr->sock_num--;
					delete_list(sock, queue_ptr);
					continue;
				}
				else if (retval == SOCKET_ERROR) {
					retval = WSAGetLastError();
					if (retval == WSAEWOULDBLOCK)
						continue;
					closesocket(sock);
					printf("close socket:%d\n",sock);
					delete_list(sock, queue_ptr);
					continue;
				}
				recv_array[retval] = 0;
				printf("recv:%d\t\t bytes\n", retval);
				send(sock, recv_array, retval, 0);
			}
			if (FD_ISSET(sock, &writefds))
			{
				
			}
		}
	}
	return 0;
}



DWORD __stdcall server_proc_2(LPVOID lpParam) {
	SOCKET sock;
	char buf[129];
	int retval;

	sock = *(SOCKET*)lpParam;
	//与客户机进行通信
	retval = recv(sock, buf, sizeof(buf), 0);
	buf[retval] = 0;
	retval = send(sock, buf, sizeof(buf), 0);

	return 0;
}

void make_fdlist(select_queue_manage_list *list, fd_set* fd_list)
{
	int count;
	for (count = 0; count < SOCK_QUEUE_NUM; count++) {			//加入套接字列表
		if (list->sock_queue[count] > 0) {
			FD_SET(list->sock_queue[count], fd_list);
		}
	}
}

void delete_list(SOCKET s, select_queue_manage_list* list)
{
	int i;
	for (i = 0; i < 64; i++) {
		if (list->sock_queue[i] == s) {
			list->sock_queue[i] = 0;
			list->sock_num -= 1;
			break;
		}
	}
}