import { createServer } from 'net';

const port = 3000;

/** @type {Set<import('net').Socket>} */
const clients = new Set;

createServer(socket => {
	clients.add(socket);
	const address = `${socket.remoteAddress}:${socket.remotePort}`;
	console.log(`[${address}] connected`);
	socket.on('data', data => console.log(`[${address}] ${data.toString()}`));
	socket.on('close', hadError => console.log(`[${address}] disconnected${hadError ? ' (error occurred)' : ''}`));
	socket.on('error', error => console.error(`error occurred with [${address}]!\n${error}`));
}).listen(port, () => {
	console.log(`listening on port ${port}`)
});
