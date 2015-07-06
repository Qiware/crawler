#if !defined(__RTSD_SEND_H__)
#define __RTSD_SEND_H__

#include "rtsd_ssvr.h"

/* 配置信息 */
typedef struct
{
    int nodeid;                         /* 设备ID: 唯一值 */
    char path[FILE_LINE_MAX_LEN];       /* 工作路径 */

    struct
    {
        char usr[RTMQ_USR_MAX_LEN];     /* 用户名 */
        char passwd[RTMQ_PWD_MAX_LEN];  /* 登录密码 */
    } auth;                             /* 鉴权信息 */

    char ipaddr[IP_ADDR_MAX_LEN];       /* 服务端IP地址 */
    int port;                           /* 服务端端口号 */

    int send_thd_num;                   /* 发送线程数 */
    int work_thd_num;                   /* 工作线程数 */

    size_t send_buff_size;              /* 发送缓存大小 */
    size_t recv_buff_size;              /* 接收缓存大小 */

    rtmq_cpu_conf_t cpu;                /* CPU亲和性配置 */

    queue_conf_t sendq;                 /* 发送队列配置 */
    queue_conf_t recvq;                 /* 接收队列配置 */
} rtsd_conf_t;

#define rtsd_get_sendq_path(conf, tidx, path, size) /* 发送队列路径 */\
    snprintf(path, size, "%s/%d-%d.sq", conf->path, conf->nodeid, tidx)

/* 全局信息 */
typedef struct
{
    rtsd_conf_t conf;                   /* 配置信息 */
    log_cycle_t *log;                   /* 日志对象 */
    slab_pool_t *slab;                  /* 内存池对象 */

    thread_pool_t *sendtp;              /* 发送线程池 */
    thread_pool_t *worktp;              /* 工作线程池 */

    rtmq_reg_t reg[RTMQ_TYPE_MAX];      /* 回调注册对象(TODO: 如果类型过多，可构造平衡二叉树) */
    queue_t **recvq;                    /* 接收队列(数组长度与send_thd_num一致) */
} rtsd_cntx_t;

/* 内部接口 */
int rtsd_ssvr_init(rtsd_cntx_t *ctx, rtsd_ssvr_t *ssvr, int tidx);
void *rtsd_ssvr_routine(void *_ctx);

int rtsd_worker_init(rtsd_cntx_t *ctx, rtmq_worker_t *wrk, int tidx);
void *rtsd_worker_routine(void *_ctx);

rtmq_worker_t *rtsd_worker_get_by_idx(rtsd_cntx_t *ctx, int idx);

/* 对外接口 */
rtsd_cntx_t *rtsd_init(const rtsd_conf_t *conf, log_cycle_t *log);
int rtsd_start(rtsd_cntx_t *ctx);
int rtsd_register(rtsd_cntx_t *ctx, int type, rtmq_reg_cb_t proc, void *args);

#endif /*__RTSD_SEND_H__*/