#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/random.h>
#include <ncurses.h>
#include "argon2.h"

#define FILEPATH "passwords.csv"
#define MAXLEN 50
#define HASHLEN 16
#define SALTLEN 16
#define MAXPASSWORDS 256

typedef struct Map{
    char *key[MAXPASSWORDS];
    char *value[MAXPASSWORDS];
    size_t size;
    int ptr;
}Map;

/*Function Declarations*/
FILE* login(void);
Map* alloc_map(void);
int load_file(FILE *fptr, Map *map);
int get_line(char *prompt, char *buffer, size_t bufsize);
int read_credentials(FILE *fptr, char *key_buffer, char *password_buffer);
int hash_password(char *password_buffer, char* hashed_password);
int gen_hash(uint8_t salt_buffer, size_t salt_len);
int map_insert(Map *map, char *key, char *value);
char* gen_password(char *buf, size_t bufsize);
char* map_get(Map *map, char *key);
void print_map(Map *map);

/**
 *  NOTES:
 *  1 - Nao faz sentido em algumas funcoes escrever no BUF e retornar o BUF
 *  2 - Tratamento de erro PESSIMO, escolher retornos adequados e dar handle neles em um nivel mais alto (caller)
 *  3 - Map pode ficar lento quando muito grande por conta do jeito que eh feito
*/

int
main()
{
    FILE *fptr;
    Map *map;

    fptr = login();
    if(fptr == NULL){
        printf("\nerror: Could not open or create file. Aborting\n");
        return -1;
    }
    map = alloc_map();
    if(map == NULL){
        printf("error: map couldn't be allocated");
        return -1;
    }

    load_file(fptr, map);
    print_map(map);

    free(fptr);
    free(map);
    return 0;
}

/*Application Logic Functions */

//Refatorar sem duvidas
FILE*
login(void)
{
    char input_username[MAXLEN];
    char input_password[MAXLEN];
    char username[MAXLEN];
    char password[MAXLEN];
    char hashed_password[MAXLEN];
    int err;

    FILE *fptr = fopen(FILEPATH, "r");

    if(!fptr){
        fptr = fopen(FILEPATH, "w");
        if(!fptr){
            return NULL;
        }
        printf("Register:\n");
    }

    err = get_line("username: ", input_username, sizeof(input_username));
    err = get_line("password: ", input_password, sizeof(input_password));

    read_credentials(fptr, username, password);
    hash_password(password, hashed_password);


    //trocar para wrong password or username depois de testar se ta tudo funcionando
    if(strcmp(input_username, username) != 0){
        printf("\nWrong username");
        return NULL;
    }

    if(strcmp(input_password, password) != 0){
        printf("\nWrong password");
        return NULL;
    }

    //printf("User Logged in\n");
    return fptr;
}

//Nao faz sentido escrever no buf e retornar buf
char*
gen_password(char *buf, size_t bufsize)
{
    ssize_t bytes;
    
    bytes = getrandom(buf, sizeof(char)*bufsize, GRND_NONBLOCK);

    if(bytes == -1){
        return NULL;
    }

    for(int i = 0; (size_t)i < sizeof(buf); i++){
        buf[i] = buf[i] % 126;

        if(buf[i] < 0){
            buf[i] *= -1;
        }
        if(buf[i] <= 32){
            buf[i] += 33;
        }
    }
    return buf;
}

int
gen_salt(uint8_t salt_buf[], size_t saltlen)
{
    ssize_t bytes;

    bytes = getrandom(salt_buf, sizeof(uint8_t)*saltlen, GRND_NONBLOCK);

    if(bytes == -1){
        return -1;
    }

    return 0;
}

int
hash_password(char *password, char *hashed_password)
{
    uint8_t hash1[HASHLEN];
    uint8_t salt[SALTLEN];
    uint32_t t_cost = 2;        //n pass computatiom
    uint32_t m_cost = (1 << 16);//memory usage
    uint32_t parallelism = 1;   //threads
    uint32_t password_len = strlen((char*)password);

    if(gen_salt(salt, SALTLEN) == -1){
        return -1;
    }
    //possivel usar memset para isso? memset(salt, RANDOM, saltlen)
    
    //argon2id_hash_raw(t_cost, m_cost, parallelism, password, password_len, salt, SALTLEN, hash1, HASHLEN);
    return 0;
}

int
load_file(FILE *fptr, Map *map)
{
    char key_str[MAXLEN];
    char hash[MAXLEN];

    while(read_credentials(fptr, key_str, hash) != EOF){
        map_insert(map, key_str, hash);
    }
    return 0;
}

/* Utils Functions */
int
get_line(char *prompt, char *buffer, size_t bufsize)
{
    int ch, extra;

    if(prompt){
        printf("%s", prompt);
        fflush(stdin);
    }

    if(fgets(buffer, bufsize, stdin) == NULL){
        return 1;
    }

    //Se a input for muito longa, nao vai ter nova linha
    //Nesse caso da flush pro fim da linha de modo que o excesso nao afete proxima call
    if(buffer[strlen(buffer)-1] != '\n'){
        extra = 0;
        while((ch = getchar()) != '\n' && ch != EOF){
            extra = 1;
        }
        return (extra == 1) ? 1 : 0;
    }

    buffer[strlen(buffer)-1] = '\0';
    return 0;
}

int 
read_credentials(FILE *fptr, char *key_buffer, char *passwd_buffer)
{
    char line_buffer[100];
    int i, j;

    if(fscanf(fptr, "%s", line_buffer) == EOF){
        return EOF;
    }

    for(i = 0; line_buffer[i] != ','; i++){
        key_buffer[i] = line_buffer[i];
    }
    key_buffer[i] = '\0';

    i += 1;
    for(j = 0; line_buffer[i] != '\0' && line_buffer[i] != '\n'; i++, j++){
        passwd_buffer[j] = line_buffer[i];
    }
    passwd_buffer[j] = '\0';

    return 0;
}

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
