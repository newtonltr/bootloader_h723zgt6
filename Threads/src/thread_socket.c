#include "thread_socket.h"
#include <stdio.h>
#include <string.h>
#include "fast_iap.h"

// TCP socket相关参数定义在这个文件中
// 用于和上位机进行bin文件传输协议
NX_TCP_SOCKET tcp_socket;
#define TCP_SERVER_PORT 7000  // 服务器监听端口

// 用于打印日志
NX_TCP_SOCKET log_socket;
#define LOG_SERVER_PORT 8000  // 日志服务器端口


// 该函数不直接发送数据，而是通过threadx的queue_log(在别的文件进行初始化)发送数据的地址到日志线程，日志线程等待到队列后再提取数据发送
UINT fi_socket_log(char* format, ...)
{
    uint32_t status = 0;
    return status;
}

// 线程入口函数，该线程用于fast-iap的socket通信
void thread_socket_entry(ULONG thread_input)
{
    UINT status;
    NX_PACKET *receive_packet;
    ULONG bytes_read;
    struct fast_iap_t *fi = &fast_iap;
    
    // 创建TCP服务器套接字
    status = nx_tcp_socket_create(&ip_0, &tcp_socket, "TCP Server Socket", 
                                 NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 
                                 1024, NX_NULL, NX_NULL);
    if (status != NX_SUCCESS)
    {
        return;
    }
    
    // 绑定TCP套接字到服务器端口
    status = nx_tcp_server_socket_listen(&ip_0, TCP_SERVER_PORT, &tcp_socket, 5, NX_NULL);
    if (status != NX_SUCCESS)
    {
        nx_tcp_socket_delete(&tcp_socket);
        return;
    }
    
    while (1) {
        // 等待客户端连接
        status = nx_tcp_server_socket_accept(&tcp_socket, NX_WAIT_FOREVER);
        if (status != NX_SUCCESS)
        {
            nx_tcp_server_socket_unaccept(&tcp_socket);
            nx_tcp_server_socket_unlisten(&ip_0, TCP_SERVER_PORT);
            nx_tcp_socket_delete(&tcp_socket);
            return;
        }

		global_boot_stat = BOOT_STAT_SOCKET;

        while (1)
        {
            // 接收数据包
            status = nx_tcp_socket_receive(&tcp_socket, &receive_packet, NX_WAIT_FOREVER);
            if (status == NX_SUCCESS) {
                // 读取数据包内容
                status = nx_packet_data_retrieve(receive_packet, fi->buffer, &bytes_read);
                if (status == NX_SUCCESS && bytes_read > 0) {
                    // 处理接收到的bin文件数据

                }
                // 释放数据包
                nx_packet_release(receive_packet);
            } else if (status == NX_NOT_CONNECTED) {
                // 接受新连接
                nx_tcp_server_socket_unaccept(&tcp_socket);
                nx_tcp_server_socket_relisten(&ip_0, TCP_SERVER_PORT, &tcp_socket);
                break;
            }
        }  
    }
}

void thread_fast_iap(ULONG thread_input)
{
    while (1) {

    }
}

// 无限等待queue_log,queue_log承载着数据的地址，本线程负责建立tcp服务器，等待客户端连接，
// 并将数据通过log_socket端口发送出去
// 不接收数据，只做发送，但同样做断连检测并做重连
void thread_log_server(ULONG thread_input)
{
    while (1) {

    }
}
