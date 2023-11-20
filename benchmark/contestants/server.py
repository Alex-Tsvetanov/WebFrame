from flask import Flask, g
import random
app = Flask(__name__)
maxInt = 2 ** 31

@app.route("/<steps>/1")
def hello(steps):
    x = 0
    i = 0
    steps = 2 ** int(steps)
    while i < steps:
        x += 1
        x %= maxInt
        i += 1
    return "Hello World!"

@app.route("/<steps>/2")
def hello2(steps):
    x = 0
    i = 0
    steps = 2 ** int(steps)
    while i < steps:
        x += random.randint(-2 ** 30, 2 ** 30)
        x %= maxInt
        i += 1
    return "Hello World!"

if __name__ == "__main__":
    app.run(port=8888)