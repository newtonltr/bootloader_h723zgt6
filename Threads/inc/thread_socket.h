#ifndef THREAD_SOCKET_H
#define THREAD_SOCKET_H

#include "main.h"
#include "nx_api.h"

// 函数声明
void thread_socket_entry(ULONG thread_input);
void thread_log_server(ULONG thread_input);
void thread_fast_iap(ULONG thread_input);

// 外部变量声明 - 这些变量在thread_init.c中定义
extern TX_THREAD thread_socket_block;
extern NX_IP ip_0;
extern ULONG ip0_address;
extern NX_PACKET_POOL pool_0;
extern uint32_t global_boot_stat;

#endif // THREAD_SOCKET_H