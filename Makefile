SRC = src/password_manager.c
CC = gcc
OBJ = password_manager.o
CFLAGS = -Wall -Wextra
LDFLAGS = -lncurses -Linclude -largon2

all:
	${CC} ${SRC} -o ${OBJ} ${CFLAGS} ${LDFLAGS}

clean:
	rm -f password_manager.o

run:
	./password_manager.o
