const express = require('express')
const app = express()
const port = 8888

app.get('/:steps/1', (req, res) => {
    let x = 0;
    for (let i = 0; i < (1 << parseInt(req.params['steps'])); i += 1) {
        x++;
    }
    res.send('Hello, World!');
});

app.get('/:steps/2', (req, res) => {
    let x = 0;
    for (let i = 0; i < (1 << parseInt(req.params['steps'])); i += 1) {
        x += parseInt(Math.random() * 1e9);
    }
    res.send('Hello, World!');
});

app.listen(port, () => {
    console.log(`Example app listening on port ${port}!`);
});