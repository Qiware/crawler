#if !defined(__FRWD_H__)
#define __FRWD_H__

#if defined(__RTTP_SUPPORT__)
#include "rtsd_cli.h"
#include "rtsd_ssvr.h"
#else /*__RTTP_SUPPORT__*/
#include "sdsd_cli.h"
#include "sdsd_ssvr.h"
#endif /*__RTTP_SUPPORT__*/

#define AGTD_SHM_SENDQ_PATH     "../temp/agentd/send.shmq"  /* 发送队列路径 */

/* 配置信息 */
typedef struct
{
#if defined(__RTTP_SUPPORT__)
    rtsd_conf_t sdtp;               /* SDTP配置 */
#else /*__RTTP_SUPPORT__*/
    sdsd_conf_t sdtp;               /* SDTP配置 */
#endif /*__RTTP_SUPPORT__*/
} frwd_conf_t;

/* 全局对象 */
typedef struct
{
    frwd_conf_t conf;               /* 配置信息 */
    log_cycle_t *log;               /* 日志对象 */
    shm_queue_t *send_to_agentd;    /* 发送至Agentd */
#if defined(__RTTP_SUPPORT__)
    rtsd_cntx_t *sdtp;              /* SDTP对象 */
#else /*__RTTP_SUPPORT__*/
    sdsd_cntx_t *sdtp;              /* SDTP对象 */
#endif /*__RTTP_SUPPORT__*/
} frwd_cntx_t;

#endif /*__FRWD_H__*/
