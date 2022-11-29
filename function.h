#pragma once
#define SOCK_QUEUE_SIZE		100
#define SOCK_QUEUE_NUM		50
#define QUEUE_FULL			1
#define QUEUE_IDLE			0
#define PROC_ON				1
#define PROC_OFF			0
#define FD_SETSIZE			SOCK_QUEUE_SIZE		
#include "stdio.h"
#include <stdint.h>
#include "winsock.h"

typedef struct select_queue_manage_list {
	DWORD	proc_index = 0;
	uint8_t queue_proc_set = PROC_OFF;
	uint8_t queue_status = QUEUE_IDLE;
	uint8_t sock_num = 0;
	SOCKET	sock_queue[SOCK_QUEUE_SIZE] = { 0 };
}select_queue_manage_list;

typedef struct recv_buf {
	SOCKET sock;
	char* ptr;
	char* next;
}recv_buf;

typedef struct proc_param {
	SOCKET newsock = 0;
	uint8_t queue_index = 0;
};

DWORD WINAPI server_proc(LPVOID lpParam);
DWORD WINAPI server_proc_2(LPVOID lpParam);
void make_fdlist(select_queue_manage_list *list, fd_set* fd_list);
void delete_list(SOCKET s, select_queue_manage_list* list);