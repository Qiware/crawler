#include "btree.h"

#define BTREE_M     (3)
#define BTREE_NUM   (10)
#define INPUT_LEN   (32)
#define __AUTO_INPUT__

int main(void)
{
    int ret = 0, key = 0, idx = 0;
    btree_t *btree = NULL;
    char input[INPUT_LEN] = {0};

    ret = btree_creat(&btree, BTREE_M);
    if (0 != ret)
    {
        fprintf(stderr, "[%s][%d] Create btree failed!\n", __FILE__, __LINE__);
        return -1;
    }

    fprintf(stderr, "[%s][%d] Create btree success!\n", __FILE__, __LINE__);

    for(idx=0; idx<BTREE_NUM; idx++)
    {
        btree_insert(btree, random()%50);
    }

	btree_insert(btree, 14);
	btree_insert(btree, 28);
	btree_insert(btree, 34);
	btree_insert(btree, 37);
	btree_insert(btree, 48);
	btree_insert(btree, 39);
	btree_insert(btree, 38);
	btree_insert(btree, 40);

    btree_print(btree);

    while(1)
    {
        memset(input, 0, sizeof(input));
    
    #if defined(__AUTO_INPUT__)
        fprintf(stdout, "Input:");
        scanf(" %s", input);
        key = atoi(input);
    #else
        key = random()%BTREE_NUM;
    #endif

        if ((0 == strcasecmp(input, "q"))
            || (0 == strcasecmp(input, "quit"))
            || (0 == strcasecmp(input, "exit")))
        {
            break;
        }
        else if (0 == strcasecmp(input, "d")
            || 0 == strcasecmp(input, "delete"))
        {
            
            scanf(" %s", input);
            key = atoi(input);

            btree_delete(btree, key);
            btree_print(btree);
            continue;
        }


        ret = btree_insert(btree, key);
        if (0 != ret)
        {
            fprintf(stderr, "[%s][%d] Insert failed!\n", __FILE__, __LINE__);
            break;
        }

        fprintf(stderr, "[%d] Insert btree success!\n", key);

        btree_print(btree);
    }

    btree_destroy(&btree);
    return 0;
}
