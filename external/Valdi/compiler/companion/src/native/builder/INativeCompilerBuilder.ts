import { NativeCompilerIR } from './NativeCompilerBuilderIR';
import { VariableContext } from './VariableContext';

export const enum NativeCompilerBuilderVariableType {
  Empty = 0,
  Undefined = 1 << 0,
  Null = 1 << 1,
  Number = 1 << 2,
  Bool = 1 << 3,
  Object = 1 << 4,
  ReturnValue = 1 << 7,
  Super = 1 << 8,
  VariableRef = 1 << 9,
  Iterator = 1 << 10,
}

export const enum NativeCompilerBuilderBranchType {
  /**
   * Condition evaluates to true if the value is:
   * - the boolean 'true' value
   * - a non zero number
   * - a non empty string
   * - an object or array
   */
  Truthy,

  /**
   * Condition evaluates to true if the value is anything
   * but undefined or null.
   */
  NotUndefinedOrNull,
}

export class NativeCompilerBuilderVariableBase {
  get type(): NativeCompilerBuilderVariableType {
    return this._type;
  }

  private _type: NativeCompilerBuilderVariableType;

  constructor(type: NativeCompilerBuilderVariableType) {
    this._type = type;
  }

  isRetainable(): boolean {
    return (
      (this._type & NativeCompilerBuilderVariableType.Object) !== 0 ||
      (this._type & NativeCompilerBuilderVariableType.VariableRef) !== 0 ||
      (this._type & NativeCompilerBuilderVariableType.Iterator) !== 0
    );
  }

  getTypeString(): string {
    const type = this._type;
    if (type === NativeCompilerBuilderVariableType.Empty) {
      return 'empty';
    }

    const components: string[] = [];

    if (type & NativeCompilerBuilderVariableType.Undefined) {
      components.push('undefined');
    }
    if (type & NativeCompilerBuilderVariableType.Null) {
      components.push('null');
    }
    if (type & NativeCompilerBuilderVariableType.Number) {
      components.push('number');
    }
    if (type & NativeCompilerBuilderVariableType.Bool) {
      components.push('bool');
    }
    if (type & NativeCompilerBuilderVariableType.Object) {
      components.push('object');
    }
    if (type & NativeCompilerBuilderVariableType.ReturnValue) {
      components.push('retval');
    }
    if (type & NativeCompilerBuilderVariableType.Super) {
      components.push('super');
    }
    if (type & NativeCompilerBuilderVariableType.VariableRef) {
      components.push('vref');
    }
    if (type & NativeCompilerBuilderVariableType.Iterator) {
      components.push('iterator');
    }

    return components.join('_');
  }

  addType(type: NativeCompilerBuilderVariableType): void {
    this._type = NativeCompilerBuilderVariableBase.combineTypes(this._type, type);
  }

  static combineTypes(
    left: NativeCompilerBuilderVariableType,
    right: NativeCompilerBuilderVariableType,
  ): NativeCompilerBuilderVariableType {
    let result = left;

    if (left === NativeCompilerBuilderVariableType.ReturnValue) {
      // Keep return value as is
      return result;
    }

    if (left === NativeCompilerBuilderVariableType.Undefined || left === NativeCompilerBuilderVariableType.Empty) {
      // If the type is undefined, just set the type
      return right;
    } else {
      result |= right;
    }

    if ((result & NativeCompilerBuilderVariableType.Object) != 0) {
      // Decay the type to object once it becomes an object
      result = NativeCompilerBuilderVariableType.Object;
    }

    return result;
  }
}

export class NativeCompilerBuilderVariableID extends NativeCompilerBuilderVariableBase {
  constructor(
    public readonly functionId: number,
    public readonly variable: number,
    type: NativeCompilerBuilderVariableType,
    readonly assignable: boolean,
  ) {
    super(type);
  }

  toString(): string {
    return `${this.getTypeString()}_var${this.variable}`;
  }
}

export enum ConstantType {
  Bool,
  Integer,
  Long,
  Double,
  String,
}
export class ConstValue {
  constructor(
    public type: NativeCompilerBuilderVariableType = NativeCompilerBuilderVariableType.Empty,
    public ctype: ConstantType | undefined = undefined,
    public value: string | undefined = undefined,
  ) {}
}

export class NativeCompilerBuilderVariableRef extends NativeCompilerBuilderVariableBase {
  constructor(
    readonly name: string,
    type: NativeCompilerBuilderVariableType,
    readonly variableId: NativeCompilerBuilderVariableID,
    readonly constValue?: ConstValue,
  ) {
    super(type);
  }
  addType(type: NativeCompilerBuilderVariableType): void {
    super.addType(type);
    if (this.constValue) {
      this.constValue.type = type;
    }
  }
}

export class NativeCompilerBuilderAtomID {
  constructor(public readonly atom: number, readonly identifier: string) {}
}

export class NativeCompilerBuilderJumpTargetID {
  constructor(public readonly label: number, public readonly tag: string) {}
}

export interface INativeCompilerJumpTargetBuilder {
  readonly target: NativeCompilerBuilderJumpTargetID;
  readonly builder: INativeCompilerBlockBuilder;
}

export interface INativeCompilerLoopBuilder {
  readonly initBuilder: INativeCompilerBlockBuilder;
  readonly condBuilder: INativeCompilerBlockBuilder;
  readonly bodyJumpTargetBuilder: INativeCompilerJumpTargetBuilder;
  readonly exitTarget: NativeCompilerBuilderJumpTargetID;
}

export interface INativeCompilerTryCatchBuilder {
  readonly tryBuilder: INativeCompilerBlockBuilder;
  readonly catchBuilder: INativeCompilerBlockBuilder | undefined;
  readonly finallyBuilder: INativeCompilerBlockBuilder | undefined;
}

export interface INativeCompilerFunctionBuilder {
  readonly name: string;
  readonly builder: INativeCompilerBlockBuilder;
  // Will be populated while the function is being built
  readonly closureArguments: NativeCompilerBuilderVariableRef[];
}

export const enum VariableRefScope {
  Block,
  Function,
}

export interface IVariableDelegate {
  onGetValue: (builder: INativeCompilerBlockBuilder) => NativeCompilerBuilderVariableID;
  onSetValue?: (
    builder: INativeCompilerBlockBuilder,
    value: NativeCompilerBuilderVariableID,
  ) => NativeCompilerBuilderVariableID;
}

export interface INativeCompilerBlockBuilder {
  readonly variableContext: VariableContext;

  buildSubBuilder(scoped: boolean): INativeCompilerBlockBuilder;

  buildGlobal(): NativeCompilerBuilderVariableID;
  buildSetProperty(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
    value: NativeCompilerBuilderVariableID,
  ): void;
  buildSetProperties(
    object: NativeCompilerBuilderVariableID,
    properties: NativeCompilerBuilderAtomID[],
    values: NativeCompilerBuilderVariableID[],
  ): void;

  buildSetPropertyValue(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
    value: NativeCompilerBuilderVariableID,
  ): void;

  buildSetPropertyIndex(
    object: NativeCompilerBuilderVariableID,
    index: number,
    value: NativeCompilerBuilderVariableID,
  ): void;

  buildCopyPropertiesFrom(
    destObject: NativeCompilerBuilderVariableID,
    sourceObject: NativeCompilerBuilderVariableID,
    propertiesToIgnore: NativeCompilerBuilderAtomID[] | undefined,
  ): NativeCompilerBuilderVariableID;

  buildGetProperty(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
    thisObject: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID;

  buildGetPropertyValue(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
    thisObject: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID;

  buildDeleteProperty(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
  ): NativeCompilerBuilderVariableID;

  buildDeletePropertyValue(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID;

  buildGetClosureVariableRef(
    variableName: string,
    closureIndex: number,
    source: NativeCompilerBuilderVariableRef,
  ): NativeCompilerBuilderVariableRef;

  buildGetModuleConst(index: number): NativeCompilerBuilderVariableID;

  buildGetFunctionArg(index: number): NativeCompilerBuilderVariableID;
  buildGetFunctionArgumentsObject(startIndex: number): NativeCompilerBuilderVariableID;

  buildGetSuper(): NativeCompilerBuilderVariableID;
  buildGetSuperConstructor(): NativeCompilerBuilderVariableID;

  buildAssignableUndefined(): NativeCompilerBuilderVariableID;

  buildUndefined(): NativeCompilerBuilderVariableID;
  buildNull(): NativeCompilerBuilderVariableID;
  buildThis(): NativeCompilerBuilderVariableID;
  buildNewTarget(): NativeCompilerBuilderVariableID;
  buildCopy(value: NativeCompilerBuilderVariableID): NativeCompilerBuilderVariableID;

  buildException(retVal?: NativeCompilerBuilderVariableID): NativeCompilerBuilderVariableID;
  buildLiteralInteger(value: string): NativeCompilerBuilderVariableID;
  buildLiteralLong(value: string): NativeCompilerBuilderVariableID;
  buildLiteralDouble(value: string): NativeCompilerBuilderVariableID;
  buildLiteralString(value: string): NativeCompilerBuilderVariableID;
  buildLiteralBool(value: boolean): NativeCompilerBuilderVariableID;
  buildGetException(): NativeCompilerBuilderVariableID;
  buildCheckException(jumpTarget: NativeCompilerBuilderJumpTargetID): void;
  buildUnaryOp(
    operator: NativeCompilerIR.UnaryOperator,
    variable: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID;
  buildBinaryOp(
    operator: NativeCompilerIR.BinaryOperator,
    left: NativeCompilerBuilderVariableID,
    right: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID;

  /**
   * Look up an intrinsic constant
   */
  getIntrinsicConstant(name: string): number | string | undefined;
  /**
   * Whether an intrinsic function is available
   */
  hasIntrinsicFunction(func: string): boolean;
  /**
   * Build an intrisic call
   */
  buildIntrinsicCall(func: string, args: Array<NativeCompilerBuilderVariableID>): NativeCompilerBuilderVariableID;

  /**
   * Build a function call that passes the given arguments
   */
  buildFunctionInvocation(
    func: NativeCompilerBuilderVariableID,
    args: Array<NativeCompilerBuilderVariableID>,
    obj: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID;

  /**
   * Build a function call that uses an array as the arguments to pass.
   */
  buildFunctionInvocationWithArgsArray(
    func: NativeCompilerBuilderVariableID,
    args: NativeCompilerBuilderVariableID,
    obj: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID;

  buildConstructorInvocation(
    func: NativeCompilerBuilderVariableID,
    new_target: NativeCompilerBuilderVariableID,
    args: Array<NativeCompilerBuilderVariableID>,
  ): NativeCompilerBuilderVariableID;

  buildConstructorInvocationWithArgsArray(
    func: NativeCompilerBuilderVariableID,
    new_target: NativeCompilerBuilderVariableID,
    args: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID;

  buildNewObject(): NativeCompilerBuilderVariableID;
  buildNewArray(): NativeCompilerBuilderVariableID;
  buildNewArrowFunctionValue(
    functionName: string,
    argc: number,
    closureArguments: NativeCompilerBuilderVariableRef[],
    stackOnHeap: boolean,
  ): NativeCompilerBuilderVariableID;
  buildNewFunctionValue(
    functionName: string,
    name: NativeCompilerBuilderAtomID,
    argc: number,
    closureArguments: NativeCompilerBuilderVariableRef[],
  ): NativeCompilerBuilderVariableID;

  buildNewClassValue(
    functionName: string,
    name: NativeCompilerBuilderAtomID,
    argc: number,
    parentClass: NativeCompilerBuilderVariableID,
    closureArguments: NativeCompilerBuilderVariableRef[],
  ): NativeCompilerBuilderVariableID;

  buildSetClassElement(
    type: NativeCompilerIR.ClassElementType,
    classVariable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderAtomID,
    value: NativeCompilerBuilderVariableID,
    isStatic: boolean,
  ): void;

  buildSetClassElementValue(
    type: NativeCompilerIR.ClassElementType,
    classVariable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderVariableID,
    value: NativeCompilerBuilderVariableID,
    isStatic: boolean,
  ): void;

  //TODO(5173): Find a way to manage variable reassignment, and loops without this function
  buildAssignment(left: NativeCompilerBuilderVariableID, right: NativeCompilerBuilderVariableID): void;
  buildReturn(value: NativeCompilerBuilderVariableID): void;

  buildJumpTarget(tag: string): INativeCompilerJumpTargetBuilder;

  buildJump(target: NativeCompilerBuilderJumpTargetID): void;
  buildBranch(
    conditions: NativeCompilerBuilderVariableID,
    type: NativeCompilerBuilderBranchType,
    trueTarget: NativeCompilerBuilderJumpTargetID,
    falseTarget: NativeCompilerBuilderJumpTargetID | undefined,
  ): void;

  buildLoop(): INativeCompilerLoopBuilder;
  buildTryCatch(hasCatchBlock: boolean, hasFinallyBlock: boolean): INativeCompilerTryCatchBuilder;

  buildIterator(value: NativeCompilerBuilderVariableID): NativeCompilerBuilderVariableID;
  buildKeysIterator(value: NativeCompilerBuilderVariableID): NativeCompilerBuilderVariableID;
  buildIteratorNext(
    value: NativeCompilerBuilderVariableID,
    type: NativeCompilerBuilderVariableType,
  ): NativeCompilerBuilderVariableID;
  buildKeysIteratorNext(
    value: NativeCompilerBuilderVariableID,
    type: NativeCompilerBuilderVariableType,
  ): NativeCompilerBuilderVariableID;

  buildGenerator(closure: NativeCompilerBuilderVariableID): NativeCompilerBuilderVariableID;
  buildResume(): NativeCompilerBuilderVariableID;
  buildSetResumePoint(resumePoint?: NativeCompilerBuilderJumpTargetID): void;
  buildSuspend(value: NativeCompilerBuilderVariableID, resumePoint: NativeCompilerBuilderJumpTargetID): void;

  buildThrow(value: NativeCompilerBuilderVariableID): void;

  buildComments(text: string): void;

  buildProgramCounterInfo(lineNumber: number, columnNumber: number): void;

  registerExitTargetInScope(target: NativeCompilerBuilderJumpTargetID): void;
  resolveExitTargetInScope(): NativeCompilerBuilderJumpTargetID | undefined;

  registerContinueTargetInScope(target: NativeCompilerBuilderJumpTargetID): void;
  resolveContinueTargetInScope(): NativeCompilerBuilderJumpTargetID | undefined;

  registerExceptionTargetInScope(target: NativeCompilerBuilderJumpTargetID): void;
  resolveExceptionTargetInScope(): NativeCompilerBuilderJumpTargetID | undefined;

  buildVariableRef(
    variableName: string,
    type: NativeCompilerBuilderVariableType,
    scope: VariableRefScope,
    isConst?: boolean,
  ): NativeCompilerBuilderVariableRef;
  buildInitializedVariableRef(
    variableName: string,
    value: NativeCompilerBuilderVariableID,
    scope: VariableRefScope,
  ): NativeCompilerBuilderVariableRef;
  buildLoadVariableRef(variableRef: NativeCompilerBuilderVariableRef): NativeCompilerBuilderVariableID;
  buildSetVariableRef(variableRef: NativeCompilerBuilderVariableRef, value: NativeCompilerBuilderVariableID): void;

  resolveVariableRefOrDelegateInScope(
    variableName: string,
  ): NativeCompilerBuilderVariableRef | IVariableDelegate | undefined;
  resolveVariableInScope(variableName: string): NativeCompilerBuilderVariableID | undefined;

  // Register the variable ref that should be used to exports symbols through the 'export' modifiers
  registerExportsVariableRef(variableRef: NativeCompilerBuilderVariableRef): void;
  resolveExportsVariableRef(): NativeCompilerBuilderVariableRef;
  resolveExportsVariable(): NativeCompilerBuilderVariableID;

  /**
   * Register a variable ref that will only be built if it is explicitly accessed through resolveVariableRefInScope().
   */
  registerLazyVariableRef(
    variableName: string,
    scope: VariableRefScope,
    doBuild: (builder: INativeCompilerBlockBuilder) => NativeCompilerBuilderVariableRef,
  ): void;

  /**
   * Builds a variable ref alias that maps to another variable ref.
   * Calls the given delegate whenever the value is requested to be read or set.
   */
  registerVariableDelegate(variableName: string, delegate: IVariableDelegate): void;
}

export interface INativeCompilerBlockBuilderFunctionContext {
  readonly requestedClosureArguments: NativeCompilerBuilderVariableRef[];

  requestClosureArgument(
    variableName: string,
    source: NativeCompilerBuilderVariableRef,
  ): NativeCompilerBuilderVariableRef;

  registerVariable(type: NativeCompilerBuilderVariableType, assignable: boolean): NativeCompilerBuilderVariableID;
}
