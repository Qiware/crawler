/******************************************************************************
 ** Coypright(C) 2014-2024 Qiware technology Co., Ltd
 **
 ** 文件名: mem_ref.c
 ** 版本号: 1.0
 ** 描  述: 内存引用技术管理
 ** 作  者: # Qifeng.zou # 2016年06月27日 星期一 21时19分37秒 #
 ******************************************************************************/
#include "comm.h"
#include "rb_tree.h"
#include "spinlock.h"

/* 内存引用项 */
typedef struct
{
    void *addr;         // 内存地址
    uint64_t count;     // 引用次数
} mem_ref_item_t;

/* 内存引用SLOT */
typedef struct
{
    spinlock_t lock;    // 内存锁
    rbt_tree_t *tab;    // 内存引用表
} mem_ref_slot_t;

/* 全局对象 */
typedef struct
{
#define MEM_REF_SLOT_NUM (55)
    mem_ref_slot_t slot[MEM_REF_SLOT_NUM];
} mem_ref_cntx_t;

static mem_ref_cntx_t g_mem_ref_ctx; // 全局对象

static uint64_t mem_ref_key_cb(const void *key, size_t len) { return (uint64_t)key; }
static inline int mem_ref_cmp_cb(const void *key, const void *data) { return 0; }

/******************************************************************************
 **函数名称: mem_ref_init
 **功    能: 初始化内存引用计数表
 **输入参数: NONE
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2016.06.29 14:45:15 #
 ******************************************************************************/
int mem_ref_init(void)
{
    int idx;
    mem_ref_slot_t *slot;
    mem_ref_cntx_t *ctx = &g_mem_ref_ctx;

    memset(ctx, 0, sizeof(mem_ref_cntx_t));

    for (idx=0; idx<MEM_REF_SLOT_NUM; ++idx) {
        slot = &ctx->slot[idx];
        spin_lock_init(&slot->lock);
        slot->tab = (rbt_tree_t *)rbt_creat(NULL,
                (key_cb_t)mem_ref_key_cb, (cmp_cb_t)mem_ref_cmp_cb);
        if (NULL == slot->tab) {
            return -1;
        }
    }

    return 0;
}

/******************************************************************************
 **函数名称: mem_ref_incr
 **功    能: 增加1次引用
 **输入参数:
 **     addr: 内存地址
 **输出参数: NONE
 **返    回: 内存池对象
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2014.09.08 #
 ******************************************************************************/
int mem_ref_incr(void *addr)
{
    int idx;
    mem_ref_slot_t *slot;
    mem_ref_item_t *item;
    mem_ref_cntx_t *ctx = &g_mem_ref_ctx;

    idx = (uint64_t)addr % MEM_REF_SLOT_NUM;
    slot = &ctx->slot[idx];

    spin_lock(&slot->lock);

    item = (mem_ref_item_t *)rbt_query(slot->tab, addr, sizeof(addr));
    if (NULL != item) {
        ++item->count;
        spin_unlock(&slot->lock);
        return 0;
    }

    item = (mem_ref_item_t *)calloc(1, sizeof(mem_ref_item_t));
    if (NULL == item) {
        spin_unlock(&slot->lock);
        return -1;
    }

    item->addr = addr;
    ++item->count;

    if (rbt_insert(slot->tab, addr, sizeof(addr), item)) {
        spin_unlock(&slot->lock);
        free(item);
        return -1;
    }

    spin_unlock(&slot->lock);
    return 0;
}

/******************************************************************************
 **函数名称: mem_ref_sub
 **功    能: 减少1次引用
 **输入参数:
 **     addr: 内存地址
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2016.06.29 14:53:09 #
 ******************************************************************************/
int mem_ref_sub(void *addr)
{
    int idx;
    mem_ref_slot_t *slot;
    mem_ref_item_t *item;
    mem_ref_cntx_t *ctx = &g_mem_ref_ctx;

    idx = (uint64_t)addr % MEM_REF_SLOT_NUM;
    slot = &ctx->slot[idx];

    spin_lock(&slot->lock);

    item = (mem_ref_item_t *)rbt_query(slot->tab, addr, sizeof(addr));
    if (NULL == item) {
        spin_unlock(&slot->lock);
        return 0; // Didn't find
    }

    if (0 == --item->count) {
        rbt_delete(slot->tab, addr, sizeof(addr), (void **)&item);
        spin_unlock(&slot->lock);
        free(item->addr);
        free(item);
        return 0;
    }

    spin_unlock(&slot->lock);
    return 0;
}