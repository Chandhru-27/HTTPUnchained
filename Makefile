test:
	printf 'GET / HTTP/1.1\r\nHost: localhost:8000\r\n\r\n' | nc 127.0.0.1 8000

compile:
	gcc socket.c -o ./output/a.out

run:
	./output/a.out