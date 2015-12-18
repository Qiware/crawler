#if !defined(__QWMQ_COMM_H__)
#define __QWMQ_COMM_H__

#include "sck.h"
#include "log.h"
#include "comm.h"
#include "queue.h"
#include "qwmq_mesg.h"

#define QWMQ_RECONN_INTV        (2)     /* 连接重连间隔 */
#define QWMQ_KPALIVE_INTV       (30)    /* 保活间隔 */
#define QWMQ_BUFF_SIZE          (5 * MB)/* 发送/接收缓存SIZE */

#define QWMQ_SSVR_TMOUT_SEC     (1)     /* SND超时: 秒 */
#define QWMQ_SSVR_TMOUT_USEC    (0)     /* SND超时: 微妙 */

#define QWMQ_WORKER_HDL_QNUM    (2)     /* 各Worker线程负责的队列数 */
#define QWMQ_TYPE_MAX           (0xFF)  /* 自定义数据类型的最大值 */
#define QWMQ_MEM_POOL_SIZE      (30 * MB) /* 内存池大小 */

/* 返回码 */
typedef enum
{
    QWMQ_OK = 0                         /* 成功 */
    , QWMQ_DONE                         /* 处理完成 */
    , QWMQ_RECONN                       /* 重连处理 */
    , QWMQ_AGAIN                        /* 未完成 */
    , QWMQ_SCK_DISCONN                  /* 连接断开 */

    , QWMQ_ERR = ~0x7FFFFFFF            /* 失败 */
    , QWMQ_ERR_CALLOC                   /* Calloc错误 */
    , QWMQ_ERR_QALLOC                   /* 申请Queue空间失败 */
    , QWMQ_ERR_QSIZE                    /* Queue的单元空间不足 */
    , QWMQ_ERR_QUEUE_NOT_ENOUGH         /* 队列空间不足 */
    , QWMQ_ERR_DATA_TYPE                /* 错误的数据类型 */
    , QWMQ_ERR_RECV_CMD                 /* 命令接收失败 */
    , QWMQ_ERR_REPEAT_REG               /* 重复注册 */
    , QWMQ_ERR_TOO_LONG                 /* 数据太长 */
    , QWMQ_ERR_UNKNOWN_CMD              /* 未知命令类型 */
} qwmq_err_e;

/* 鉴权配置 */
typedef struct
{
    int nodeid;                         /* 结点ID */
    char usr[QWMQ_USR_MAX_LEN];         /* 用户名 */
    char passwd[QWMQ_PWD_MAX_LEN];      /* 登录密码 */
} qwmq_auth_conf_t;

/* 接收/发送快照 */
typedef struct
{
    /*  |<------------       size       --------------->|
     *  | 已发送 |          未发送           | 空闲空间 |
     *  | 已处理 |          已接收           | 空闲空间 |
     *   -----------------------------------------------
     *  |XXXXXXXX|///////////////////////////|          |
     *  |XXXXXXXX|///////////////////////////|<--left-->|
     *  |XXXXXXXX|///////////////////////////|          |
     *   -----------------------------------------------
     *  ^        ^                           ^          ^
     *  |        |                           |          |
     * addr     optr                        iptr       end
     */
    char *addr;                         /* 发送缓存 */
    char *end;                          /* 结束地址 */

    size_t size;                        /* 缓存大小 */

    char *optr;                         /* 发送偏移 */
    char *iptr;                         /* 输入偏移 */
} qwmq_snap_t;

#define qwmq_snap_setup(snap, _addr, _size) \
   (snap)->addr = (_addr);  \
   (snap)->end = (_addr) + (_size); \
   (snap)->size = (_size); \
   (snap)->optr = (_addr);  \
   (snap)->iptr = (_addr); 

#define qwmq_snap_reset(snap)           /* 重置标志 */\
   (snap)->optr = (snap)->addr;  \
   (snap)->iptr = (snap)->addr; 

/* 绑定CPU配置信息 */
typedef struct
{
    
    int ison:1;                         /* 是否开启绑定CPU功能 */
    int start:7;                        /* 绑定CPU的起始CPU编号 */
} qwmq_cpu_conf_t;

/* 工作对象 */
typedef struct
{
    int id;                             /* 对象索引 */
    log_cycle_t *log;                   /* 日志对象 */

    int cmd_sck_id;                     /* 命令套接字 */

    int max;                            /* 套接字最大值 */
    fd_set rdset;                       /* 可读套接字集合 */

    uint64_t proc_total;                /* 已处理条数 */
    uint64_t drop_total;                /* 丢弃条数 */
    uint64_t err_total;                 /* 错误条数 */
} qwmq_worker_t;

/******************************************************************************
 **函数名称: qwmq_reg_cb_t
 **功    能: 回调注册类型
 **输入参数:
 **     type: 扩展消息类型 Range:(0 ~ QWMQ_TYPE_MAX)
 **     orig: 源结点ID
 **     data: 数据
 **     len: 数据长度
 **     param: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 ******************************************************************************/
typedef int (*qwmq_reg_cb_t)(int type, int orig, char *data, size_t len, void *param);
typedef struct
{
    int type;                           /* 消息类型. Range: 0~QWMQ_TYPE_MAX */
#define QWMQ_FLAG_UNREG     (0)         /* 0: 未注册 */
#define QWMQ_FLAG_REGED     (1)         /* 1: 已注册 */
    int flag;                           /* 注册标识 
                                            - 0: 未注册
                                            - 1: 已注册 */
    qwmq_reg_cb_t proc;                 /* 回调函数指针 */
    void *param;                        /* 附加参数 */
} qwmq_reg_t;

#endif /*__QWMQ_COMM_H__*/
