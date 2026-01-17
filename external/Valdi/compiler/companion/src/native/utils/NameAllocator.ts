export class NameAllocator {
  private allocatedNames = new Set<string>();

  allocate(nameInitial: string): string {
    let name = nameInitial;

    let index = 0;
    for (;;) {
      if (!this.allocatedNames.has(name)) {
        this.allocatedNames.add(name);
        return name;
      }

      name = `${nameInitial}_${index}`;

      index++;
    }

    return name;
  }
}
