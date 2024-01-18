const { createServer } = require('net');
const { createInterface } = require('readline');

const clients = new Set();
const port = 3000;

createServer((socket) => {
	console.log(`[${socket.remoteAddress}] connected`);
	clients.add(socket);

	socket.on('data', (data) => {
		const message = data.toString().trim();
		process.stdout.write(`\x1b[0G[${socket.remoteAddress}] ${message}\n> `);
	});

	socket.on('end', () => {
		clients.delete(socket);
		console.log(`[${socket.remoteAddress}] disconnected`);
	});
}).listen(port, () => {
	console.log(`Listening on port ${port}`);

	const rl = createInterface({
		input: process.stdin,
		output: process.stdout
	});

	process.stdout.write('> ');

	rl.on('line', (input) => {
		for (const client of clients)
			client.write(input);
		process.stdout.write('> ');
	});
});
