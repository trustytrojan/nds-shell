const { createServer } = require('net');
const { createInterface } = require('readline');

const clients = new Set();

createServer((socket) => {
	clients.add(socket);

	socket.on('data', (data) => {
		const message = data.toString().trim();
		console.log(`Received message from ${socket.remoteAddress}: ${message}`);
	});

	socket.on('end', () => {
		clients.delete(socket);
		console.log(`Client disconnected: ${socket.remoteAddress}`);
	});
}).listen(3000, () => {
	console.log('TCP server started on port 3000');

	const rl = createInterface({
		input: process.stdin,
		output: process.stdout
	});

	process.stdout.write('> ');

	rl.on('line', (input) => {
		for (const client of clients)
			client.write(input + '\n');
		process.stdout.write('> ');
	});
});
