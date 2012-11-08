#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>
#include <asm/ioctls.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/param.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#include "common.h"
#include "porting.h"
#include "multicast.h"
#include "mid_push.h"
#include "sqlite.h"
#include "softdmx.h"

static char s_data_source[64];

#define MULTI_BUF_POINTER_MOVE(p,len) (p)=(((p)+(len))%MULTI_BUF_SIZE)

typedef struct{
	unsigned char buf[MULTI_BUF_SIZE];
	unsigned int p_read;
	unsigned int p_write;
	int full_flag;	// 当p_read等于p_write时，必须用这个flag指明buf目前是空闲还是充满。0表示有空闲（部分空闲或全部空闲）
}MULTI_BUF;

static int p_read;
static int p_write;
static unsigned char *p_buf = NULL;
extern int loader_dsc_fid;

static pthread_mutex_t mtx_getip = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_getip = PTHREAD_COND_INITIALIZER;
static int igmp_running = 0;
static int softdvb_running = 0;
static pthread_t pth_softdvb_id;
static pthread_t pth_igmp_id;


static pthread_mutex_t mtx_rely_condition = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_rely_condition = PTHREAD_COND_INITIALIZER;
static int s_rely_condition = 0;
static int s_igmp_restart = 0;
static int s_igmp_recvbuf_init_flag = 0;

static int multicast_init()
{
	p_read = 0;
	p_write = 0;
	
	return 0;
}

static int multicast_uninit()
{
	p_read = 0;
	p_write = 0;
	
	return 0;
}

int igmp_init()
{
	memset(s_data_source, 0, sizeof(s_data_source));
	if(-1==data_source_get(s_data_source, sizeof(s_data_source)-1)){
		DEBUG("has no data source to process, exit from %s()\n", __FUNCTION__);
		return -1;
	}
	multicast_init();
	if(-1==multicast_add(s_data_source)){
		DEBUG("igmp join failed\n");
		return -1;
	}
	return 0;
}

int igmp_uninit()
{
	if(0==igmp_running){
		net_rely_condition_set(RELY_CONDITION_EXIT);
		
		pthread_mutex_lock(&mtx_getip);
		pthread_cond_signal(&cond_getip);
		pthread_mutex_unlock(&mtx_getip);
	}
	/* 
	 soft_dvb_thread中要使用全局变量p_buf，因此要先停止soft_dvb，然后停止igmp（释放p_buf）
	*/
	softdvb_running = 0;
	igmp_running = 0;
	pthread_join(pth_igmp_id, NULL);
	
	return multicast_uninit();
}

int igmp_recvbuf_init()
{
	DEBUG("s_igmp_recvbuf_init_flag=%d\n", s_igmp_recvbuf_init_flag);
	
	if(0==s_igmp_recvbuf_init_flag){
		if(p_buf){
			DEBUG("free p_buf: %p\n", p_buf);
			free(p_buf);
			p_buf = NULL;
		}
		p_buf = (unsigned char *)malloc(MULTI_BUF_SIZE);
		if(NULL==p_buf){
			ERROROUT("can not malloc space for p_buf\n");
			return -1;
		}
		s_igmp_recvbuf_init_flag = 1;
		DEBUG("malloc %d for igmp receive buffer\n", MULTI_BUF_SIZE);
	}
	else{
		DEBUG("have malloc igmp receive buffer already\n");
	}
	return 0;
}

static void *igmp_thread()
{
    char if_ip[16] = {0};
	int sock, opt;
	struct sockaddr_in sin;
	int sizeof_sin = -1;
    int recv_len, dfree;
    struct ip_mreq ipmreq;
    int multicast_failed_sleep = 7;
    
	struct timeval now;
	struct timespec outtime;
	int retcode = 0;
	
	char multi_ip[16];
	int multi_port = 3000;

MULTITASK_START:
	pthread_mutex_lock(&mtx_rely_condition);
	pthread_cond_wait(&cond_rely_condition,&mtx_rely_condition); //wait
	pthread_mutex_unlock(&mtx_rely_condition);
	
	/*
	 只要具备网络条件即可启动组播业务，不需要等待硬盘。因为升级不需要硬盘，有flash即可。
	*/
	if(RELY_CONDITION_NET & s_rely_condition)
		DEBUG("network condition is ready\n");
	else if(s_rely_condition&RELY_CONDITION_EXIT){
		DEBUG("exit from here by external action\n");
		return NULL;
	}
	else{
		DEBUG("net rely condition is not ready, %d\n", s_rely_condition);
		goto MULTITASK_START;
	}
	
	sleep(2);
	DEBUG("igmp thread will goto its main loop\n");
	
	memset(multi_ip, 0, sizeof(multi_ip));
	if(-1==igmp_simple_check(s_data_source, multi_ip, &multi_port)){
		DEBUG("check igmp addr: %s invalid\n", s_data_source);
		goto MULTITASK_START;
	}
	DEBUG("multicast ip: %s, port: %d\n", multi_ip, multi_port);
	
	
	while(1){
		while(1){
			pthread_mutex_lock(&mtx_getip);
			
			memset(if_ip, 0, sizeof(if_ip));
			if(0==ifconfig_get("eth0", if_ip, NULL, NULL)){
				if(0==ipv4_simple_check(if_ip)){
					pthread_mutex_unlock(&mtx_getip);
					break;
				}
				else
					DEBUG("ip(%s) of eth0 is invalid\n", if_ip);
			}
			else{
				DEBUG("get eth0 ip failed\n");
			}
			
			gettimeofday(&now, NULL);
			outtime.tv_sec = now.tv_sec + 7;
			outtime.tv_nsec = now.tv_usec;
			retcode = pthread_cond_timedwait(&cond_getip, &mtx_getip, &outtime);
			if(ETIMEDOUT!=retcode){
				DEBUG("igmp thread is canceled by external signal\n");
				pthread_mutex_unlock(&mtx_getip);
				return NULL;
			}
			pthread_mutex_unlock(&mtx_getip);
		}
		DEBUG("get eth0 ip: %s\n", if_ip);
	
		bzero((char *)&sin, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = inet_addr( multi_ip );
		sin.sin_port = htons(multi_port);
	
		if ((sock = socket( AF_INET, SOCK_DGRAM, 0)) == -1) {
			DEBUG("Error opening igmp socket\n");
		}
		else{
			DEBUG("create igmp socket %d\n", sock);
		
			opt = 1;
			if( ioctl( sock,  FIONBIO, (int)&opt ) < 0 ){
				DEBUG("set nonblock mode failed!\n");
			}
			else{
				if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) < 0) {
					DEBUG("setsockopt(allow multiple socket use) failed\n");
				}
				else{
					if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
						DEBUG("call to bind failed\n");
					}
					else{
						DEBUG("set FIONBIO, setsockopt(SO_REUSEADDR), bind, ok\n");
					
						opt = 1024 * 1280;
						if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void*)&opt, sizeof(opt)) < 0){
							DEBUG("Can't change system network size (wanted size = %d)\n", opt);
						}
						/*
						opt = 1316 * 8;
						if (osex_setsockopt(sock, SOL_SOCKET, SO_RCVLOWAT, (void*)&opt, sizeof(opt)) < 0){
							TDY_PERROR("Can't change system csv  lowat(wanted size = %d)", opt);
						}
						*/
						/*
						if(1 == yx_pppoe_status()){
							if( osex_ipaddr_get("pppoe0", if_ip)==0 ){
								ipmreq.imr_multiaddr.s_addr = inet_addr(multiaddr);
								ipmreq.imr_interface.s_addr = inet_addr(if_ip);
								TDY_DEBUG(("PPPOE IPTV ENTER %s\n", inet_ntoa((struct in_addr)(ipmreq.imr_interface))));
								if (osex_setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&ipmreq, sizeof(ipmreq)) < 0){
									TDY_PERROR("Error in setsocket(add membership)");
									osex_close( sock );
									return -1;
								}
								return sock;
							}		
						}
						*/
					
						ipmreq.imr_multiaddr.s_addr = inet_addr(multi_ip);
						ipmreq.imr_interface.s_addr = inet_addr(if_ip);
						if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&ipmreq, sizeof(ipmreq)) < 0){
							DEBUG("Error in setsocket(add membership)\n");
						}
						else
							break;
					}
				}
			}
		}
		
		pthread_mutex_lock(&mtx_getip);
		multicast_failed_sleep = multicast_failed_sleep << 1;
		if(multicast_failed_sleep>60*60)
			multicast_failed_sleep = 60*60;
			
		gettimeofday(&now, NULL);
		outtime.tv_sec = now.tv_sec + multicast_failed_sleep;
		outtime.tv_nsec = now.tv_usec;
		retcode = pthread_cond_timedwait(&cond_getip, &mtx_getip, &outtime);
		if(ETIMEDOUT!=retcode){
			DEBUG("igmp thread is canceled by external signal\n");
			pthread_mutex_unlock(&mtx_getip);
			return NULL;
		}
		pthread_mutex_unlock(&mtx_getip);
	}
	DEBUG("add membership OK\n");

	sizeof_sin = sizeof(sin);
	
	if(-1==igmp_recvbuf_init())
		return NULL;
	
	igmp_running = 1;
	s_igmp_restart = 0;
	while(1==igmp_running){
        if (p_write >= p_read)
        {
        	dfree = MULTI_BUF_SIZE - p_write;
        }
        else
        {
        	dfree = p_read - p_write - 1;  //not let p_write = p_read 
        }
		recv_len = recvfrom(sock, p_buf+p_write, dfree, 0, (struct sockaddr *)&sin, (socklen_t*)&sizeof_sin);
		if( recv_len > 0 )
		{
			//DEBUG("igmp recv len: %d\n:", recv_len);
			p_write += recv_len;
			if (p_write >= MULTI_BUF_SIZE)
				p_write -= MULTI_BUF_SIZE;
			//DEBUG("recv_len=%d\n", recv_len);
			//multi_buf_write(buf, recv_len);
		}
		
		if (recv_len < 1024)
		{
            usleep(10000);
            if(1==s_igmp_restart){
            	DEBUG("will restart igmp thread loop\n");
            	break;
            }
		}
	}
	
	// do igmp leave
	if(sock>=0){
		if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&ipmreq, sizeof(ipmreq)) < 0) {
			perror("Error in setsocket(add membership)");
		}
		else
			DEBUG("do igmp leave\n");
	
		close(sock);
		DEBUG("close igmp socket: %d\n", sock);
	}
	sock = -1;
	
	if(1==s_igmp_restart)
		goto MULTITASK_START;
	
	free(p_buf);
	p_buf = NULL;

	DEBUG("igmp join thread exit\n");
	return NULL;
}

void net_rely_condition_set(int rely_cond)
{
	s_igmp_restart = 1;
	
	pthread_mutex_lock(&mtx_rely_condition);
	int tmp_cond = s_rely_condition;
	if(rely_cond>=0 || (rely_cond<0 && s_rely_condition>0))
		s_rely_condition += rely_cond;
	DEBUG("net origine %d, set %d, so s_rely_condition is %d\n", tmp_cond, rely_cond, s_rely_condition);
	pthread_cond_signal(&cond_rely_condition);
	pthread_mutex_unlock(&mtx_rely_condition);
}

void *softdvb_thread()
{
	int left = 0;
	
	softdvb_running = 1;
	
	/*
	由于加入组播组是在一个线程中进行的，容易出现加入组播工作还未完毕，这里就已经开始判断igmp_running，从而导致错误退出。
	所以这里延迟一下判断。
	*/
	int i = 0;
	for(i=0; i<100; i++){
		if(1!=softdvb_running)
			break;
		
		if(1==igmp_running)
			break;
		else
			usleep(100000);
	}
	
	/*
	 组播任务的开启、关闭会根据网络情况处理，这里就不再判断igmp_running了，要不然逻辑很复杂。
	*/
	while(1==softdvb_running)	// make sure the igmp thread is start firstly
	{
		if(p_write >= p_read)
			left = p_write - p_read;
		else
			left = MULTI_BUF_SIZE - p_read + p_write;
		
//		printf("left: %d\n", left);
		if(left<18800){
			usleep(10000);
			continue;
		}
		
		if(p_buf)
			parse_ts_packet(p_buf,p_write,&p_read);	// make sure 'p_buf' is not NULL
	}
	DEBUG("exit from soft dvb thread\n");
	
	return NULL;
}

/*
此函数目前只能调用一次。
*/
int multicast_add(const char *multi_addr)
{
	if(NULL==multi_addr || 0!=strncasecmp(multi_addr, "igmp://", strlen("igmp://"))){
		DEBUG("this multicast addr is invalid: %s\n", multi_addr);
		return -1;
	}
	else
		DEBUG("will join: %s", multi_addr);
	
	int ret = -1;
	// 创建接收线程
	
	if(0==pthread_create(&pth_igmp_id, NULL, igmp_thread, NULL)){
		//pthread_detach(pth_igmp_id);
		DEBUG("create multicast receive thread success\n");
		ret = 0;
	}
	else{
		ERROROUT("create multicast receive thread failed\n");
		ret = -1;
	}
	
	return ret;
}

static int allpid_sqlite_cb(char **result, int row, int column, void *filter_act)
{
	DEBUG("sqlite callback, row=%d, column=%d, filter_act addr: %p\n", row, column, filter_act);
	if(row<1 || NULL==filter_act){
		DEBUG("no record in table, return\n");
		return 0;
	}
	
	int i = 0;
	
	for(i=1;i<row+1;i++)
	{
		unsigned short pid = (unsigned short)(strtol(result[i*column],NULL,0));
		if(1==*((int *)filter_act) && CHANNEL_INEFFECTIVE==atoi(result[i*column+1])){
			int ret = free_filter(pid);
			DEBUG("free pid %d return with %d\n", pid, ret);
		}
	}
	for(i=1;i<row+1;i++)
	{
		DEBUG("PID --- %s:%s --- \n", result[i*column], result[i*column+1]);
		unsigned short pid = (unsigned short)(strtol(result[i*column],NULL,0));
		if(1==*((int *)filter_act) && CHANNEL_EFFECTIVE==atoi(result[i*column+1])){
			int filter = alloc_filter(pid, 1);
			DEBUG("set filter, pid=%d[%s], fid=%d\n", pid, result[i*column], filter);
		}
		else{
			int ret = free_filter(pid);
			DEBUG("free pid %d return with %d\n", pid, ret);
		}
	}
	
	return 0;
}

/*
 初始化及反初始化demux过滤器
 act_flag：	1——初始化demux过滤器
 			0——反初始化demux过滤器
*/
int pid_init(int act_flag)
{
	char sqlite_cmd[256+128];
	int (*sqlite_callback)(char **, int, int, void *) = allpid_sqlite_cb;

	snprintf(sqlite_cmd,sizeof(sqlite_cmd),"SELECT pid,EffectFlag FROM Channel;");
	// 1 means alloc filter
	int filter_act = act_flag;
	return sqlite_read(sqlite_cmd, &filter_act, sqlite_callback);
}

int softdvb_init()
{
	int ret = 0;
	Filter_param param;
	
	chanFilterInit();
	
//	// prog/file
//	unsigned short root_pid = root_channel_get();
//	int filter1 = alloc_filter(root_pid, 0);
//	DEBUG("set dvb filter1, pid=%d, fid=%d\n", root_pid, filter1);
	
	memset(&param,0,sizeof(param));
	param.filter[0] = 0xf0;
	param.mask[0] = 0xff;
	loader_dsc_fid=TC_alloc_filter(0x1ff0, &param, loader_des_section_handle, NULL, 0);
	DEBUG("set upgrade filter1, pid=0x1ff0, fid=%d\n", loader_dsc_fid);
	
//	memset(&param,0,sizeof(param));
//	param.filter[0] = 0x1;
//	param.mask[0] = 0xff;
//	int ca_dsc_fid=TC_alloc_filter(0x1, &param, ca_section_handle, NULL, 0);
//	DEBUG("set ca filter1, pid=0x1, fid=%d\n", ca_dsc_fid);
	
#ifdef PUSH_LOCAL_TEST
	// prog/video
	unsigned short video_pid = 123;
	int filter5 = alloc_filter(video_pid, 1);
	DEBUG("set dvb filter3, pid=%d, fid=%d\n", video_pid, filter5);
	
	// prog/file
	unsigned short file_pid = 654;
	int filter4 = alloc_filter(file_pid, 1);
	DEBUG("set dvb filter3, pid=%d, fid=%d\n", file_pid, filter4);
	
	// prog/audio
	unsigned short audio_pid = 8123;
	int filter3 = alloc_filter(audio_pid, 1);
	DEBUG("set dvb filter3, pid=%d, fid=%d\n", audio_pid, filter3);
#else
	if(-1==pid_init(1)){
		DEBUG("allpid init faild\n");
		return -1;
	}
#endif
	
	if(0==pthread_create(&pth_softdvb_id, NULL, softdvb_thread, NULL)){
		//pthread_detach(pth_softdvb_id);
		DEBUG("create soft dvb thread success\n");
		ret = 0;
	}
	else{
		ERROROUT("create multicast receive thread failed\n");
		ret = -1;
	}
	
	return ret;
}

int softdvb_uninit()
{
	int ret = 0;
	
	softdvb_running = 0;
	pthread_join(pth_softdvb_id, NULL);
	
	// prog/file
	unsigned short root_pid = root_channel_get();
	ret = free_filter(root_pid);
	DEBUG("free pid %d return with %d\n", root_pid, ret);
	
#ifdef PUSH_LOCAL_TEST
	// prog/video
	unsigned short video_pid = 123;
	ret = free_filter(video_pid);
	DEBUG("free pid %d return with %d\n", video_pid, ret);
	
	// prog/file
	unsigned short file_pid = 654;
	ret = free_filter(file_pid);
	DEBUG("free pid %d return with %d\n", file_pid, ret);
	
	// prog/audio
	unsigned short audio_pid = 8123;
	ret = free_filter(audio_pid);
	DEBUG("free pid %d return with %d\n", audio_pid, ret);
#else
	if(-1==pid_init(0)){
		DEBUG("allpid init faild\n");
		return -1;
	}	
#endif

	return ret;
}
