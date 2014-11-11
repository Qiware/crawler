/******************************************************************************
 ** Coypright(C) 2014-2024 Xundao technology Co., Ltd
 **
 ** 文件名: crawler.c
 ** 版本号: 1.0
 ** 描  述: 网络爬虫
 **         负责下载指定URL网页
 ** 作  者: # Qifeng.zou # 2014.09.04 #
 ******************************************************************************/
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>


#include "log.h"
#include "lock.h"
#include "hash.h"
#include "common.h"
#include "crawler.h"
#include "syscall.h"
#include "xd_socket.h"
#include "crwl_sched.h"
#include "crwl_worker.h"
#include "crwl_parser.h"

#define CRWL_PROC_LOCK_PATH "../temp/crwl/crwl.lck"

static int crwl_init_workers(crwl_cntx_t *ctx);
int crwl_workers_destroy(crwl_cntx_t *ctx);
static int crwl_domain_cmp(const void *ukey, const void *data);

/******************************************************************************
 **函数名称: crwl_getopt 
 **功    能: 解析输入参数
 **输入参数: 
 **     argc: 参数个数
 **     argv: 参数列表
 **输出参数:
 **     opt: 参数选项
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 解析输入参数
 **     2. 验证输入参数
 **注意事项: 
 **     c: 配置文件路径
 **     h: 帮助手册
 **作    者: # Qifeng.zou # 2014.09.05 #
 ******************************************************************************/
int crwl_getopt(int argc, char **argv, crwl_opt_t *opt)
{
    int ch;

    /* 1. 解析输入参数 */
    while (-1 != (ch = getopt(argc, argv, "c:hd")))
    {
        switch (ch)
        {
            case 'c':   /* 指定配置文件 */
            {
                snprintf(opt->conf_path, sizeof(opt->conf_path), "%s", optarg);
                break;
            }
            case 'd':
            {
                opt->isdaemon = true;
                break;
            }
            case 'h':   /* 显示帮助信息 */
            default:
            {
                return CRWL_SHOW_HELP;
            }
        }
    }

    optarg = NULL;
    optind = 1;

    /* 2. 验证输入参数 */
    if (!strlen(opt->conf_path))
    {
        snprintf(opt->conf_path, sizeof(opt->conf_path), "%s", CRWL_DEF_CONF_PATH);
    }

    return CRWL_OK;
}

/******************************************************************************
 **函数名称: crwl_usage
 **功    能: 显示启动参数帮助信息
 **输入参数:
 **     name: 程序名
 **输出参数: NULL
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.09.11 #
 ******************************************************************************/
int crwl_usage(const char *exec)
{
    printf("\nUsage: %s [-h] [-d] -c <config file> [-l log_level]\n", exec);
    printf("\t-h\tShow help\n"
           "\t-c\tConfiguration path\n\n");
    return CRWL_OK;
}

/******************************************************************************
 **函数名称: crwl_init
 **功    能: 初始化全局信息
 **输入参数: 
 **     path: 配置文件路径
 **     log: 日志对象
 **输出参数: NONE
 **返    回: 全局对象
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.09.04 #
 ******************************************************************************/
crwl_cntx_t *crwl_init(const char *path, log_cycle_t *log)
{
    crwl_cntx_t *ctx;

    /* 1. 判断程序是否已运行 */
    if (0 != crwl_proc_lock())
    {
        log_error(log, "Crawler is running!");
        return NULL;
    }

    /* 2. 创建全局对象 */
    ctx = (crwl_cntx_t *)calloc(1, sizeof(crwl_cntx_t));
    if (NULL == ctx)
    {
        log_error(log, "errmsg:[%d] %s!", errno, strerror(errno));
        return NULL;
    }

    /* 3. 加载配置文件 */
    ctx->conf = crwl_conf_creat(path, log);
    if (NULL == ctx->conf)
    {
        free(ctx);
        log_error(log, "Load configuration failed! path:%s", path);
        return NULL;
    }

    ctx->log = log;
    log_set_level(log, ctx->conf->log.level);
    log2_set_level(ctx->conf->log.level2);

    /* 4. 新建域名IP映射表 */
    ctx->domain_ip_map = hash_tab_init(CRWL_DOMAIN_SLOT_LEN, hash_time33_ex, crwl_domain_cmp);
    if (NULL == ctx->domain_ip_map)
    {
        crwl_conf_destroy(ctx->conf);
        free(ctx);
        log_error(log, "Initialize hash table failed!");
        return NULL;
    }

    limit_file_num(4096);

    /* 5. 创建Worker线程池 */
    if (crwl_init_workers(ctx))
    {
        crwl_conf_destroy(ctx->conf);
        free(ctx);
        log_error(log, "Initialize thread pool failed!");
        return NULL;
    }

    return ctx;
}

/******************************************************************************
 **函数名称: crwl_startup
 **功    能: 启动爬虫服务
 **输入参数: 
 **     ctx: 全局信息
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     设置线程回调
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.09.04 #
 ******************************************************************************/
int crwl_startup(crwl_cntx_t *ctx)
{
    int idx;
    pthread_t tid;
    const crwl_conf_t *conf = ctx->conf;

    /* 1. 设置Worker线程回调 */
    for (idx=0; idx<conf->worker.num; ++idx)
    {
        thread_pool_add_worker(ctx->workers, crwl_worker_routine, ctx);
    }
    
    /* 2. 设置Sched线程回调 */
    if (thread_creat(&tid, crwl_sched_routine, ctx))
    {
        log_error(ctx->log, "Create thread failed!");
        return CRWL_ERR;
    }

    return CRWL_OK;
}

/******************************************************************************
 **函数名称: crwl_init_workers
 **功    能: 初始化爬虫线程池
 **输入参数: 
 **     ctx: 全局信息
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.10.11 #
 ******************************************************************************/
static int crwl_init_workers(crwl_cntx_t *ctx)
{
    int idx, num;
    crwl_worker_t *worker;
    const crwl_worker_conf_t *conf = &ctx->conf->worker;

    /* 1. 创建Worker线程池 */
    ctx->workers = thread_pool_init(conf->num);
    if (NULL == ctx->workers)
    {
        log_error(ctx->log, "Initialize thread pool failed!");
        return CRWL_ERR;
    }

    /* 2. 新建Worker对象 */
    ctx->workers->data =
        (crwl_worker_t *)calloc(conf->num, sizeof(crwl_worker_t));
    if (NULL == ctx->workers->data)
    {
        thread_pool_destroy(ctx->workers);
        log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
        return CRWL_ERR;
    }

    /* 3. 依次初始化Worker对象 */
    for (idx=0; idx<conf->num; ++idx)
    {
        worker = (crwl_worker_t *)ctx->workers->data + idx;

        worker->tidx = idx;

        if (crwl_worker_init(ctx, worker))
        {
            log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
            break;
        }
    }

    if (idx == conf->num)
    {
        return CRWL_OK; /* 成功 */
    }

    /* 4. 释放Worker对象 */
    num = idx;
    for (idx=0; idx<num; ++idx)
    {
        worker = (crwl_worker_t *)ctx->workers->data + idx;

        crwl_worker_destroy(worker);
    }

    free(ctx->workers->data);
    thread_pool_destroy(ctx->workers);

    return CRWL_ERR;
}

/******************************************************************************
 **函数名称: crwl_workers_destroy
 **功    能: 销毁爬虫线程池
 **输入参数: 
 **     ctx: 全局信息
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.10.14 #
 ******************************************************************************/
int crwl_workers_destroy(crwl_cntx_t *ctx)
{
    int idx;
    crwl_worker_t *worker;
    const crwl_worker_conf_t *conf = &ctx->conf->worker;

    /* 1. 释放Worker对象 */
    for (idx=0; idx<conf->num; ++idx)
    {
        worker = (crwl_worker_t *)ctx->workers->data + idx;

        crwl_worker_destroy(worker);
    }

    free(ctx->workers->data);

    /* 2. 释放线程池对象 */
    thread_pool_destroy(ctx->workers);

    ctx->workers = NULL;

    return CRWL_ERR;
}

/******************************************************************************
 **函数名称: crwl_proc_lock
 **功    能: 爬虫进程锁(防止同时启动两个服务进程)
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.10.21 #
 ******************************************************************************/
int crwl_proc_lock(void)
{
    int fd;
    char path[FILE_PATH_MAX_LEN];

    /* 1. 获取路径 */
    snprintf(path, sizeof(path), "%s", CRWL_PROC_LOCK_PATH);

    Mkdir2(path, DIR_MODE);

    /* 2. 打开文件 */
    fd = Open(path, OPEN_FLAGS, OPEN_MODE);
    if(fd < 0)
    {
        return -1;
    }

    /* 3. 尝试加锁 */
    if(proc_try_wrlock(fd) < 0)
    {
        Close(fd);
        return -1;
    }

    return 0;
}

/******************************************************************************
 **函数名称: crwl_get_ip_by_domain
 **功    能: 获取域名对应的IP地址
 **输入参数:
 **     ctx: 全局信息
 **     host: 域名
 **输出参数: NONE
 **返    回: 域名IP映射信息
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.10.21 #
 ******************************************************************************/
crwl_domain_ip_map_t *crwl_get_ip_by_domain(crwl_cntx_t *ctx, char *host)
{
    int ret;
    crwl_domain_ip_map_t *map;
    struct sockaddr_in *sockaddr;
    struct addrinfo *addrinfo, *curr;

CRWL_FETCH_IP_BY_DOMAIN:
    /* 1. 从域名表中查找IP地址 */
    map = hash_tab_search(ctx->domain_ip_map, host, strlen(host));
    if (NULL == map)
    {
        /* 查找失败则通过getaddrinfo()查询IP地址 */
        if (0 != getaddrinfo(host, NULL, NULL, &addrinfo))
        {
            log_error(ctx->log, "Get address info failed! host:%s", host);
            return NULL;
        }

        /* 申请域名信息：此空间插入哈希表中(此处不释放空间) */
        map = (crwl_domain_ip_map_t *)calloc(1, sizeof(crwl_domain_ip_map_t));
        if (NULL == map)
        {
            freeaddrinfo(addrinfo);

            log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
            return NULL;
        }

        snprintf(map->host, sizeof(map->host), "%s", host);
        map->ip_num = 0;

        curr = addrinfo;
        while (NULL != curr
            && map->ip_num < CRWL_IP_MAX_NUM)
        {
            sockaddr = (struct sockaddr_in *)curr->ai_addr;
            if (0 == sockaddr->sin_addr.s_addr)
            {
                curr = curr->ai_next;
                continue;
            }

            inet_ntop(AF_INET,
                    &sockaddr->sin_addr.s_addr,
                    map->ip[map->ip_num],
                    sizeof(map->ip[map->ip_num]));
            ++map->ip_num;

            curr = curr->ai_next;
        }

        freeaddrinfo(addrinfo);

        ret = hash_tab_insert(ctx->domain_ip_map, host, strlen(host), map);
        if (0 != ret)
        {
            free(map);

            if (AVL_NODE_EXIST == ret)
            {
                log_debug(ctx->log, "Domain is exist! host:[%s]", host);
                goto CRWL_FETCH_IP_BY_DOMAIN;
            }

            log_error(ctx->log, "Insert into hash table failed! ret:[%x/%x] host:[%s]",
                    ret, AVL_NODE_EXIST, host);
            return NULL;
        }
    }

    return map;
}

/******************************************************************************
 **函数名称: crwl_get_ip_by_domain
 **功    能: 获取域名对应的IP地址
 **输入参数:
 **     ctx: 全局信息
 **     host: 域名
 **输出参数: NONE
 **返    回: 获取域名对应的地址信息
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.10.21 #
 ******************************************************************************/
static int crwl_domain_cmp(const void *_domain, const void *data)
{
    const char *host = (const char *)_domain;
    const crwl_domain_ip_map_t *map = (const crwl_domain_ip_map_t *)data;

    return strcmp(host, map->host);
}

/******************************************************************************
 **函数名称: crwl_init_log
 **功    能: 初始化日志模块
 **输入参数:
 **     fname: 日志文件名
 **输出参数: NONE
 **返    回: 获取域名对应的地址信息
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2014.10.21 #
 ******************************************************************************/
log_cycle_t *crwl_init_log(char *fname)
{
    log_cycle_t *log;
    char path[FILE_NAME_MAX_LEN];

    /* 1. 初始化系统日志 */
    log2_get_path(path, sizeof(path), basename(fname));

    if (log2_init(LOG_LEVEL_ERROR, path))
    {
        fprintf(stderr, "Init log2 failed!");
        return NULL;
    }

    /* 2. 初始化业务日志 */
    log_get_path(path, sizeof(path), basename(fname));

    log = log_init(LOG_LEVEL_ERROR, path);
    if (NULL == log)
    {
        log2_error("Initialize log failed!");
        log2_destroy();
        return NULL;
    }

    return log;
}