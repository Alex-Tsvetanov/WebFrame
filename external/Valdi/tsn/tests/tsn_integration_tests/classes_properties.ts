test(() => {
  class Person {
    firstName: string;
    lastName: string;

    get name(): string {
      return this.firstName + " " + this.lastName;
    }

    set name(name: string) {
      const components = name.split(" ");
      this.firstName = components[0];
      this.lastName = components[1];
    }

    private static _allPersons: Person[] = [];

    static get allPersons(): Person[] {
      return Person._allPersons;
    }

    static set allPersons(v: Person[]) {
      Person._allPersons = v;
    }

    constructor(firstName: string, lastName: string) {
      this.firstName = firstName;
      this.lastName = lastName;

      Person._allPersons.push(this);
    }
  }

  const person = new Person("Jean", "Jacques");

  assertEquals("Jean", person.firstName);
  assertEquals("Jacques", person.lastName);
  assertEquals("Jean Jacques", person.name);

  person.name = "Micheline Legros";
  assertEquals("Micheline", person.firstName);
  assertEquals("Legros", person.lastName);
  assertEquals("Micheline Legros", person.name);

  let allPersons = Person.allPersons;

  assertEquals(1, allPersons.length);
  assertEquals(person, allPersons[0]);

  Person.allPersons = [];

  allPersons = Person.allPersons;

  assertEquals(0, allPersons.length);

  const person2 = new Person("Luke", "Skywalker");

  allPersons = Person.allPersons;
  assertEquals(1, allPersons.length);
  assertEquals(person2, allPersons[0]);
}, module);
