import 'jasmine';
import { toPascalCase, toSnakeCase } from './stringUtils';

describe('stringUtils', () => {
    it('converts strings to pascal case', () => {
      const testCases: [string, string][] = [['hello', 'Hello'], ['hello world', 'HelloWorld'], ['My New Module', 'MyNewModule'], ['my_new_module', 'MyNewModule']];
      testCases.forEach(([input, expected]) => {
        expect(toPascalCase(input)).toBe(expected);
      });
    });

    it('converts strings to snake case', () => {
      const testCases: [string, string][] = [['hello', 'hello'], ['hello world', 'hello_world']];
      testCases.forEach(([input, expected]) => {
        expect(toSnakeCase(input)).toBe(expected);
      });
    });
});

