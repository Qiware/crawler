#include "syscall.h"
#include "qwsd_send.h"

/******************************************************************************
 **函数名称: qwsd_creat_workers
 **功    能: 创建工作线程线程池
 **输入参数:
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.08.19 #
 ******************************************************************************/
static int qwsd_creat_workers(qwsd_cntx_t *ctx)
{
    int idx;
    qwmq_worker_t *worker;
    thread_pool_opt_t opt;
    qwsd_conf_t *conf = &ctx->conf;

    /* > 创建对象 */
    worker = (qwmq_worker_t *)slab_alloc(ctx->slab, conf->work_thd_num * sizeof(qwmq_worker_t));
    if (NULL == worker)
    {
        log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
        return QWMQ_ERR;
    }

    /* > 创建线程池 */
    memset(&opt, 0, sizeof(opt));

    opt.pool = (void *)ctx->slab;
    opt.alloc = (mem_alloc_cb_t)slab_alloc;
    opt.dealloc = (mem_dealloc_cb_t)slab_dealloc;

    ctx->worktp = thread_pool_init(conf->work_thd_num, &opt, (void *)worker);
    if (NULL == ctx->worktp)
    {
        log_error(ctx->log, "Initialize thread pool failed!");
        slab_dealloc(ctx->slab, worker);
        return QWMQ_ERR;
    }

    /* > 初始化线程 */
    for (idx=0; idx<conf->work_thd_num; ++idx)
    {
        if (qwsd_worker_init(ctx, worker+idx, idx))
        {
            log_fatal(ctx->log, "Initialize work thread failed!");
            slab_dealloc(ctx->slab, worker);
            thread_pool_destroy(ctx->worktp);
            return QWMQ_ERR;
        }
    }

    return QWMQ_OK;
}

/******************************************************************************
 **函数名称: qwsd_creat_sends
 **功    能: 创建发送线程线程池
 **输入参数:
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.08.19 #
 ******************************************************************************/
static int qwsd_creat_sends(qwsd_cntx_t *ctx)
{
    int idx;
    qwsd_ssvr_t *ssvr;
    thread_pool_opt_t opt;
    qwsd_conf_t *conf = &ctx->conf;

    /* > 创建对象 */
    ssvr = (qwsd_ssvr_t *)slab_alloc(ctx->slab, conf->send_thd_num * sizeof(qwsd_ssvr_t));
    if (NULL == ssvr)
    {
        log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
        return QWMQ_ERR;
    }

    /* > 创建线程池 */
    memset(&opt, 0, sizeof(opt));

    opt.pool = (void *)ctx->slab;
    opt.alloc = (mem_alloc_cb_t)slab_alloc;
    opt.dealloc = (mem_dealloc_cb_t)slab_dealloc;

    ctx->sendtp = thread_pool_init(conf->send_thd_num, &opt, (void *)ssvr);
    if (NULL == ctx->sendtp)
    {
        log_error(ctx->log, "Initialize thread pool failed!");
        slab_dealloc(ctx->slab, ssvr);
        return QWMQ_ERR;
    }

    /* > 初始化线程 */
    for (idx=0; idx<conf->send_thd_num; ++idx)
    {
        if (qwsd_ssvr_init(ctx, ssvr+idx, idx))
        {
            log_fatal(ctx->log, "Initialize send thread failed!");
            slab_dealloc(ctx->slab, ssvr);
            thread_pool_destroy(ctx->sendtp);
            return QWMQ_ERR;
        }
    }

    return QWMQ_OK;
}


/******************************************************************************
 **函数名称: qwsd_creat_recvq
 **功    能: 创建接收队列
 **输入参数:
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.06.04 #
 ******************************************************************************/
static int qwsd_creat_recvq(qwsd_cntx_t *ctx)
{
    int idx;
    qwsd_conf_t *conf = &ctx->conf;

    /* > 创建队列对象 */
    ctx->recvq = (queue_t **)calloc(conf->send_thd_num, sizeof(queue_t *));
    if (NULL == ctx->recvq)
    {
        log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
        return QWMQ_ERR;
    }

    /* > 创建接收队列 */
    for (idx=0; idx<conf->work_thd_num; ++idx)
    {
        ctx->recvq[idx] = queue_creat(conf->recvq.max, conf->recvq.size);
        if (NULL == ctx->recvq[idx])
        {
            log_error(ctx->log, "Create recvq failed!");
            return QWMQ_ERR;
        }
    }

    return QWMQ_OK;
}

/******************************************************************************
 **函数名称: qwsd_init
 **功    能: 初始化发送端
 **输入参数:
 **     conf: 配置信息
 **     log: 日志对象
 **输出参数: NONE
 **返    回: 发送对象
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.05.19 #
 ******************************************************************************/
qwsd_cntx_t *qwsd_init(const qwsd_conf_t *conf, log_cycle_t *log)
{
    qwsd_cntx_t *ctx;
    slab_pool_t *slab;

    /* > 创建内存池 */
    slab = slab_creat_by_calloc(30 * MB, log);
    if (NULL == slab)
    {
        log_error(log, "Initialize slab failed!");
        return NULL;
    }

    /* > 创建对象 */
    ctx = (qwsd_cntx_t *)slab_alloc(slab, sizeof(qwsd_cntx_t));
    if (NULL == ctx)
    {
        log_fatal(log, "errmsg:[%d] %s!", errno, strerror(errno));
        slab_destroy(slab);
        return NULL;
    }

    ctx->log = log;
    ctx->slab = slab;

    /* > 加载配置信息 */
    memcpy(&ctx->conf, conf, sizeof(qwsd_conf_t));

    /* > 创建接收队列 */
    if (qwsd_creat_recvq(ctx))
    {
        log_fatal(log, "Create recv-queue failed!");
        slab_destroy(slab);
        return NULL;
    }

    /* > 创建工作线程池 */
    if (qwsd_creat_workers(ctx))
    {
        log_fatal(ctx->log, "Create work thread pool failed!");
        slab_destroy(slab);
        return NULL;
    }

    /* > 创建发送线程池 */
    if (qwsd_creat_sends(ctx))
    {
        log_fatal(ctx->log, "Create send thread pool failed!");
        slab_destroy(slab);
        return NULL;
    }

    return ctx;
}

/******************************************************************************
 **函数名称: qwsd_launch
 **功    能: 启动发送端
 **输入参数:
 **     ctx: 全局信息
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **     1. 创建工作线程池
 **     2. 创建发送线程池
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
int qwsd_launch(qwsd_cntx_t *ctx)
{
    int idx;
    qwsd_conf_t *conf = &ctx->conf;

    /* > 注册Worker线程回调 */
    for (idx=0; idx<conf->work_thd_num; ++idx)
    {
        thread_pool_add_worker(ctx->worktp, qwsd_worker_routine, ctx);
    }

    /* > 注册Send线程回调 */
    for (idx=0; idx<conf->send_thd_num; ++idx)
    {
        thread_pool_add_worker(ctx->sendtp, qwsd_ssvr_routine, ctx);
    }

    return QWMQ_OK;
}

/******************************************************************************
 **函数名称: qwsd_register
 **功    能: 消息处理的注册接口
 **输入参数:
 **     ctx: 全局对象
 **     type: 扩展消息类型 Range:(0 ~ QWMQ_TYPE_MAX)
 **     proc: 回调函数
 **     param: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **     1. 只能用于注册处理扩展数据类型的处理
 **     2. 不允许重复注册
 **作    者: # Qifeng.zou # 2015.05.19 #
 ******************************************************************************/
int qwsd_register(qwsd_cntx_t *ctx, int type, qwmq_reg_cb_t proc, void *param)
{
    qwmq_reg_t *reg;

    if (type >= QWMQ_TYPE_MAX)
    {
        log_error(ctx->log, "Data type [%d] is out of range!", type);
        return QWMQ_ERR;
    }

    if (0 != ctx->reg[type].flag)
    {
        log_error(ctx->log, "Repeat register type [%d]!", type);
        return QWMQ_ERR_REPEAT_REG;
    }

    reg = &ctx->reg[type];
    reg->type = type;
    reg->proc = proc;
    reg->param = param;
    reg->flag = 1;

    return QWMQ_OK;
}
