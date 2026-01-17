test(() => {
  class Parent {
    static test = "What is up";
    test2 = "What is great";
    id: number;

    constructor(id: number) {
      this.id = id;
    }

    describe(): string {
      return "Object ID #" + this.id;
    }

    sayHello(): string {
      return "I am the parent #" + this.id;
    }
  }

  class Child extends Parent {
    constructor(id: number) {
      super(id);
    }
    describe(): string {
      return super.describe();
    }
    sayHello(): string {
      return "I am the child #" + this.id;
    }

    static foo(x: number, y: number, z: number) {}
  }

  const myParent = new Parent(42);
  assertEquals("I am the parent #42", myParent.sayHello());
  assertEquals("Object ID #42", myParent.describe());

  const child = new Child(100);
  assertEquals("I am the child #100", child.sayHello());
  assertEquals("Object ID #100", child.describe());

  // Check that prototype inheritance chain is correct
  assertEquals(Parent.prototype, Object.getPrototypeOf(Child.prototype));
  assertEquals(Parent, Object.getPrototypeOf(Child));

  // Check that constructor back reference from prototype to constructor is set
  assertEquals(Child, Child.prototype.constructor);
  assertEquals(Parent, Parent.prototype.constructor);

  // Test function length (number of args)
  assertEquals(Child.foo.length, 3);

  // Test branching super call
  class Rectangle {
    name: string = "Rectangle";
    constructor(
      private height: number,
      private width: number,
    ) {}
    get area() {
      return this.height * this.width;
    }
  }

  class Square extends Rectangle {
    constructor(length: number) {
      if (length > 10) {
        super(length, length);
      } else {
        super(10, 10);
      }
      assertEquals(this.name, "Rectangle");
      this.name = "Square";
    }
  }

  assertEquals(new Square(5).area, 100);
  assertEquals(new Square(15).area, 225);
  assertEquals(new Square(15).name, "Square");

  // Test parameter property in derived class
  class Square2 extends Rectangle {
    constructor(
      length: number,
      public color: string = "red",
    ) {
      super(length, length);
      this.name = "Square";
    }
  }
  assertEquals(new Square2(15).color, "red");

  // Test new.target
  let newtarget: string | undefined = undefined;
  class A {
    constructor() {
      newtarget = new.target.name;
    }
  }
  class B extends A {
    constructor() {
      super();
    }
  }
  const b = new B();
  assertTrue(newtarget !== undefined);

  // Test interpreted class inheriting compiled class
  const InterpretedChildClass = eval(`
(function (base) {
  return class InterpretedChild extends base {
    constructor() {
      super(15, 15);
      assertEquals(this.name, "Rectangle");
      this.name = "InterpretedChild";
    }
  }
})
`)(Rectangle);
  const interpretedChildObject = new InterpretedChildClass();
  assertEquals(interpretedChildObject.area, 225);
  assertEquals(interpretedChildObject.name, "InterpretedChild");

  // Test compiled class inheriting interpreted class
  const InterpretedRectangleClass = eval(`
(function () {
  return class InterpretedRectangle {
    constructor(height, width) {
      this.name = "InterpretedRectangle";
      this.height = height;
      this.width = width;
    }
    get area() {
      return this.height * this.width;
    }
  }
})
`)();
  class CompiledChild extends InterpretedRectangleClass {
    constructor() {
      super(15, 15);
      assertEquals(this.name, "InterpretedRectangle");
      this.name = "CompiledChild";
    }
  }
  const compiledChildObject = new CompiledChild();
  assertEquals(compiledChildObject.area, 225);
  assertEquals(compiledChildObject.name, "CompiledChild");
}, module);
