/*
 * Author: fasion
 * Created time: 2022-09-12 11:30:02
 * Last Modified by: fasion
 * Last Modified time: 2022-09-12 12:08:07
 */

const net = require("net");

/**
 * 使用指定套接字对象，建立到指定服务器和端口的TCP连接
 * @param {*} s 套接字
 * @param {*} server 服务器地址
 * @param {*} port 服务器端口
 */
async function socket_conn(s, server, port) {
	await new Promise((resolve, reject) => {
		s.connect(port, server, resolve);
		s.on("error", reject);
	});
}

/**
 * 从指定套接字接收数据
 * @param {*} s 套接字
 * @returns 接收到的数据，以Buffer的形式存在
 */
async function socket_recv(s) {
	return await new Promise((resolve, reject) => {
		s.on("data", resolve);
		s.on("error", reject);
	});
}

/**
 * 根据指定服务器地址和端口，建立TCP连接并返回套接字
 * @param {*} server 服务器地址
 * @param {*} port 服务器端口
 * @returns 套接字
 */
async function tcp_connect(server, port) {
	// 创建TCP套接字，并连接到服务器IP和端口上
	// 通过await Promise确保等待连接建立完毕，即等待三次握手完成
	const s = new net.Socket();
	await socket_conn(s, server, port);

	return s;
}

/**
 * 发送CONNECT方法请求，通过代理服务器建立到目标服务器的TCP隧道，返回套接字
 * @param {*} server 目标服务器地址
 * @param {*} port 目标服务器端口
 * @param {*} proxy_server 代理服务器地址
 * @param {*} proxy_port 代理服务器端口
 * @returns 套接字
 */
async function http_connect(server, port, proxy_server="127.0.0.1", proxy_port=3128) {
	// 先连接到代理服务器
	const s = await tcp_connect(proxy_server, proxy_port);

	// 发送CONNECT请求，包含请求行和一个空行
	s.write(`CONNECT ${server}:${port} HTTP/1.0\r\n\r\n`);

	// 接收代理服务器发来的响应（假设响应一次性到达）
	const response = (await socket_recv(s)).toString();
	// 解析响应码，200表示成功
	const code = response.split(" ")[1].trim();
	if (code !== "200") {
		return null;
	}

	// 返回套接字，后续通过该套接字即可跟目标服务通信
	return s
}

async function demo() {
	// 通过代理服务器，连接10.0.0.2上的tcp-upper-server，端口为9999
	const s = await http_connect("10.0.0.2", 9999, "127.0.0.1", 13128);
	if (!s) {
		return;
	}

	// 向tcp-upper-server发送数据
	s.write("abc");

	// 接收到大写转换结果
	const result = (await socket_recv(s)).toString();
	console.log(result);

	// 关闭连接
	s.end();
}

demo();
