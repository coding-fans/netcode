/*
 * Author: fasion
 * Created time: 2022-08-31 11:27:14
 * Last Modified by: fasion
 * Last Modified time: 2022-08-31 17:00:13
 */

const net = require('net');

const PORT = 8080;

// 新建TCP服务器
const server = new net.Server();

// 新连接事件回调，有请求连上来就会执行
// 参数client是代表客户端请求的套接字连接，通过它可以
// - 读，即从客户端接收数据
// - 写，即向客户端发送数据
server.on("connection", function (client) {
	// 用来解析协议的缓冲区
	let parseBuffer = Buffer.alloc(0);
	// 暂存需要发往服务器的数据
	let chunksToServer = null;
	// 暂存需要发往客户端的数据
	let chunksToClient = null;
	// 代表服务端的套接字，跟client一样，通过它可以跟服务端交互
	let server = null;

	// 关闭客户端连接
	const closeClient = function () {
		if (client) {
			client.end();
			client = null;
		}
	}

	// 关闭服务端连接
	const closeServer = function () {
		if (server) {
			server.end();
			server = null;
		}
	}

	// 同时关闭两端连接
	const close = function () {
		closeClient();
		closeServer();
	}

	// 客户端套接字注册事件处理函数
	// - 连接关闭或出错直接执行close，释放连接
	// - 收到客户端发来的数据要处理
	client.on("end", close);
	client.on("error", close);
	client.on("data", function (chunk) {
		// 收到数据，准备解析请求行
		// - GET http://fasionchan.com HTTP/1.1 (代理HTTP请求，例如：http://fasionchan.com)
		// - CONNECT fasionchan.com:443 HTTP/1.1 (代理HTTPS请求，例如：https://fasionchan.com)
		// 由于数据可能不是一次性到达的，我们将它放在buffer里面，
		// 解析完毕就将buffer删掉，因此buffer还在说明我们还在解析请求行
		if (parseBuffer) {
			// 将当前收到的数据片跟buffer中保存的数据合起来
			parseBuffer = Buffer.concat([parseBuffer, chunk]);

			// 请求行以\r\n结束，如果还没有\r\n，说明数据还不完整，直接退出，等下次data事件继续处理
			const index = parseBuffer.indexOf("\r\n");
			if (index === -1) {
				return;
			}

			// 从buffer中取出请求行，\r\n后面的数据也保存为rest
			const line = parseBuffer.slice(0, index).toString();
			const rest = parseBuffer.slice(index+2);

			// 解析切分请求行，分成：方法 URL地址 版本三部分
			// 如果格式有误，直接关闭客户端连接
			const parts = line.split(" ").filter(part => part);
			if (parts.length !== 3) {
				closeClient();
				return;
			}

			// 拿到方法、地址、版本三要素
			const [method, address, version] = parts;

			// 保存客户端请求我们代为连接的服务器地址和端口
			let host, port;

			// 如果收到CONNECT请求，要求建立隧道
			// CONNECT fasionchan.com:443 HTTP/1.1
			if (method.toLowerCase() === "connect") {
				console.log("CONNECT", address);

				// 确保消耗尽整个HTTP请求的数据，标志是以\r\n\r\n结尾
				if (parseBuffer.indexOf("\r\n\r\n") === -1) {
					return;
				}

				// 从地址部分切出地址端口对
				// 格式有误同样直接关闭客户端连接
				const pair = address.split(":");
				if (pair.length !== 2) {
					closeClient();
					return;
				}

				// 记录客户端请求连接的服务器地址和端口
				[host, port] = pair;

				// 隧道建立完毕要给客户端200响应
				// 暂存给客户端的响应，等我们连上服务器后再发给它
				chunksToClient = [`${version} 200 OK\r\n\r\n`];
			} else {
				// 其他情况是普通HTTP请求
				// GET http://fasionchan.com HTTP/1.1
				console.log("PROXY", address);

				// 地址部分是客户端要请求的URL资源，先解析它
				const url = new URL(address);

				// 记录请求该URL资源需要连接的服务器地址和端口号
				host = url.hostname;
				port = url.port || 80;

				chunksToServer = [
					// 连上服务器后，我们需要重新构造一个请求行发给服务器
					`${method} ${url.pathname}${url.search || ""} ${version}\r\n`,
					// 解析原请求行的剩下的数据，一般是请求的头部和请求体
					// 这部分需原封不动发给服务器
					rest,
				];
			}

			// 将解析缓冲区删掉，表示解析已完成，后续数据直接转发
			parseBuffer = null;

			// 开始建立到服务器的连接，成功连上会执行回调函数
			const socket = net.createConnection({host, port}, () => {
				// 成功连上服务器了，将服务端套接字记录下来
				server = socket;

				// 开始发送暂存要发往服务器的数据
				chunksToServer?.forEach((chunk) => {
					server.write(chunk);
				});
				chunksToServer = null;

				// 开始发送暂存要发往客户端的数据
				chunksToClient?.forEach((chunk) => {
					client?.write(chunk);
				})

				// 注册data事件处理函数，从服务端收到数据，直接转给客户端
				server.on("data", (chunk) => {
					client?.write(chunk);
				});
			});

			// 为连接服务端套接字，注册关闭和出错的处理函数，直接调用close清理
			socket.on("end", close);
			socket.on("error", close);

			return;
		}

		// 解析缓冲区已经删掉，说明解析工作已完成，接下来只需做数据转发
		// 如果到服务器的连接已经建好，就直接将数据发给服务器
		// 否则，先暂存到chunksToServer
		if (server) {
			server.write(chunk);
		} else {
			chunksToServer?.push(chunk);
		}
	});
});

// 开始监听8080，等待客户端请求
server.listen(PORT, function () {
	console.log(`server listening at port ${PORT}`);
});
