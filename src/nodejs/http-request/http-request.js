/*
 * Author: fasion
 * Created time: 2022-09-11 12:15:35
 * Last Modified by: fasion
 * Last Modified time: 2022-09-11 16:56:28
 */

const dns = require("dns");
const net = require("net");

function parseHttpResponseInBuffer(buffer) {
	// 记录已解析字节数
	let parsed = 0;

	// 解析状态行
	// 先找到第一个换行标志
	const i = buffer.indexOf("\r\n", parsed);
	if (i === -1) {
		return null;
	}

	// 从缓冲区中取出第一行内容，按空格切分并取出状态码
	const code = buffer.slice(parsed, i).toString().split(" ")[1];
	parsed = i + 2;

	// 逐行遍历解析响应头部
	const headers = new Map();
	while (true) {
		// 找出头部行换行标志
		const i = buffer.indexOf("\r\n", parsed);
		if (i === -1) {
			return null;
		}

		// 取出下一行
		const line = buffer.slice(parsed, i).toString();
		parsed = i + 2;

		// 遇到空行，表明头部部分已经解析完毕，跳出循环
		if (!line) {
			break;
		}

		// 找到第一个冒号，据此切出头部名称和值
		const splitIndex = line.indexOf(":");
		if (splitIndex === -1) {
			return null;
		}

		headers[line.slice(0, splitIndex).trim()] = line.slice(splitIndex+1).trim();
	}

	return {
		code,
		headers,
		body: buffer.slice(parsed),
	}
}

async function httpRequest(url, method="GET", headers=null, body=null) {
	const urlInfo = new URL(url);

	// 取出主机中的地址
	let addr = urlInfo.hostname;
	console.log(addr)
	if (!/[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/.test(addr)) {
		addr = (await dns.promises.lookup(addr)).address;
	}

	console.log(urlInfo)
	console.log(addr, urlInfo.port || 80)

	// 创建TCP套接字，并连接到服务器IP和端口上
	// 通过await Promise确保等待连接建立完毕，即等待三次握手完成
	const s = new net.Socket();
	await new Promise((resolve) => {
		s.connect(urlInfo.port || 80, addr, () => {
			resolve();
		});
	});

	// 发送HTTP请求报文
	// 请求行
	s.write(`${method} ${urlInfo.pathname}${urlInfo.search} HTTP/1.0\r\n`);
	// Host头部
	s.write(`Host: ${urlInfo.host}\r\n`);
	// Connection头部，指定不保持长连接
	s.write(`Connection: close\r\n`);

	// 额外的自定义头部（如有）
	if (headers) {
		for (const name of headers) {
			s.write(`${name}: ${headers[name]}\r\n`);
		}
	}

	// 空行
	s.write(`\r\n`);

	// 请求体，即数据部分（如有）
	if (body) {
		s.write(body);
	}

	// 收集服务器响应，因为服务器响应可能不是一次性返回的
	const chunks = [];
	s.on("data", (chunk) => {
		chunks.push(chunk);
	});

	// 等服务器关闭连接后，我们再解析HTTP响应
	// 最佳做法应该是边接收边解析
	// 这里为了简化，我们先将服务器响应收集起来后再处理
	await new Promise((resolve, reject) => {
		s.on("end", () => {
			resolve();
		});
	});

	// 将服务器响应拼接起来得到完整的响应报文
	// 再按HTTP响应报文格式来解析
	return parseHttpResponseInBuffer(new Buffer.concat(chunks));
}

async function demo() {
	const url = "http://cors.fasionchan.com/about.txt";
	const response = await httpRequest(url);
	console.log(response.code);
	console.log(response.headers);
	console.log(response.body.toString())
}

demo();
