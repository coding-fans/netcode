#!/usr/bin/env python
# -*- encoding=utf8 -*-

'''
Author: fasion
Created time: 2022-09-11 18:22:55
Last Modified by: fasion
Last Modified time: 2022-09-12 11:55:33
'''

import re
from socket import (
	socket,
	gethostbyname,
	AF_INET,
	SOCK_STREAM,
)

def http_connect(server, port, proxy_server='127.0.0.1', proxy_port=80):
	'''创建套接字，通过指定代理服务器，建立到目标服务器的TCP连接'''
	# 根据IP和端口，建立到代理服务器的TCP连接
	s = socket(AF_INET, SOCK_STREAM)
	s.connect((proxy_server, int(proxy_port)))

	# 向代理服务器发送CONNECT请求，包含请求行和一个空行
	s.send('CONNECT {}:{} HTTP/1.0\r\n\r\n'.format(server, port).encode('utf8'))

	# 接收代理服务器发来的响应，假设响应是一次性送达的
	response = s.recv(10240).decode('utf8')
	code = response.split(' ')[1].strip()
	# 如果响应码不是200，说明失败了
	if code != '200':
		return

	# 返回套接字，后续通过该套接字即可跟目标服务通信
	return s

def main():
	# 通过代理服务器，连接10.0.0.2上的tcp-upper-server，端口为9999
	s = http_connect('10.0.0.2', 9999, proxy_server='127.0.0.1', proxy_port=13128)
	if not s:
		return

	# 向tcp-upper-server发送数据
	s.send(b'abc')

	# 接收到大写转换结果
	print(s.recv(10240).decode('utf8'))

if __name__ == '__main__':
	main()
