/******************************************************************************
 ** Coypright(C) 2014-2024 Xundao technology Co., Ltd
 **
 ** 文件名: filter.h
 ** 版本号: 1.0
 ** 描  述: 网页过滤器
 **         负责网页的分析, 过滤
 ** 作  者: # Qifeng.zou # 2014.09.04 #
 ******************************************************************************/
#if !defined(__FILTER_H__)
#define __FILTER_H__

#include "log.h"
#include "redis.h"
#include "queue.h"
#include "hash_tab.h"
#include "gumbo_ex.h"
#include "flt_priv.h"
#include "flt_conf.h"
#include "flt_worker.h"
#include "thread_pool.h"
#include <hiredis/hiredis.h>

#define FLT_REDIS_UNDO_LIMIT_NUM   (20000)


/* 任务对象 */
typedef struct
{
    char path[FILE_PATH_MAX_LEN];           /* 文件路径 */
} flt_task_t;

/* 全局对象 */
typedef struct
{
    log_cycle_t *log;                       /* 日志对象 */
    flt_conf_t *conf;                       /* 配置信息 */
    slab_pool_t *slab;                      /* 内存池 */

    time_t run_tm;                          /* 运行时间 */

    pthread_t sched_tid;                    /* Sched线程ID */
    thread_pool_t *worker_tp;               /* Worker线程池 */
    flt_worker_t *worker;                   /* 工作对象 */

    queue_t *taskq;                         /* 任务队列 */
    redis_cluster_t *redis;                 /* Redis集群 */

    hash_tab_t *domain_ip_map;              /* 域名IP映射表: 通过域名找到IP地址 */
    hash_tab_t *domain_blacklist;           /* 域名黑名单 */
} flt_cntx_t;

flt_cntx_t *flt_init(char *pname, const char *path);
int flt_startup(flt_cntx_t *ctx);
void flt_destroy(flt_cntx_t *ctx);

int flt_get_domain_ip_map(flt_cntx_t *ctx, char *host, flt_domain_ip_map_t *map);

int flt_worker_init(flt_cntx_t *ctx, flt_worker_t *worker, int idx);
int flt_worker_destroy(flt_cntx_t *ctx, flt_worker_t *worker);

#endif /*__FILTER_H__*/