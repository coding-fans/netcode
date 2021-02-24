# Author: fasion
# Created time: 2021-02-23 16:14:31
# Last Modified by: fasion
# Last Modified time: 2021-02-24 16:52:16

server: server.c argparse.c
	gcc -o $@ $^

client: client.c argparse.c
	gcc -o $@ $^

clean:
	rm -f client server
