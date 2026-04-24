#ifndef MAP_H
#define MAP_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAXPASSWORDS 256

typedef struct Map{
    char *key[MAXPASSWORDS];
    char *value[MAXPASSWORDS];
    size_t size;
    int ptr;
}Map;

Map* alloc_map(void);
int map_insert(Map *map, char *key, char *value);
char* map_get(Map *map, char *key);
void print_map(Map *map);

#endif
