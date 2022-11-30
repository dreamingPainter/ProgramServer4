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
	int retval,len,data_size;
	timeval timeout;
	select_queue_manage_list* queue_ptr = &queue_list_table[param_ptr->queue_index];
	char recv_array[1024];
	recv_buf *buffer_head,*buffer_tail,*ptr;
	long unsigned int arg = 1;//套接字设置为非阻塞模式

	timeout.tv_sec = 0;
	timeout.tv_usec = 1;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	//初始化


	//buffer_head 的sock值表示未发出的数据块个数
	buffer_head = (recv_buf*)malloc(sizeof(recv_buf));
	buffer_head->sock = 0;
	buffer_tail = buffer_head;
	ptr = buffer_head;

	while (1)
	{
		retval = 0;
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
			ioctlsocket(sock, FIONBIO, &arg);
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
					if (retval == WSAEWOULDBLOCK|errno==EAGAIN|errno==EINTR)
						continue;
					closesocket(sock);
					printf("close socket:%d\n",sock);
					delete_list(sock, queue_ptr);
					continue;
				}
				recv_array[retval] = 0;
				printf("socket:%d\trecv:%d\t\t bytes\n", sock,retval);
				data_size = retval;
				retval = send(sock, recv_array, retval, 0);

				//对暂时没有发出去的数据的处理
				if (retval == 0)
					printf("socket:%d\tsend nothing\n", sock);
				else if (retval > 0)
					printf("socket:%d\tsend %d\t\tbytes\n", sock, retval);
				else if (retval < 0)
				{
					if (errno == EAGAIN)
					{
						buffer_head->sock++;
						buffer_tail->next = (recv_buf*)malloc(sizeof(recv_buf));
						buffer_tail = buffer_tail->next;

						buffer_tail->sock = sock;
						buffer_tail->len = data_size + 1;
						buffer_tail->ptr = (char*)malloc(sizeof(char) * (data_size + 1));
						strcpy_s(buffer_tail->ptr, data_size + 1, recv_array);
						buffer_tail->next = NULL;
					}
				}
			}

			//增加了很多处理和定位工作，非常糟糕
			if (FD_ISSET(sock, &writefds))
			{
				recv_buf *p;
				ptr = buffer_head;
				for (int count = 0; count < buffer_head->sock; count++)
				{
					if (ptr->next->sock == sock)
					{
						char buf[1024];
						strcpy_s(buf, ptr->next->len,ptr->next->ptr);
						retval = send(sock, buf, ptr->next->len, 0);
						if (retval > 0)
						{
							printf("socket:%d\tsend %d\t\tbytes\n", sock, retval);
							p = ptr->next;
							ptr->next = ptr->next->next;
							free(p);
							
						}
						else if (errno == EAGAIN|errno==EWOULDBLOCK|errno==EINTR)
							break;
					}
					ptr = ptr->next;
				}
			}
		}
		FD_ZERO(&writefds);
		FD_ZERO(&readfds);
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