import 'ts-jest';
import * as ts from 'typescript';
import { IProjectListener, Project } from './Project';

class TestProjectListener implements IProjectListener {
  programChangedEventsCount = 0;
  compilerOptionsResolvedEventsCount = 0;
  fileContentLoadedEvents: string[] = [];
  sourceFileCreatedEvents: string[] = [];
  sourceFileInvalidatedEvents: string[] = [];

  onProgramChanged(): void {
    this.programChangedEventsCount++;
  }

  onProgramWillChange(): void {}

  onCompilerOptionsResolved(): void {
    this.compilerOptionsResolvedEventsCount++;
  }

  onFileContentLoaded(path: string): void {
    this.fileContentLoadedEvents.push(path);
  }

  onSourceFileCreated(path: string): void {
    this.sourceFileCreatedEvents.push(path);
  }

  onSourceFileInvalidated(path: string): void {
    this.sourceFileInvalidatedEvents.push(path);
  }
}

function createDefaultCompilerOptions(): ts.CompilerOptions {
  return {
    target: ts.ScriptTarget.ESNext,
    module: ts.ModuleKind.NodeNext,
    lib: ['lib.es2015.d.ts'],
  };
}

describe('Project', () => {
  it('can resolves tsconfig json from file', () => {
    const listener = new TestProjectListener();
    const project = new Project('/', undefined, listener);

    project.setFileContentAtPath(
      'tsconfig.json',
      `
      {
        "compilerOptions": {
          "target": "es6",
          "module": "commonjs",
          "lib": ["es6"],
          "strict": true,
          "moduleResolution": "node",
        }
      }
    `,
    );

    const compilerOptions = project.compilerOptions;

    expect(listener.compilerOptionsResolvedEventsCount).toBe(1);

    expect(compilerOptions.target).toBe(ts.ScriptTarget.ES2015);
    expect(compilerOptions.module).toBe(ts.ModuleKind.CommonJS);
    expect(compilerOptions.lib).toEqual(['lib.es2015.d.ts']);
    expect(compilerOptions.strict).toBeTruthy();
    expect(compilerOptions.moduleResolution).toBe(ts.ModuleResolutionKind.NodeJs);

    const newCompilerOptions = project.compilerOptions;

    expect(newCompilerOptions).toBe(compilerOptions);
    expect(listener.compilerOptionsResolvedEventsCount).toBe(1);
  });

  it('doesnt resolve tsconfig json from file when provided through ctor', () => {
    const compilerOptions: ts.CompilerOptions = {
      target: ts.ScriptTarget.ESNext,
      module: ts.ModuleKind.NodeNext,
      lib: ['es6'],
    };
    const listener = new TestProjectListener();
    const project = new Project('/', compilerOptions, listener);

    const newCompilerOptions = project.compilerOptions;

    expect(newCompilerOptions).toBe(compilerOptions);
    expect(listener.compilerOptionsResolvedEventsCount).toBe(0);
  });

  it('can create source file', () => {
    const listener = new TestProjectListener();
    const project = new Project('/', createDefaultCompilerOptions(), listener);

    const sourceFile = project.createSourceFile(
      'file.ts',
      `
    export interface MyInterface {
      value: Array<string>
    }
    `,
    );

    expect(sourceFile.statements.length).toBe(1);
    expect(ts.isInterfaceDeclaration(sourceFile.statements[0])).toBeTruthy();

    const interfaceDeclaration = sourceFile.statements[0] as ts.InterfaceDeclaration;
    expect(interfaceDeclaration.name.text).toBe('MyInterface');
    expect(interfaceDeclaration.members.length).toBe(1);

    expect(listener.programChangedEventsCount).toBe(0);
    expect(listener.fileContentLoadedEvents).toEqual(['/file.ts']);
    expect(listener.sourceFileCreatedEvents).toEqual(['/file.ts']);
    expect(listener.sourceFileInvalidatedEvents).toEqual([]);
  });

  it('can open source file', () => {
    const listener = new TestProjectListener();
    const project = new Project('/', createDefaultCompilerOptions(), listener);

    project.setFileContentAtPath('file.ts', `export const MY_NUMBER = 42;`);

    expect(listener.programChangedEventsCount).toBe(0);
    expect(listener.fileContentLoadedEvents).toEqual([]);
    expect(listener.sourceFileCreatedEvents).toEqual([]);
    expect(listener.sourceFileInvalidatedEvents).toEqual([]);

    expect(project.getOpenedSourceFile('file.ts')).toBeUndefined();
    const sourceFile = project.openSourceFile('file.ts');

    expect(sourceFile.statements.length).toBe(1);
    expect(sourceFile.statements[0].kind).toBe(ts.SyntaxKind.FirstStatement);

    expect(project.getOpenedSourceFile('file.ts')).toBe(sourceFile);

    expect(listener.programChangedEventsCount).toBe(0);
    expect(listener.fileContentLoadedEvents).toEqual(['/file.ts']);
    expect(listener.sourceFileCreatedEvents).toEqual(['/file.ts']);
    expect(listener.sourceFileInvalidatedEvents).toEqual([]);
  });

  it('returns same source file when no change is found', () => {
    const listener = new TestProjectListener();
    const project = new Project('/', createDefaultCompilerOptions(), listener);

    project.setFileContentAtPath('file.ts', `export const MY_NUMBER = 42;`);

    const sourceFile = project.openSourceFile('file.ts');
    const newSourceFile = project.openSourceFile('file.ts');

    expect(sourceFile).toBe(newSourceFile);
    expect(listener.programChangedEventsCount).toBe(0);
    expect(listener.fileContentLoadedEvents).toEqual(['/file.ts']);
    expect(listener.sourceFileCreatedEvents).toEqual(['/file.ts']);
    expect(listener.sourceFileInvalidatedEvents).toEqual([]);
  });

  it('returns different source file on change', () => {
    const listener = new TestProjectListener();
    const project = new Project('/', createDefaultCompilerOptions(), listener);

    project.setFileContentAtPath('file.ts', `export const MY_NUMBER = 42;`);

    const sourceFile = project.openSourceFile('file.ts');
    expect(sourceFile.statements.length).toBe(1);
    expect(sourceFile.statements[0].kind).toBe(ts.SyntaxKind.FirstStatement);

    expect(listener.programChangedEventsCount).toBe(0);
    expect(listener.fileContentLoadedEvents).toEqual(['/file.ts']);
    expect(listener.sourceFileCreatedEvents).toEqual(['/file.ts']);
    expect(listener.sourceFileInvalidatedEvents).toEqual([]);

    project.setFileContentAtPath(
      'file.ts',
      `
    export interface MyInterface {
      value: Array<string>
    }    `,
    );

    expect(listener.programChangedEventsCount).toBe(0);
    expect(listener.fileContentLoadedEvents).toEqual(['/file.ts']);
    expect(listener.sourceFileCreatedEvents).toEqual(['/file.ts']);
    expect(listener.sourceFileInvalidatedEvents).toEqual(['/file.ts']);

    const newSourceFile = project.openSourceFile('file.ts');

    expect(newSourceFile).not.toBe(sourceFile);
    expect(newSourceFile.statements.length).toBe(1);
    expect(newSourceFile.statements[0].kind).toBe(ts.SyntaxKind.InterfaceDeclaration);

    expect(listener.programChangedEventsCount).toBe(0);
    expect(listener.fileContentLoadedEvents).toEqual(['/file.ts', '/file.ts']);
    expect(listener.sourceFileCreatedEvents).toEqual(['/file.ts', '/file.ts']);
    expect(listener.sourceFileInvalidatedEvents).toEqual(['/file.ts']);
  });

  it('loads tslib when program loads', () => {
    const listener = new TestProjectListener();
    const project = new Project('/', createDefaultCompilerOptions(), listener);

    project.setFileContentAtPath('file.ts', `export const MY_NUMBER = 42;`);

    project.openSourceFile('file.ts');
    project.loadProgramIfNeeded();

    expect(listener.programChangedEventsCount).toBe(1);
    expect(listener.fileContentLoadedEvents).toEqual([
      '/file.ts',
      '/__tslibs__/lib.es2015.d.ts',
      '/__tslibs__/lib.es5.d.ts',
      '/__tslibs__/lib.decorators.d.ts',
      '/__tslibs__/lib.decorators.legacy.d.ts',
      '/__tslibs__/lib.es2015.core.d.ts',
      '/__tslibs__/lib.es2015.collection.d.ts',
      '/__tslibs__/lib.es2015.iterable.d.ts',
      '/__tslibs__/lib.es2015.symbol.d.ts',
      '/__tslibs__/lib.es2015.generator.d.ts',
      '/__tslibs__/lib.es2015.promise.d.ts',
      '/__tslibs__/lib.es2015.proxy.d.ts',
      '/__tslibs__/lib.es2015.reflect.d.ts',
      '/__tslibs__/lib.es2015.symbol.wellknown.d.ts',
    ]);
    expect(listener.sourceFileCreatedEvents).toEqual([
      '/file.ts',
      '/__tslibs__/lib.es2015.d.ts',
      '/__tslibs__/lib.es5.d.ts',
      '/__tslibs__/lib.decorators.d.ts',
      '/__tslibs__/lib.decorators.legacy.d.ts',
      '/__tslibs__/lib.es2015.core.d.ts',
      '/__tslibs__/lib.es2015.collection.d.ts',
      '/__tslibs__/lib.es2015.iterable.d.ts',
      '/__tslibs__/lib.es2015.symbol.d.ts',
      '/__tslibs__/lib.es2015.generator.d.ts',
      '/__tslibs__/lib.es2015.promise.d.ts',
      '/__tslibs__/lib.es2015.proxy.d.ts',
      '/__tslibs__/lib.es2015.reflect.d.ts',
      '/__tslibs__/lib.es2015.symbol.wellknown.d.ts',
    ]);
    expect(listener.sourceFileInvalidatedEvents).toEqual([]);
  });

  it('can open source file with dependent file', () => {
    const listener = new TestProjectListener();
    const project = new Project('/', createDefaultCompilerOptions(), listener);

    project.setFileContentAtPath(
      'utils/format.ts',
      `
    export function format(input: string): string {
      return input.toUpperCase();
    }
    `,
    );

    project.setFileContentAtPath(
      'file.ts',
      `
      import { format } from './utils/format';

      export function showSomething(): string {
        return format('This is sparta');
      }
    `,
    );

    expect(project.getOpenedSourceFile('utils/format.ts')).toBeUndefined();

    project.openSourceFile('file.ts');
    project.loadProgramIfNeeded();

    expect(project.getOpenedSourceFile('utils/format.ts')).not.toBeUndefined();

    expect(listener.programChangedEventsCount).toBe(1);
    expect(listener.fileContentLoadedEvents).toEqual([
      '/file.ts',
      '/utils/format.ts',
      '/__tslibs__/lib.es2015.d.ts',
      '/__tslibs__/lib.es5.d.ts',
      '/__tslibs__/lib.decorators.d.ts',
      '/__tslibs__/lib.decorators.legacy.d.ts',
      '/__tslibs__/lib.es2015.core.d.ts',
      '/__tslibs__/lib.es2015.collection.d.ts',
      '/__tslibs__/lib.es2015.iterable.d.ts',
      '/__tslibs__/lib.es2015.symbol.d.ts',
      '/__tslibs__/lib.es2015.generator.d.ts',
      '/__tslibs__/lib.es2015.promise.d.ts',
      '/__tslibs__/lib.es2015.proxy.d.ts',
      '/__tslibs__/lib.es2015.reflect.d.ts',
      '/__tslibs__/lib.es2015.symbol.wellknown.d.ts',
    ]);
    expect(listener.sourceFileCreatedEvents).toEqual([
      '/file.ts',
      '/utils/format.ts',
      '/__tslibs__/lib.es2015.d.ts',
      '/__tslibs__/lib.es5.d.ts',
      '/__tslibs__/lib.decorators.d.ts',
      '/__tslibs__/lib.decorators.legacy.d.ts',
      '/__tslibs__/lib.es2015.core.d.ts',
      '/__tslibs__/lib.es2015.collection.d.ts',
      '/__tslibs__/lib.es2015.iterable.d.ts',
      '/__tslibs__/lib.es2015.symbol.d.ts',
      '/__tslibs__/lib.es2015.generator.d.ts',
      '/__tslibs__/lib.es2015.promise.d.ts',
      '/__tslibs__/lib.es2015.proxy.d.ts',
      '/__tslibs__/lib.es2015.reflect.d.ts',
      '/__tslibs__/lib.es2015.symbol.wellknown.d.ts',
    ]);
    expect(listener.sourceFileInvalidatedEvents).toEqual([]);
  });

  it('can reload source file', () => {
    const listener = new TestProjectListener();
    const project = new Project('/', createDefaultCompilerOptions(), listener);

    project.setFileContentAtPath(
      'utils/format.ts',
      `
    export function format(input: string): string {
      return input.toUpperCase();
    }
    `,
    );

    project.setFileContentAtPath(
      'file.ts',
      `
      import { format } from './utils/format';

      export function showSomething(): string {
        return format('This is sparta');
      }
    `,
    );

    const fileDotTs = project.openSourceFile('file.ts');
    project.loadProgramIfNeeded();
    const formatDotTs = project.getOpenedSourceFile('utils/format.ts');

    expect(formatDotTs).not.toBeUndefined();

    listener.fileContentLoadedEvents = [];
    listener.sourceFileCreatedEvents = [];
    listener.sourceFileInvalidatedEvents = [];

    expect(listener.programChangedEventsCount).toBe(1);

    project.setFileContentAtPath(
      'utils/format.ts',
      `
    export function format(input: string): string {
      return input.toLowerCase();
    }
    `,
    );

    project.loadProgramIfNeeded();

    const newFileDotTs = project.getOpenedSourceFile('file.ts');
    const newFormatDotTs = project.getOpenedSourceFile('utils/format.ts');

    expect(newFileDotTs).toBe(fileDotTs);
    expect(newFormatDotTs).not.toBe(formatDotTs);
    expect(newFormatDotTs).not.toBeUndefined();

    expect(listener.programChangedEventsCount).toBe(2);
    expect(listener.fileContentLoadedEvents).toEqual(['/utils/format.ts']);
    expect(listener.sourceFileCreatedEvents).toEqual(['/utils/format.ts']);
    expect(listener.sourceFileInvalidatedEvents).toEqual(['/utils/format.ts']);

    project.loadProgramIfNeeded();
    project.loadProgramIfNeeded();

    // Should not change if no changes were detected
    expect(listener.programChangedEventsCount).toBe(2);
  });
});
