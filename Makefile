PROG=	sish
OBJS=	sish.o opts.o

CFLAGS = -Wall -Werror -Wextra -Wformat=2 -Wjump-misses-init -Wlogical-op -Wundef -Wshadow -Wpointer-arith -Wwrite-strings -Wcast-qual -Wswitch-default -Wswitch-enum -Wunreachable-code -Wpedantic -g

all: ${PROG}
	/usr/sbin/paxctl +a sish

${PROG}: ${OBJS}
	${CC} ${LDFLAGS} ${OBJS} -o ${PROG}

clean:
	rm -f ${PROG} ${OBJS}

format:
	clang-format --style=file -i *.c
