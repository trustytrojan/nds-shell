import express from 'express';

const port = 8080;
const app = express();

app.get('/', (_, res) => res.send('hi\n'));
app.post('/', express.text(), (req, res) => res.send(`you sent: '${req.body}'\n`));

app.listen(port, () => console.log(`listening on port ${port}`));
