interface StackOverflowCounter {
  counter: number;
}

function testStackOverflow(counter: StackOverflowCounter) {
  counter.counter++;
  testStackOverflow(counter);
}

function testStackOverflowArguments(counter: StackOverflowCounter, p0: number, p1: number, p2: number, p3: number, p4: number, p5: number, p6: number, p7: number, p8: number, p9: number) {
  counter.counter++;
  testStackOverflowArguments(counter, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);
}

const counter: StackOverflowCounter = {
  counter: 0,
}

console.log('Running stack overflow test');
try {
  testStackOverflow(counter);
} catch (exc) {}
console.log(`Stack overflow limit with no arguments: ${counter.counter}`);

counter.counter = 0;
try {
  testStackOverflowArguments(counter, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
} catch (exc) {}
console.log(`Stack overflow limit with arguments: ${counter.counter}`);
