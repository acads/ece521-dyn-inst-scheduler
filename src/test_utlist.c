#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "utlist.h"

#define LIST_LEN 10
#define LIST_TYPE_0 0
#define LIST_TYPE_1 1
#define LIST_STATE_F    0
#define LIST_STATE_E    1

struct inst {
    uint8_t     state;
    uint8_t     type;
    uint32_t    tag;
};

struct list_dispatch {
    struct inst             data;
    struct list_dispatch    *prev;
    struct list_dispatch    *next;
};

struct list_dispatch    *list = NULL;
struct list_dispatch    *tmp[LIST_LEN];

void
list_init(void)
{
    uint32_t i = 0;

    for (i = 0; i < LIST_LEN; ++i) {
        tmp[i] = calloc(1, sizeof(*tmp[i]));

        if (i % 3) {
            tmp[i]->data.state = LIST_STATE_F;
            tmp[i]->data.type = LIST_TYPE_0;
            tmp[i]->data.tag = i;
        } else {
            tmp[i]->data.state = LIST_STATE_E;
            tmp[i]->data.type = LIST_TYPE_1;
            tmp[i]->data.tag = (i * 4);
        }
    }

    return;
}

int
main(int argv, char **argc)
{
    uint32_t i = 0;
    uint32_t count = 0;
    struct list_dispatch *loc, *loc1;

    list_init();

    for (i = 0; i < LIST_LEN; ++i) {
        DL_APPEND(list, tmp[i]);
    }


    printf("\nbefore delete\n");
    DL_COUNT(list, loc, count);
    printf("# elements in list is %u\n", count);
    DL_FOREACH(list, loc)
        printf("tag %u, state %u, type %u\n", loc->data.tag, loc->data.state, loc->data.type);

    DL_FOREACH_SAFE(list, loc, loc1) {
        if (!(loc->data.tag % 3)) {
            DL_DELETE(list, loc);
            free(loc);
        }
    }
    
    printf("\nafter delete\n");
    DL_COUNT(list, loc, count);
    printf("# elements in list is %u\n", count);
    DL_FOREACH(list, loc)
        printf("tag %u, state %u, type %u\n", loc->data.tag, loc->data.state, loc->data.type);

    DL_FOREACH_SAFE(list, loc, loc1) {
        DL_DELETE(list, loc);
        free(loc);
    }

    return 0;
}
