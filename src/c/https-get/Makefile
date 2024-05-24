# Author: fasion
# Created time: 2024-05-18 09:41:17
# Last Modified by: fasion
# Last Modified time: 2024-05-24 13:50:56

.PHONY: clean

https-get: https-get.c
	gcc -Wall -o $@ $^ -lssl -lcrypto

run: https-get
	./https-get

clean:
	rm -f https-get
