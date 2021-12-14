all:
	gcc -g io_server.c -o io_server
	gcc -g io_client.c -o io_client
clean:
	rm io_server
	rm io_client
