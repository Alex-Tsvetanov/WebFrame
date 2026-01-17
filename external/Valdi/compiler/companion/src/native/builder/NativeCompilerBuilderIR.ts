import {
  ConstValue,
  NativeCompilerBuilderAtomID,
  NativeCompilerBuilderBranchType,
  NativeCompilerBuilderJumpTargetID,
  NativeCompilerBuilderVariableID,
} from './INativeCompilerBuilder';

export namespace NativeCompilerIR {
  export enum Kind {
    Slot,
    Global,
    Keyword,
    Exception,

    SetProperty,
    SetPropertyValue,
    SetPropertyIndex,
    CopyPropertiesFrom,
    GetProperty,
    GetPropertyFree,
    GetPropertyValue,
    DeleteProperty,
    DeletePropertyValue,

    GetFunctionArg,
    GetFunctionArgumentsObject,

    GetSuper,
    GetSuperConstructor,

    GetClosureArg,

    GetModuleConst,

    LiteralInteger,
    LiteralLong,
    LiteralDouble,
    LiteralString,
    LiteralBool,

    UnaryOp,
    BinaryOp,

    NewObject,
    NewArray,
    NewFunctionValue,
    NewArrowFunctionValue,
    NewClassValue,

    SetClassElement,
    SetClassElementValue,

    Retain,
    Free,
    FreeV,

    NewVariableRef,
    LoadVariableRef,
    SetVariableRef,

    IntrinsicCall,

    FunctionInvocation,
    ConstructorInvocation,
    BindJumpTarget,
    Jump,
    Branch,

    Iterator,
    IteratorNext,
    KeysIterator,
    KeysIteratorNext,

    Generator,
    Resume,
    Reentry,
    SetResumePoint,

    Assignment,
    GetException,
    CheckException,
    Throw,

    StartFunction,
    EndFunction,

    Comments,
    ProgramCounterInfo,

    BuilderStub,

    BoilerplatePrologue,
    BoilerplateEpilogue,
  }

  export enum KeywordKind {
    Undefined,
    Null,
    This,
    NewTarget,
  }

  export enum UnaryOperator {
    Neg,
    Plus,
    Inc,
    Dec,
    BitwiseNot,
    LogicalNot,
    TypeOf,
  }

  export enum BinaryOperator {
    Mult,
    Add,
    Sub,
    Div,
    LeftShift,
    RightShift,
    UnsignedRightShift,
    BitwiseXOR,
    BitwiseAND,
    BitwiseOR,
    LessThan,
    LessThanOrEqual,
    LessThanOrEqualEqual,
    GreaterThan,
    GreaterThanOrEqual,
    GreaterThanOrEqualEqual,
    EqualEqual,
    EqualEqualEqual,
    DifferentThan,
    DifferentThanStrict,
    Modulo,
    Exponentiation,
    InstanceOf,
    In,
  }

  export enum FunctionType {
    ModuleGenericFunction,
    ClassConstructor,
  }

  export enum FunctionTypeArgs {
    Context,
    This,
    StackFrame,
    Argc,
    Argv,
    Closure,
    NewTarget,
  }

  export const enum FunctionArgumentsType {
    /**
     * Arguments are passed directly.
     */
    Direct,
    /**
     * Arguments are passed through a TS array that
     * represents the arguments.
     */
    Indirect,
    /**
     * Arguments are forwarded from the arguments
     * that exists in the current call.
     */
    ForwardFromCurrentCall,
  }

  export enum ClassElementType {
    Method,
    Getter,
    Setter,
  }

  const FUNCTION_ARGS_BY_TYPE = {
    [NativeCompilerIR.FunctionType.ModuleGenericFunction]: [
      NativeCompilerIR.FunctionTypeArgs.Context,
      NativeCompilerIR.FunctionTypeArgs.This,
      NativeCompilerIR.FunctionTypeArgs.StackFrame,
      NativeCompilerIR.FunctionTypeArgs.Argc,
      NativeCompilerIR.FunctionTypeArgs.Argv,
      NativeCompilerIR.FunctionTypeArgs.Closure,
    ],
    [NativeCompilerIR.FunctionType.ClassConstructor]: [
      NativeCompilerIR.FunctionTypeArgs.Context,
      NativeCompilerIR.FunctionTypeArgs.NewTarget,
      NativeCompilerIR.FunctionTypeArgs.StackFrame,
      NativeCompilerIR.FunctionTypeArgs.Argc,
      NativeCompilerIR.FunctionTypeArgs.Argv,
      NativeCompilerIR.FunctionTypeArgs.Closure,
    ],
  };

  export function getFunctionArgsFromType(type: NativeCompilerIR.FunctionType): Array<FunctionTypeArgs> {
    return FUNCTION_ARGS_BY_TYPE[type];
  }

  export interface Base {
    readonly kind: Kind;
  }

  export interface BaseWithReturn extends Base {
    readonly variable: NativeCompilerBuilderVariableID;
  }

  export interface BaseWithExceptionTarget extends Base {
    readonly exceptionTarget: NativeCompilerBuilderJumpTargetID;
  }

  export interface BaseWithReturnAndExceptionTarget extends BaseWithReturn, BaseWithExceptionTarget {}

  interface LiteralBase extends BaseWithReturn {
    readonly value: string;
  }

  interface UnaryOPBase extends BaseWithReturnAndExceptionTarget {
    readonly operand: NativeCompilerBuilderVariableID;
  }

  interface BinaryOpBase extends BaseWithReturnAndExceptionTarget {
    readonly left: NativeCompilerBuilderVariableID;
    readonly right: NativeCompilerBuilderVariableID;
  }

  export interface Slot extends Base {
    readonly kind: Kind.Slot;
    readonly value: NativeCompilerBuilderVariableID;
  }

  export interface Global extends BaseWithReturn {
    readonly kind: Kind.Global;
  }

  export interface SetProperty extends BaseWithExceptionTarget {
    readonly kind: Kind.SetProperty;
    readonly object: NativeCompilerBuilderVariableID;
    readonly properties: NativeCompilerBuilderAtomID[];
    readonly values: NativeCompilerBuilderVariableID[];
  }

  export interface SetPropertyValue extends BaseWithExceptionTarget {
    readonly kind: Kind.SetPropertyValue;
    readonly object: NativeCompilerBuilderVariableID;
    readonly property: NativeCompilerBuilderVariableID;
    readonly value: NativeCompilerBuilderVariableID;
  }

  export interface SetPropertyIndex extends BaseWithExceptionTarget {
    readonly kind: Kind.SetPropertyIndex;
    readonly object: NativeCompilerBuilderVariableID;
    readonly index: number;
    readonly value: NativeCompilerBuilderVariableID;
  }

  export interface CopyPropertiesFrom extends BaseWithExceptionTarget {
    readonly kind: Kind.CopyPropertiesFrom;
    readonly object: NativeCompilerBuilderVariableID;
    readonly value: NativeCompilerBuilderVariableID;
    readonly propertiesToIgnore: NativeCompilerBuilderAtomID[] | undefined;
    readonly copiedPropertiesCount: NativeCompilerBuilderVariableID;
  }

  export interface Throw extends Base {
    readonly kind: Kind.Throw;
    readonly value: NativeCompilerBuilderVariableID;
    readonly target: NativeCompilerBuilderJumpTargetID;
  }

  export interface Retain extends Base {
    readonly kind: Kind.Retain;
    readonly value: NativeCompilerBuilderVariableID;
  }

  export interface Free extends Base {
    readonly kind: Kind.Free;
    readonly value: NativeCompilerBuilderVariableID;
  }

  export interface FreeV extends Base {
    readonly kind: Kind.FreeV;
    readonly values: NativeCompilerBuilderVariableID[];
  }

  /**
   * Creates a new variable ref which can hold variables that can be mutated
   * arbitrarily within closures.
   */
  export interface NewVariableRef extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.NewVariableRef;
    readonly name: string | undefined;
    readonly constValue?: ConstValue;
  }

  /**
   * Loads the content of a variable ref.
   */
  export interface LoadVariableRef extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.LoadVariableRef;
    readonly target: NativeCompilerBuilderVariableID;
  }

  /**
   * Sets the content of a variable ref.
   */
  export interface SetVariableRef extends Base {
    readonly kind: Kind.SetVariableRef;
    readonly target: NativeCompilerBuilderVariableID;
    readonly value: NativeCompilerBuilderVariableID;
  }

  export interface GetProperty extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.GetProperty;

    readonly object: NativeCompilerBuilderVariableID;
    readonly property: NativeCompilerBuilderAtomID;
    readonly thisObject: NativeCompilerBuilderVariableID;
    readonly propCacheSlot: number;
  }

  export interface GetPropertyFree extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.GetPropertyFree;

    readonly object: NativeCompilerBuilderVariableID;
    readonly property: NativeCompilerBuilderAtomID;
    readonly thisObject: NativeCompilerBuilderVariableID;
    readonly propCacheSlot: number;
  }

  export interface GetPropertyValue extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.GetPropertyValue;
    readonly object: NativeCompilerBuilderVariableID;
    readonly property: NativeCompilerBuilderVariableID;
    readonly thisObject: NativeCompilerBuilderVariableID;
  }

  export interface DeleteProperty extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.DeleteProperty;

    readonly object: NativeCompilerBuilderVariableID;
    readonly property: NativeCompilerBuilderAtomID;
  }

  export interface DeletePropertyValue extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.DeletePropertyValue;
    readonly object: NativeCompilerBuilderVariableID;
    readonly property: NativeCompilerBuilderVariableID;
  }

  export interface GetFunctionArg extends BaseWithReturn {
    readonly kind: Kind.GetFunctionArg;
    readonly index: number;
  }

  export interface GetFunctionArgumentsObject extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.GetFunctionArgumentsObject;
    readonly startIndex: number;
  }

  export interface GetSuper extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.GetSuper;
  }

  export interface GetSuperConstructor extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.GetSuperConstructor;
  }

  export interface GetClosureArg extends BaseWithReturn {
    readonly kind: Kind.GetClosureArg;
    readonly index: number;
    readonly constValue?: ConstValue;
  }

  export interface GetModuleConst extends BaseWithReturn {
    readonly kind: Kind.GetModuleConst;
    readonly index: number;
  }

  export interface GetException extends BaseWithReturn {
    readonly kind: Kind.GetException;
  }

  export interface CheckException extends BaseWithExceptionTarget {
    readonly kind: Kind.CheckException;
  }

  export interface Keyword extends BaseWithReturn {
    readonly kind: Kind.Keyword;
    readonly keyword: KeywordKind;
  }

  export interface Exception extends BaseWithReturn {
    readonly kind: Kind.Exception;
  }

  export interface LiteralInteger extends LiteralBase {
    readonly kind: Kind.LiteralInteger;
  }

  export interface LiteralLong extends LiteralBase {
    readonly kind: Kind.LiteralLong;
  }

  export interface LiteralDouble extends LiteralBase {
    readonly kind: Kind.LiteralDouble;
  }

  export interface LiteralString extends LiteralBase, BaseWithExceptionTarget {
    readonly kind: Kind.LiteralString;
  }

  export interface LiteralBool extends BaseWithReturn {
    readonly kind: Kind.LiteralBool;
    readonly value: boolean;
  }

  export interface UnaryOp extends UnaryOPBase {
    readonly kind: Kind.UnaryOp;
    readonly operator: UnaryOperator;
  }

  export interface BinaryOp extends BinaryOpBase {
    readonly kind: Kind.BinaryOp;
    readonly operator: BinaryOperator;
  }

  export interface Assignment extends Base {
    readonly kind: Kind.Assignment;
    readonly left: NativeCompilerBuilderVariableID;
    readonly right: NativeCompilerBuilderVariableID;
  }

  export interface IntrinsicCall extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.IntrinsicCall;
    readonly func: string;
    readonly args: Array<NativeCompilerBuilderVariableID>;
  }

  export interface FunctionInvocation extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.FunctionInvocation;
    readonly func: NativeCompilerBuilderVariableID;
    readonly args: Array<NativeCompilerBuilderVariableID>;
    readonly obj: NativeCompilerBuilderVariableID;
    readonly argsType: FunctionArgumentsType;
  }

  export interface ConstructorInvocation extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.ConstructorInvocation;
    readonly func: NativeCompilerBuilderVariableID;
    readonly new_target: NativeCompilerBuilderVariableID;
    readonly args: Array<NativeCompilerBuilderVariableID>;
    readonly argsType: FunctionArgumentsType;
  }

  export interface NewObject extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.NewObject;
  }

  export interface NewArray extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.NewArray;
  }

  export interface NewFunctionValueBase extends BaseWithReturnAndExceptionTarget {
    readonly functionName: string;
    readonly argc: number;
    readonly closureArgs: NativeCompilerBuilderVariableID[];
  }

  export interface NewArrowFunctionValue extends NewFunctionValueBase {
    readonly kind: Kind.NewArrowFunctionValue;
    readonly stackOnHeap: boolean;
  }

  export interface NewFunctionValue extends NewFunctionValueBase {
    readonly kind: Kind.NewFunctionValue;
    readonly name: NativeCompilerBuilderAtomID;
  }

  export interface NewClassValue extends NewFunctionValueBase {
    readonly kind: Kind.NewClassValue;
    readonly name: NativeCompilerBuilderAtomID;
    readonly parentClass: NativeCompilerBuilderVariableID;
  }

  export interface SetClassElement extends BaseWithExceptionTarget {
    readonly kind: Kind.SetClassElement;
    readonly type: ClassElementType;
    readonly cls: NativeCompilerBuilderVariableID;
    readonly name: NativeCompilerBuilderAtomID;
    readonly value: NativeCompilerBuilderVariableID;
    readonly static: boolean;
  }

  export interface SetClassElementValue extends BaseWithExceptionTarget {
    readonly kind: Kind.SetClassElementValue;
    readonly type: ClassElementType;
    readonly cls: NativeCompilerBuilderVariableID;
    readonly name: NativeCompilerBuilderVariableID;
    readonly value: NativeCompilerBuilderVariableID;
    readonly static: boolean;
  }

  export interface Iterator extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.Iterator;
    readonly arg: NativeCompilerBuilderVariableID;
  }

  export interface IteratorNext extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.IteratorNext;
    readonly iterator: NativeCompilerBuilderVariableID;
  }

  export interface KeysIterator extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.KeysIterator;
    readonly arg: NativeCompilerBuilderVariableID;
  }

  export interface KeysIteratorNext extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.KeysIteratorNext;
    readonly iterator: NativeCompilerBuilderVariableID;
  }

  export interface Generator extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.Generator;
    readonly arg: NativeCompilerBuilderVariableID;
  }

  export interface Resume extends BaseWithReturnAndExceptionTarget {
    readonly kind: Kind.Resume;
  }

  export interface Reentry extends Base {
    readonly kind: Kind.Reentry;
    readonly resumePoints: NativeCompilerBuilderJumpTargetID[];
  }

  export interface SetResumePoint extends Base {
    readonly kind: Kind.SetResumePoint;
    readonly resumePoint: NativeCompilerBuilderJumpTargetID;
  }

  export interface BindJumpTarget extends Base {
    readonly kind: Kind.BindJumpTarget;
    readonly target: NativeCompilerBuilderJumpTargetID;
  }

  export interface Jump extends Base {
    readonly kind: Kind.Jump;
    readonly target: NativeCompilerBuilderJumpTargetID;
  }

  export interface Branch extends Base {
    readonly kind: Kind.Branch;
    readonly conditionVariable: NativeCompilerBuilderVariableID;
    readonly type: NativeCompilerBuilderBranchType;
    readonly trueTarget: NativeCompilerBuilderJumpTargetID;
    readonly falseTarget: NativeCompilerBuilderJumpTargetID | undefined;
  }

  export interface StartFunction extends Base {
    readonly kind: Kind.StartFunction;
    readonly name: string;
    readonly type: FunctionType;
    readonly exceptionJumpTarget: NativeCompilerBuilderJumpTargetID;
    readonly returnJumpTarget: NativeCompilerBuilderJumpTargetID;
    readonly returnVariable: NativeCompilerBuilderVariableID;
    readonly localVars: NativeCompilerBuilderVariableID[];
  }

  export interface EndFunction extends Base {
    readonly kind: Kind.EndFunction;
    readonly returnVariable: NativeCompilerBuilderVariableID;
  }

  export interface Comments extends Base {
    readonly kind: Kind.Comments;
    readonly text: string;
  }

  export interface ProgramCounterInfo extends Base {
    readonly kind: Kind.ProgramCounterInfo;
    readonly lineNumber: number;
    readonly columnNumber: number;
  }

  export interface BoilerplateEpilogue extends Base {
    readonly kind: Kind.BoilerplateEpilogue;
    readonly moduleName: string;
    readonly modulePath: string;
    readonly moduleInitFunctionName: string;
    readonly atoms: readonly NativeCompilerBuilderAtomID[];
    readonly stringConstants: readonly string[];
    readonly propCacheSlots: number;
  }

  export interface BoilerplatePrologue extends Base {
    readonly kind: Kind.BoilerplatePrologue;
    readonly atoms: NativeCompilerBuilderAtomID[];
  }

  export interface BuilderStub extends Base {
    readonly kind: Kind.BuilderStub;
    readonly index: number;
  }
}
