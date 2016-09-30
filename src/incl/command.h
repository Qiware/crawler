/******************************************************************************
 ** Copyright(C) 2014-2024 Qiware technology Co., Ltd
 **
 ** 文件名: search.h
 ** 版本号: 1.0
 ** 描  述: 搜索消息协议的定义
 ** 作  者: # Qifeng.zou # Fri 08 May 2016 17:53:30 PM CST #
 ******************************************************************************/
#if !defined(__SEARCH_H__)
#define __SEARCH_H__

#include "uri.h"
#include "mesg.h"

/* 消息类型 */
typedef enum
{
    MSG_TYPE_UNKNOWN                    /* 未知消息 */

    , MSG_PING                          /* PING-请求 */
    , MSG_PONG                          /* PONG-应答 */

    , MSG_QUERY_CONF_REQ                /* 查询配置信息-请求 */
    , MSG_QUERY_CONF_RSP                /* 反馈配置信息-应答 */

    , MSG_QUERY_WORKER_STAT_REQ         /* 查询工作信息-请求 */
    , MSG_QUERY_WORKER_STAT_RSP         /* 反馈工作信息-应答 */

    , MSG_QUERY_WORKQ_STAT_REQ          /* 查询工作队列信息-请求 */
    , MSG_QUERY_WORKQ_STAT_RSP          /* 反馈工作队列信息-应答 */

    , MSG_SWITCH_SCHED_REQ              /* 切换调度-请求 */
    , MSG_SWITCH_SCHED_RSP              /* 反馈切换调度信息-应答 */

    , MSG_SUB_REQ                       /* 订阅-请求 */
    , MSG_SUB_RSP                       /* 订阅-应答 */

    , MSG_TYPE_TOTAL                    /* 消息类型总数 */
} mesg_type_e;

/* 搜索消息结构 */
#define SRCH_WORD_LEN       (128)
typedef struct
{
    char words[SRCH_WORD_LEN];          /* 搜索关键字 */
} mesg_search_req_t;

////////////////////////////////////////////////////////////////////////////////
/* 插入关键字-请求 */
typedef struct
{
    char word[SRCH_WORD_LEN];           /* 关键字 */
    char url[URL_MAX_LEN];              /* 关键字对应的URL */
    int freq;                           /* 频率 */
} mesg_insert_word_req_t;

/* 插入关键字-应答 */
typedef struct
{
#define MESG_INSERT_WORD_FAIL   (0)
#define MESG_INSERT_WORD_SUCC   (1)
    int code;                           /* 应答码 */
    char word[SRCH_WORD_LEN];           /* 关键字 */
} mesg_insert_word_rsp_t;

#define mesg_insert_word_resp_hton(rsp) do { \
    (rsp)->code = htonl((rsp)->code); \
} while(0)

#define mesg_insert_word_resp_ntoh(rsp) do { \
    (rsp)->code = ntohl((rsp)->code); \
} while(0)

////////////////////////////////////////////////////////////////////////////////
/* 订阅-请求 */
typedef struct
{
    mesg_type_e type;                   /* 订阅类型(消息类型) */
    int nid;                            /* 结点ID */
} mesg_sub_req_t;

#define mesg_sub_req_hton(req) do { /* 主机 -> 网络 */\
    (req)->type = htonl((req)->type); \
    (req)->nid = htonl((req)->nid); \
} while(0)

#define mesg_sub_req_ntoh(req) do { /* 网络 -> 主机 */\
    (req)->type = ntohl((req)->type); \
    (req)->nid = ntohl((req)->nid); \
} while(0)

/* 订阅-应答 */
typedef struct
{
    int code;                           /* 应答码 */
    mesg_type_e type;                   /* 订阅类型(消息类型) */
} mesg_sub_rsp_t;

#define mesg_sub_rsp_hton(rsp) do { /* 主机 -> 网络 */\
    (rsp)->code = htonl((rsp)->code); \
    (rsp)->type = htonl((rsp)->type); \
} while(0)

#define mesg_sub_rsp_ntoh(rsp) do { /* 网络 -> 主机 */\
    (rsp)->code = ntohl((rsp)->code); \
    (rsp)->type = ntohl((rsp)->type); \
} while(0)

#endif /*__SEARCH_H__*/
