#include "thread_init.h"
#include "nx_stm32_eth_driver.h"
#include "thread_socket.h"
#include "fast_iap.h"

// ---------thread parameters
// thread init parameters
#define THREAD_INIT_STACK_SIZE		4096u
#define THREAD_INIT_PRIO			28u
TX_THREAD thread_init_block;
uint64_t thread_init_stack[THREAD_INIT_STACK_SIZE/8];
void thread_init(ULONG input);

// thread socket parameters
#define THREAD_SOCKET_STACK_SIZE    4096u
#define THREAD_SOCKET_PRIO          25u
TX_THREAD thread_socket_block;
uint64_t thread_socket_stack[THREAD_SOCKET_STACK_SIZE/8];

// thread log server parameters
#define THREAD_LOG_SERVER_STACK_SIZE    4096u
#define THREAD_LOG_SERVER_PRIO          24u
TX_THREAD thread_log_server_block;
uint64_t thread_log_server_stack[THREAD_LOG_SERVER_STACK_SIZE/8];

// thread fast iap parameters
#define THREAD_FAST_IAP_STACK_SIZE    4096u
#define THREAD_FAST_IAP_PRIO          23u
TX_THREAD thread_fast_iap_block;
uint64_t thread_fast_iap_stack[THREAD_FAST_IAP_STACK_SIZE/8];

// tx queue
#define QUEUE_LOG_MESSAGE_SIZE		4	// 队列里每一条消息占多少个 ULONG (1-16)。
#define QUEUE_LOG_MESSAGE_NUM		10	// 队列里最多有多少条消息
#define QUEUE_LOG_QUEUE_SIZE		QUEUE_LOG_MESSAGE_NUM*QUEUE_LOG_MESSAGE_SIZE*8	// area的总字节数
TX_QUEUE queue_log;
ULONG queue_log_area[QUEUE_LOG_QUEUE_SIZE/8];

// queue socket buffer
#define QUEUE_SOCKET_MESSAGE_SIZE	4	// 队列里每一条消息占多少个 ULONG (1-16)。
#define QUEUE_SOCKET_MESSAGE_NUM	10	// 队列里最多有多少条消息
#define QUEUE_SOCKET_BUFFER_QUEUE_SIZE	QUEUE_SOCKET_MESSAGE_NUM*QUEUE_SOCKET_MESSAGE_SIZE*8	// area的总字节数
TX_QUEUE queue_socket_buffer;
ULONG queue_socket_buffer_area[QUEUE_SOCKET_BUFFER_QUEUE_SIZE/8];

// ---------netxduo parameters
NX_PACKET_POOL    pool_0;
NX_IP             ip_0;
#define NX_PACKET_POOL_SIZE ((1536 + sizeof(NX_PACKET)) * 8)
ULONG  packet_pool_area[NX_PACKET_POOL_SIZE/4 + 4] __attribute__((section(".NetXPoolSection")));
ULONG  arp_space_area[52*20 / sizeof(ULONG)] __attribute__((section(".NetXPoolSection")));

#define IP_ADDR0                        192
#define IP_ADDR1                        168
#define IP_ADDR2                        0
#define IP_ADDR3                        202

ULONG  ip0_address = IP_ADDRESS(IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);

#define  THREAD_NETX_IP0_PRIO0                          2u
#define  THREAD_NETX_IP0_STK_SIZE                     	1024*16u
static   uint64_t  thread_netx_ip0_stack[THREAD_NETX_IP0_STK_SIZE/8];

// ---------
void  tx_application_define(void *first_unused_memory)
{
	UINT nx_init_status = 0;

	HAL_ETH_DeInit(&heth);
	nx_system_initialize();
	nx_init_status |= nx_packet_pool_create(&pool_0,
									"NetX Main Packet Pool",
									1536,  (ULONG*)(((int)packet_pool_area + 15) & ~15) ,
									NX_PACKET_POOL_SIZE);
	nx_init_status |= nx_ip_create(&ip_0,
						"NetX IP0",
						ip0_address,
						0xFFFFFF00UL,
						&pool_0, nx_stm32_eth_driver,
						(UCHAR*)thread_netx_ip0_stack,
						sizeof(thread_netx_ip0_stack),
						THREAD_NETX_IP0_PRIO0);
	nx_init_status |= nx_arp_enable(&ip_0, (void *)arp_space_area, sizeof(arp_space_area));
	nx_init_status |= nx_ip_fragment_enable(&ip_0);
	nx_init_status |= nx_tcp_enable(&ip_0);
	nx_init_status |= nx_udp_enable(&ip_0);
	nx_init_status |= nx_icmp_enable(&ip_0);

	ULONG gateway_ip = ip0_address;
	gateway_ip = (gateway_ip & 0xFFFFFF00) | 0x01;
	nx_ip_gateway_address_set(&ip_0, gateway_ip);

	sleep_ms(300);

	// tx queue create
    // 创建日志队列
	tx_queue_create(&queue_log,
					"queue_log",
					QUEUE_LOG_MESSAGE_SIZE,
					queue_log_area,
					QUEUE_LOG_QUEUE_SIZE);
    // 创建socket缓冲区队列
	tx_queue_create(&queue_socket_buffer,
					"queue_socket_buffer",
					QUEUE_SOCKET_MESSAGE_SIZE,
					queue_socket_buffer_area,
					QUEUE_SOCKET_BUFFER_QUEUE_SIZE);

	tx_thread_create(&thread_init_block,
					"tx_init",
					thread_init,
					0,
					&thread_init_stack[0],
					THREAD_INIT_STACK_SIZE,
					THREAD_INIT_PRIO,
					THREAD_INIT_PRIO,
					TX_NO_TIME_SLICE,
					TX_AUTO_START);

}

uint32_t gloabal_time_ms = 0;
uint32_t global_boot_stat = 0;
void thread_init(ULONG input)  // 将UINT改为ULONG
{
	// 创建socket线程
	tx_thread_create(&thread_socket_block,
		"tx_socket",
		thread_socket_entry,
		0,
		&thread_socket_stack[0],
		THREAD_SOCKET_STACK_SIZE,
		THREAD_SOCKET_PRIO,
		THREAD_SOCKET_PRIO,
		TX_NO_TIME_SLICE,
		TX_AUTO_START);

	// 创建log server线程
	tx_thread_create(&thread_log_server_block,
		"tx_log_server",
		thread_log_server,
		0,
		&thread_log_server_stack[0],
		THREAD_LOG_SERVER_STACK_SIZE,
		THREAD_LOG_SERVER_PRIO,
		THREAD_LOG_SERVER_PRIO,
		TX_NO_TIME_SLICE,
		TX_AUTO_START);

	// 创建fast iap线程
	tx_thread_create(&thread_fast_iap_block,
		"tx_fast_iap",
		thread_fast_iap,
		0,
		&thread_fast_iap_stack[0],
		THREAD_FAST_IAP_STACK_SIZE,
		THREAD_FAST_IAP_PRIO,
		THREAD_FAST_IAP_PRIO,
		TX_NO_TIME_SLICE,
		TX_AUTO_START);

	while (1) {
		gloabal_time_ms += 100;
		if(global_boot_stat == BOOT_STAT_IDLE && gloabal_time_ms >= 8000) {
			// jump_to_app();
		}
		sleep_ms(100);
	}
}


