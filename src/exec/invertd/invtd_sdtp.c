/******************************************************************************
 ** Coypright(C) 2014-2024 Xundao technology Co., Ltd
 **
 ** 文件名: invtd_sdtp.c
 ** 版本号: 1.0
 ** 描  述: 倒排服务中与SDTP相关内容的处理
 ** 作  者: # Qifeng.zou # Fri 08 May 2015 10:37:21 PM CST #
 ******************************************************************************/

#include "mesg.h"
#include "invertd.h"
#include "rtrd_recv.h"

/******************************************************************************
 **函数名称: invtd_search_word_req_hdl
 **功    能: 处理搜索请求
 **输入参数:
 **     type: 消息类型
 **     orig: 源设备ID
 **     buff: 搜索关键字-请求数据
 **     len: 数据长度
 **     args: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 从倒排表中查询结果，并将结果返回给客户端
 **注意事项:
 **作    者: # Qifeng.zou # 2015.05.08 #
 ******************************************************************************/
static int invtd_search_word_req_hdl(int type, int orig, char *buff, size_t len, void *args)
{
    int idx;
    list_node_t *node;
    invt_word_doc_t *doc;
    invt_dic_word_t *word;
    mesg_search_word_rsp_t rsp; /* 应答 */
    invtd_cntx_t *ctx = (invtd_cntx_t *)args;
    mesg_search_word_req_t *req = (mesg_search_word_req_t *)buff; /* 请求 */

    log_trace(ctx->log, "Call %s()! serial:%ld words:%s",
            __func__, req->serial, req->words);

    memset(&rsp, 0, sizeof(rsp));

    req->serial = ntoh64(req->serial);

    /* > 搜索倒排表 */
    pthread_rwlock_rdlock(&ctx->invtab_lock);

    word = invtab_query(ctx->invtab, req->words);
    if (NULL == word
        || NULL == word->doc_list)
    {
        pthread_rwlock_unlock(&ctx->invtab_lock);
        log_error(ctx->log, "Didn't search anything! words:%s", req->words);
        goto INVTD_SRCH_RSP;
    }

    /* > 打印搜索结果 */
    idx = 0;
    node = word->doc_list->head;
    for (; NULL!=node && idx < MSG_SRCH_RSP_URL_NUM; node=node->next, ++idx)
    {
        doc = (invt_word_doc_t *)node->data;

        log_trace(ctx->log, "[%d]: url:%s freq:%d", idx+1, doc->url.str, doc->freq);

        ++rsp.url_num;
        snprintf(rsp.url[idx], sizeof(rsp.url[idx]), "%s:%d", doc->url.str, doc->freq);
    }
    pthread_rwlock_unlock(&ctx->invtab_lock);

INVTD_SRCH_RSP:
    /* > 应答搜索结果 */
    rsp.serial = hton64(req->serial);
    rsp.url_num = htonl(rsp.url_num);
    if (rtrd_cli_send(ctx->rtrd_cli, MSG_SEARCH_WORD_RSP, orig, (void *)&rsp, sizeof(rsp)))
    {
        log_error(ctx->log, "Send response failed! serial:%ld words:%s",
                req->serial, req->words);
    }

    return INVT_OK;
}

/******************************************************************************
 **函数名称: invtd_insert_word_req_hdl
 **功    能: 插入关键字的处理
 **输入参数:
 **     type: 消息类型
 **     orig: 源节点ID
 **     buff: 插入关键字-请求数据
 **     len: 数据长度
 **     args: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项: 源节点ID(orig)将成为应答消息的目的节点ID(dest)
    **作    者: # Qifeng.zou # 2015-06-17 21:37:55 #
 ******************************************************************************/
static int invtd_insert_word_req_hdl(int type, int orig, char *buff, size_t len, void *args)
{
    mesg_insert_word_rsp_t rsp; /* 应答 */
    invtd_cntx_t *ctx = (invtd_cntx_t *)args;
    mesg_insert_word_req_t *req = (mesg_insert_word_req_t *)buff; /* 请求 */

    log_trace(ctx->log, "Call %s()! serial:%ld word:%s url:%s",
            __func__, req->serial, req->word, req->url);

    memset(&rsp, 0, sizeof(rsp));

    req->serial = ntoh64(req->serial);
    req->freq = ntohl(req->freq);

    /* > 插入倒排表 */
    pthread_rwlock_wrlock(&ctx->invtab_lock);
    if (invtab_insert(ctx->invtab, req->word, req->url, req->freq))
    {
        pthread_rwlock_unlock(&ctx->invtab_lock);
        log_error(ctx->log, "Insert invert table failed! serial:%s word:%s url:%s freq:%d",
                req->serial, req->word, req->url, req->freq);
        /* > 设置应答信息 */
        rsp.code = htonl(0); // 失败
        snprintf(rsp.word, sizeof(rsp.word), "%s", req->word);
        goto INVTD_INSERT_WORD_RSP;
    }
    pthread_rwlock_unlock(&ctx->invtab_lock);

    /* > 设置应答信息 */
    rsp.code = htonl(1); // 成功
    snprintf(rsp.word, sizeof(rsp.word), "%s", req->word);

INVTD_INSERT_WORD_RSP:
    /* > 发送应答信息 */
    rsp.serial = hton64(req->serial);
    if (rtrd_cli_send(ctx->rtrd_cli, MSG_INSERT_WORD_RSP, orig, (void *)&rsp, sizeof(rsp)))
    {
        log_error(ctx->log, "Send response failed! serial:%s word:%s url:%s freq:%d",
                req->serial, req->word, req->url, req->freq);
    }

    return INVT_OK;
}

/******************************************************************************
 **函数名称: invtd_print_invt_tab_req_hdl
 **功    能: 处理打印倒排表的请求
 **输入参数:
 **     type: 消息类型
 **     orig: 源设备ID
 **     buff: 搜索请求的数据
 **     len: 数据长度
 **     args: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.05.08 #
 ******************************************************************************/
static int invtd_print_invt_tab_req_hdl(int type, int orig, char *buff, size_t len, void *args)
{
    return INVT_OK;
}

/******************************************************************************
 **函数名称: invtd_rttp_reg
 **功    能: 注册RTTP回调
 **输入参数:
 **     ctx: SDTP对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 进行回调函数的注册
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.05.08 #
 ******************************************************************************/
static int invtd_rttp_reg(invtd_cntx_t *ctx)
{
#define INVTD_RTTP_REG(ctx, type, proc, args) \
    if (rtrd_register((ctx)->rtrd, type, proc, args)) \
    { \
        log_error(ctx->log, "Register callback failed!"); \
        return INVT_ERR; \
    }

   INVTD_RTTP_REG(ctx, MSG_SEARCH_WORD_REQ, invtd_search_word_req_hdl, ctx);
   INVTD_RTTP_REG(ctx, MSG_INSERT_WORD_REQ, invtd_insert_word_req_hdl, ctx);
   INVTD_RTTP_REG(ctx, MSG_PRINT_INVT_TAB_REQ, invtd_print_invt_tab_req_hdl, ctx);

    return INVT_OK;
}

/******************************************************************************
 **函数名称: invtd_start_rttp
 **功    能: 启动RTTP服务
 **输入参数:
 **     ctx: SDTP对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 进行回调函数的注册, 并启动SDTP服务
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.05.08 #
 ******************************************************************************/
int invtd_start_rttp(invtd_cntx_t *ctx)
{
    if (invtd_rttp_reg(ctx))
    {
        return INVT_ERR;
    }

    return rtrd_startup(ctx->rtrd);
}
