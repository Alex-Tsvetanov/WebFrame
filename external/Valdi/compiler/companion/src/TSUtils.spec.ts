import 'ts-jest';
import * as ts from 'typescript';
import { findNode, resolveTypeOfExpression, isExpressionNode, SymbolType, getRootNodesToDump } from './TSUtils';
import { Project } from './project/Project';

interface CompiledFile {
  sourceFile: ts.Node;
  expression: ts.Expression;
  typeChecker: ts.TypeChecker;
}

function compile(str: string): CompiledFile {
  const project = new Project('/', {
    strict: true,
    strictNullChecks: true,
  });
  const sourceFile = project.createSourceFile('File.ts', str);

  const node = findNode(sourceFile, undefined, (node) => isExpressionNode(node));
  if (!node) {
    throw new Error('Cannot resolve expression node');
  }

  return {
    sourceFile: sourceFile,
    expression: node as ts.Expression,
    typeChecker: project.typeChecker,
  };
}

describe('TSUtils', () => {
  it('can find types to dump', () => {
    const result = compile(
      `class Component<VM> {

      }

      // @ExportProxy({ios: 'SCTCallGridParticipant', android: 'com.snap.talk.SCTCallGridParticipant'})
      interface Participant {
        displayName: string;
        color: string;
        hasVideo: boolean;
      }

      /* @ViewModel
      @ExportModel({ios: 'SCTCallGridViewModel', android: 'com.snap.talk.CallGridViewModel'}) */
      interface CallGridViewModel {
        participants: Participant[];
        foo: number;
        bar: string;
      }

      /**
       * @Component
       * @ExportModel({ios: 'SCTCallGrid, android: 'com.snap.talk.CallGrid'})
       */
      export class CallGrid extends Component<CallGridViewModel> {
        // @Action
        someAction() {

        }

        onRender() {
          <layout></layout>;
        }
      }
    `,
    );

    const typesToDump = getRootNodesToDump(result.sourceFile, (nodeToDump) => {
      if (!nodeToDump.leadingComments) {
        return false;
      }

      return nodeToDump.leadingComments.text?.trim().length > 0;
    });
    expect(typesToDump.length).toEqual(3);
  });

  it('can resolve type of number literals', () => {
    const result = compile(`42`);

    const resolvedType = resolveTypeOfExpression(result.expression, result.typeChecker);

    expect(resolvedType.effectiveType).toEqual(SymbolType.number);
    expect(resolvedType.isNullable).toEqual(false);
  });

  it('can resolve type of string literals', () => {
    const result = compile(`'42'`);

    const resolvedType = resolveTypeOfExpression(result.expression, result.typeChecker);

    expect(resolvedType.effectiveType).toEqual(SymbolType.string);
    expect(resolvedType.isNullable).toEqual(false);
  });

  it('can resolve type of compound number expressions', () => {
    const result = compile(`(42 * 2 / 9)`);

    const resolvedType = resolveTypeOfExpression(result.expression, result.typeChecker);

    expect(resolvedType.effectiveType).toEqual(SymbolType.number);
    expect(resolvedType.isNullable).toEqual(false);
  });

  it('can resolve type of optional numbers', () => {
    const result = compile(
      `
    declare function getMeANumber(): number | undefined;
    getMeANumber();
    `,
    );

    const resolvedType = resolveTypeOfExpression(result.expression, result.typeChecker);

    expect(resolvedType.effectiveType).toEqual(SymbolType.number);
    expect(resolvedType.isNullable).toEqual(true);
  });

  it('can resolve type of optional strings', () => {
    const result = compile(
      `
    declare function getMeAString(): string | undefined;
    getMeAString();
    `,
    );

    const resolvedType = resolveTypeOfExpression(result.expression, result.typeChecker);

    expect(resolvedType.effectiveType).toEqual(SymbolType.string);
    expect(resolvedType.isNullable).toEqual(true);
  });

  it('resolves union unrelated types to any', () => {
    const result = compile(
      `
    declare function getMeAStringOrNumber(): string | number | undefined;
    getMeAStringOrNumber();
    `,
    );

    const resolvedType = resolveTypeOfExpression(result.expression, result.typeChecker);

    expect(resolvedType.effectiveType).toEqual(SymbolType.any);
    expect(resolvedType.isNullable).toEqual(true);
  });

  it('resolves object type', () => {
    const result = compile(
      `
    interface MyObject {}
    declare function getMeAnObject(): MyObject;
    getMeAnObject();
    `,
    );

    const resolvedType = resolveTypeOfExpression(result.expression, result.typeChecker);

    expect(resolvedType.effectiveType).toEqual(SymbolType.object);
    expect(resolvedType.isNullable).toEqual(false);

    const objectInfo = resolvedType.resolvedSymbols[0]?.objectInfo!;
    expect(objectInfo).toBeDefined();
    expect(objectInfo.name).toBe('MyObject');

    const fileName = objectInfo.declaration?.getSourceFile()?.fileName!;
    expect(fileName).toBeDefined();

    expect(fileName.endsWith('/File.ts')).toBeTruthy();
  });

  it('resolves function type from lambda', () => {
    const result = compile(
      `
    declare function getMeAFunction(): () => void;
    getMeAFunction();
    `,
    );

    const resolvedType = resolveTypeOfExpression(result.expression, result.typeChecker);

    expect(resolvedType.effectiveType).toEqual(SymbolType.function);
    expect(resolvedType.isNullable).toEqual(false);
  });

  it('resolves function type from function class', () => {
    const result = compile(
      `
    declare function getMeAFunction(): Function;
    getMeAFunction();
    `,
    );

    const resolvedType = resolveTypeOfExpression(result.expression, result.typeChecker);

    expect(resolvedType.effectiveType).toEqual(SymbolType.function);
    expect(resolvedType.isNullable).toEqual(false);
  });

  it('resolves type references', () => {
    const result = compile(
      `
    declare class Hello {}
    declare function getMeAnObject(): Hello;
    getMeAnObject();
    `,
    );

    const resolvedType = resolveTypeOfExpression(result.expression, result.typeChecker);

    expect(resolvedType.effectiveType).toEqual(SymbolType.object);
    expect(resolvedType.isNullable).toEqual(false);
  });
});
