#!/usr/bin/env python
# -*- encoding=utf8 -*-

'''
Author: fasion
Created time: 2022-09-11 10:11:02
Last Modified by: fasion
Last Modified time: 2022-09-12 11:15:59
'''

import re
from socket import (
	socket,
	gethostbyname,
	AF_INET,
	SOCK_STREAM,
)
from urllib.parse import urlparse

# IP判断正则表达式
IP_PATTERN = re.compile(r'^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$')

def parse_http_response(response):
	# 记录已解析字节数
	parsed = 0

	# 解析第一行，找到\r\n换行标志
	i = response.find(b'\r\n', parsed)
	if i == -1:
		return None, None, None

	# 第一行为状态行，切成三部分，并取出状态码
	parts = response[parsed:i].split(b' ')
	code = int(parts[1])
	parsed = i + 2

	# 解析HTTP头部，每个一行，直到遇到空行
	headers = {}
	while True:
		i = response.find(b'\r\n', parsed)
		if i == -1:
			return None, None, None

		header_line = response[parsed:i]
		parsed = i + 2

		# 遇到空行，说明头部部分已结束
		if not header_line:
			break

		# 切分出头部名和值，并保存到字典
		name, value = header_line.decode('utf8').split(':', 1)
		headers[name.strip()] = value.strip()

	# 其余部分为响应体，即数据
	body = response[parsed:]

	return code, headers, body

def http_request(url, method='GET', headers=None, data=None):
	# 解析URL
	url_info = urlparse(url)

	# 端口默认是80
	port = 80
	# 取出主机地址端口
	netloc = url_info.netloc
	if ':' in netloc:
		host, port = netloc.split(':')
	else:
		host = netloc

	if IP_PATTERN.match(host):
		# 如果主机是IP地址，直接使用
		ip = host
	else:
		# 如果主机是域名，先解析成IP地址
		ip = gethostbyname(host)

	# 根据IP和端口，建立到服务器的TCP连接
	s = socket(AF_INET, SOCK_STREAM)
	s.connect((ip, int(port)))

	# 发送请求行
	s.send('{} {} HTTP/1.0\r\n'.format(method, url_info.path).encode('utf8'))

	# 设置Host头部（由URL主机地址部分决定）
	s.send('Host: {}\r\n'.format(netloc).encode('utf8'))

	# 如果有指定额外头部，逐个发送，每个一行
	if headers:
		for name, value in headers.items():
			s.send('{}: {}\r\n'.format(name, value).encode('utf8'))

	# 发送一个空行
	s.send(b'\r\n')

	# 发送数据（请求体）
	if data:
		s.send(data)

	# 解析HTTP响应
	# 通常需要考虑数据分多次送达的情况，边接收数据边解析
	# 这里为加以简化，假设响应一次性送达
	return parse_http_response(s.recv(10240))

def main():
	url = 'http://cors.fasionchan.com:80/about.txt'

	code, headers, body = http_request(url)
	print(code)
	print(headers)
	print(body.decode('utf8'))

if __name__ == '__main__':
	main()
