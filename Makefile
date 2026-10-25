SRCS := $(shell find src -name '*.c')
HTTP_SRCS := $(shell find src/http -name '*.c')

setup:
	mkdir output

compile:
	gcc -Iinclude $(SRCS) -o ./output/a.out

scompile:
	gcc -std=c11 -Wall -Wextra -Wpedantic \
    -Iinclude $(SRCS) \
    -o output/a.out

run:
	./output/a.out

test:
	printf 'GET / HTTP/1.1\r\nHost: localhost:8000\r\n\r\n' | nc 127.0.0.1 8000

unit-test:
	gcc -std=c11 -Wall -Wextra -Wpedantic \
    -Iinclude tests/test_http.c $(HTTP_SRCS) \
    -o output/test_http
	./output/test_http

kill-port:
	fuser -k 8000/tcp