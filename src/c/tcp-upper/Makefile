# Author: fasion
# Created time: 2021-05-13 17:01:08
# Last Modified by: fasion
# Last Modified time: 2021-05-13 17:01:12

server: server.c argparse.c
	gcc -o $@ $^

client: client.c argparse.c
	gcc -o $@ $^

clean:
	rm -f client server
