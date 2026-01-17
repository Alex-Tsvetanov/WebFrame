const counterClass = eval(`
(class Counter {
  count(...args) {
    return args.length;
  }
})
`);

const counter = new counterClass();

test(() => {
  const value = counter.count("This is a string", 42);
}, module);
