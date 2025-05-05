CFLAGS = -Wall -Wextra -pedantic -std=c89 -O0 -fsanitize=address,undefined

all: cursed

cursed: main.o utility.o
	gcc ${CFLAGS} -o ./cursed ./main.o ./utility.o -lncurses

main.o: main.c
	gcc ${CFLAGS} -c -o ./main.o ./main.c

utility.o: utility.c
	gcc ${CFLAGS} -c -o ./utility.o ./utility.c

clean:
	rm -f ./main.o ./utility.o ./cursed

install:
	cp ./cursed /usr/bin/cursed
	chmod 755 /usr/bin/cursed

utility.c: utility.h
main.c: utility.h buffer.h
utility.h: buffer.h
