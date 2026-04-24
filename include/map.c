#include "map.h"

Map*
alloc_map(void)
{
    Map *map = (Map*)malloc(sizeof(Map));
    if(map == NULL){
        return NULL;
    }
    map->size = MAXPASSWORDS;
    map->ptr = 0;

    return map;
}

int
map_insert(Map *map, char *key, char *value)
{
    if(map->ptr >= MAXPASSWORDS){
        return -1;
    }
    map->key[map->ptr] = strdup(key);
    map->value[map->ptr] = strdup(value);
    map->ptr++;

    return 0;
}

char*
map_get(Map *map, char *key)
{
    int idx;
    for(idx = 0; idx < map->ptr; idx++){
        if(strcmp(key, map->key[idx]) == 0){
            return map->value[idx];
        }
    }
    return NULL;
}

void
print_map(Map *map)
{
    int pwd_len = 0;

    for(int i = 0; i < map->ptr; i++){
        printf("%d. %s\t| ", i+1, map->key[i]);
        pwd_len = strlen(map->value[i]);

        for(int j = 0; j < pwd_len; j++){
            printf("*");
        }
        printf("\n");
    }
}
