declare function print(arg: unknown): void;

interface Module {
  path: string;
  exports: any;
}
declare const module: Module;

declare function assertEquals(left: any, right: any): void;
declare function assertNotEquals(left: any, right: any): void;
declare function assertTrue(value: boolean): void;
declare function assertFalse(value: boolean): void;

type TestFuncReturn = void | PromiseLike<void>;
declare function test(cb: () => TestFuncReturn, m?: Module): void;

declare let lastTestComplete: Promise<void>;
declare const failedTests: string[];

declare function setExitCode(code: number): void;
