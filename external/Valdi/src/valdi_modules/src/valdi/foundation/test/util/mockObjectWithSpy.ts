export function mockObjectWithSpy<TObject>(object: TObject): TObject {
  if (spyOn) {
    for (const propertyKey in object) {
      const value = object[propertyKey];
      if (typeof value === 'function') {
        spyOn<Record<string, any | jasmine.Func>>(
          object as Record<string, any | jasmine.Func>,
          propertyKey,
        ).and.callThrough();
      }
    }
  }
  return object;
}
