/******************************************************************************
 ** Coypright(C) 2014-2024 Xundao technology Co., Ltd
 **
 ** 文件名: inverted.c
 ** 版本号: 1.0
 ** 描  述: 倒排索引、倒排文件处理
 **         如: 创建、插入、查找、删除、归并等
 ** 作  者: # Qifeng.zou # 2015.01.29 #
 ******************************************************************************/
#include "invert.h"
#include "syscall.h"

/******************************************************************************
 **函数名称: invert_creat
 **功    能: 创建倒排信息
 **输入参数: 
 **     path: 倒排索引路径
 **     max: 哈希数组大小
 **输出参数: NONE
 **返    回: 倒排对象
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.01.27 #
 ******************************************************************************/
invert_cntx_t *invert_creat(const char *path, int max)
{
    int n, _max;
    invert_cntx_t *ctx;

    ctx = (invert_cntx_t *)calloc(1, sizeof(invert_cntx_t));
    if (NULL == ctx)
    {
        return NULL;
    }

    ctx->max = max;
    snprintf(ctx->path, sizeof(ctx->path), "%s", ctx->path);

    ctx->fd = Open(path, OPEN_FLAGS, OPEN_MODE);
    if (ctx->fd < 0)
    {
        free(ctx);
        return NULL;
    }

    n = read(ctx->fd, &_max, sizeof(int));
    if (n < 0)
    {
        return NULL;
    }

    return ctx;
}

/******************************************************************************
 **函数名称: invert_insert
 **功    能: 插入倒排信息
 **输入参数: 
 **     keyword: 关键字
 **     doc: 包含关键字的文档
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.01.27 #
 ******************************************************************************/
int invert_insert(const char *keyword, const char *doc)
{
    return 0;
}

/******************************************************************************
 **函数名称: invert_query
 **功    能: 查询倒排信息
 **输入参数: 
 **     keyword: 关键字
 **     list: 文档列表
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.01.27 #
 ******************************************************************************/
int invert_query(const char *keyword, list_t *list)
{
    return 0;
}

/******************************************************************************
 **函数名称: invert_remove
 **功    能: 删除倒排信息
 **输入参数: 
 **     keyword: 关键字
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2015.01.27 #
 ******************************************************************************/
int invert_remove(const char *keyword)
{
    return 0;
}
