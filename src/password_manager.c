#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/random.h>
#include <ncurses.h>
#include "map.h"
#include "argon2.h"

#define FILEPATH "passwords.csv"
#define MAXLEN 50
#define HASHLEN 16
#define SALTLEN 16
#define NR 10
#define defer_return(value) do{ status = value; goto defer;} while(0);

/*Function Declarations*/
FILE* login(void);
int load_file(FILE *fptr, Map *map);
int get_line(char *prompt, char *buffer, size_t bufsize);
int read_credentials(FILE *fptr, char *key_buffer, char *password_buffer);
int encrypt_password(char *password_buffer, char* hashed_password);
int gen_key(char *plaintext, uint8_t key[]);
int gen_salt(uint8_t buf[], size_t bufsize);
char* gen_password(char *buf, size_t bufsize);
void add_round_key(uint8_t state[4][4]);
void key_expansion(uint8_t key[], uint32_t words[]);

/**
 *  NOTES:
 *  1 - Nao faz sentido em algumas funcoes escrever no BUF e retornar o BUF
 *  2 - Tratamento de erro PESSIMO, escolher retornos adequados e dar handle neles em um nivel mais alto (caller)
 *  3 - Map pode ficar lento quando muito grande por conta do jeito que eh feito
*/

int
main()
{
    FILE *fptr = NULL;
    Map *map = NULL;
    int status = 0;

    fptr = login();
    if(fptr == NULL){
        return -1;
    }

    if(map == NULL){
        defer_return(-1);
    }

    load_file(fptr, map);
    print_map(map);

    encrypt_password("password", "seis nove");

    fclose(fptr);
    free(map);
    return 0;

defer:
    free(map);
    if(fptr != NULL) fclose(fptr);
    return status;
}

/*Application Logic Functions */
int
encrypt_password(char *plaintext, char *cypher_text)
{
    int rounds = 10;
    uint8_t key[16];
    uint8_t state[4][4];
    uint32_t words[44];
    int idx;

    gen_key(plaintext, key);
    key_expansion(key, words);
    idx = 0;
    for(int row = 0; row < 4; row++){
        for(int column = 0; column < 4; column++){
            state[row][column] = key[idx];
            idx += 4;
        }
        idx = idx - 12;
    }    

    return 0;
}

void
key_expansion(uint8_t key[], uint32_t words[])
{
    uint32_t round_const[] = {
        0x01, 0x02, 0x04, 0x08, 0x10,
        0x20, 0x40, 0x80, 0x1b, 0x36
    };

    printf("rc: [%x]", round_const[2]);
}

void
add_round_key(uint8_t state[4][4])
{

}

int
gen_key(char *plaintext, uint8_t key[])
{
    uint8_t *text = (uint8_t *)strdup(plaintext);
    int pwdlen = strlen(plaintext);
    uint8_t salt[SALTLEN];

    memset(salt, 0x30, SALTLEN);

    uint32_t time_cost = 2;
    uint32_t mem_cost = (1 << 16);
    uint32_t parallelism = 1;

    argon2id_hash_raw(time_cost, mem_cost, parallelism, text, pwdlen, salt, SALTLEN, key, HASHLEN);

    memset(text, 0, pwdlen);
    free(text);
    return 0;
}

//Refatorar sem duvidas
FILE*
login(void)
{
    char stored_username[MAXLEN];
    char stored_password[MAXLEN];
    char username[MAXLEN];
    char password[MAXLEN];
    FILE *fptr;
    fptr = fopen(FILEPATH, "r");

    if(!fptr){
        fptr = fopen(FILEPATH, "w");

        get_line("Register username: ", username, sizeof(username));
        get_line("Register password: ", password, sizeof(password));
        fprintf(fptr, "%s,%s\n", username, password);
        read_credentials(fptr, stored_username, stored_password);
        fclose(fptr);
    }
    fptr = fopen(FILEPATH, "r");

    get_line("username: ", username, sizeof(username));
    get_line("password: ", password, sizeof(password));
    read_credentials(fptr, stored_username, stored_password);

    if(strcmp(stored_username, username) != 0 || strcmp(stored_password, password) != 0){
        fprintf(stderr, "error: wrong credentials\n");
        return NULL;
    }

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
gen_salt(uint8_t buf[], size_t bufsize)
{
    ssize_t bytes;
    bytes = getrandom(buf, sizeof(char)*bufsize, GRND_NONBLOCK);
    if(bytes == -1) return -1;

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

