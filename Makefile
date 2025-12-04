override CFLAGS=-Wall -Wextra -Wshadow -Wno-unused-parameter -Wno-unused-const-variable -g -O0 -fsanitize=address,undefined

ifdef CI
override CFLAGS=-Wall -Wextra -Wshadow -Werror -Wno-unused-parameter -Wno-unused-const-variable
endif

NAME=sop-collection

.PHONY: clean all tidy

all: ${NAME}

${NAME}: ${NAME}.c
	gcc $(CFLAGS) -o ${NAME} ${NAME}.c

clean:
	rm -f ${NAME}

tidy:
	find . -name "skarbona_*" -delete
