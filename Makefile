compile:
	gcc -Wall -std=gnu89 server.c network_util.c -o server
	gcc -Wall -std=gnu89 client.c network_util.c -o client

	