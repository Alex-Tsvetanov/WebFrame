import {
  NativeCompilerBuilderVariableID,
  NativeCompilerBuilderJumpTargetID,
  NativeCompilerBuilderAtomID,
  NativeCompilerBuilderBranchType,
} from '../builder/INativeCompilerBuilder';
import { NativeCompilerIR } from '../builder/NativeCompilerBuilderIR';

export interface INativeCompilerIREmitter {
  emitBoilerplatePrologue(atoms: NativeCompilerBuilderAtomID[]): void;
  emitBoilerplateEpilogue(
    moduleName: string,
    modulePath: string,
    moduleInitFunctionName: string,
    atoms: readonly NativeCompilerBuilderAtomID[],
    stringConstants: readonly string[],
    propCacheSlots: number,
  ): void;

  emitStartFunction(
    name: string,
    type: NativeCompilerIR.FunctionType,
    exceptionJumpTarget: NativeCompilerBuilderJumpTargetID,
    returnJumpTarget: NativeCompilerBuilderJumpTargetID,
    returnVariable: NativeCompilerBuilderVariableID,
    localVars: NativeCompilerBuilderVariableID[],
  ): void;

  emitEndFunction(returnVariable: NativeCompilerBuilderVariableID): void;

  emitSlot(value: NativeCompilerBuilderVariableID): void;

  emitGlobal(variable: NativeCompilerBuilderVariableID): void;

  emitUndefined(variable: NativeCompilerBuilderVariableID): void;

  emitNull(variable: NativeCompilerBuilderVariableID): void;

  emitThis(variable: NativeCompilerBuilderVariableID): void;

  emitNewTarget(variable: NativeCompilerBuilderVariableID): void;

  emitException(variable: NativeCompilerBuilderVariableID): void;

  emitGetException(variable: NativeCompilerBuilderVariableID): void;

  emitCheckException(exceptionTarget: NativeCompilerBuilderJumpTargetID): void;

  emitSetProperty(
    object: NativeCompilerBuilderVariableID,
    properties: NativeCompilerBuilderAtomID[],
    valuees: NativeCompilerBuilderVariableID[],
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitSetPropertyValue(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
    value: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitSetPropertyIndex(
    object: NativeCompilerBuilderVariableID,
    index: number,
    value: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitCopyPropertiesFrom(
    object: NativeCompilerBuilderVariableID,
    value: NativeCompilerBuilderVariableID,
    propertiesToIgnore: NativeCompilerBuilderAtomID[] | undefined,
    copiedPropertiesCount: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitGetProperty(
    variable: NativeCompilerBuilderVariableID,
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
    thisObject: NativeCompilerBuilderVariableID,
    propCacheSlot: number,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;
  emitGetPropertyFree(
    variable: NativeCompilerBuilderVariableID,
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
    thisObject: NativeCompilerBuilderVariableID,
    propCacheSlot: number,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitGetPropertyValue(
    variable: NativeCompilerBuilderVariableID,
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
    thisObject: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitDeleteProperty(
    variable: NativeCompilerBuilderVariableID,
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitDeletePropertyValue(
    variable: NativeCompilerBuilderVariableID,
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitGetFunctionArg(variable: NativeCompilerBuilderVariableID, index: number): void;

  emitGetFunctionArgumentsObject(
    variable: NativeCompilerBuilderVariableID,
    startIndex: number,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitGetSuper(
    targetVariable: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;
  emitGetSuperConstructor(
    targetVariable: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitGetClosureArg(variable: NativeCompilerBuilderVariableID, index: number): void;

  emitGetModuleConst(variable: NativeCompilerBuilderVariableID, index: number): void;

  emitLiteralInteger(variable: NativeCompilerBuilderVariableID, value: string): void;

  emitLiteralLong(variable: NativeCompilerBuilderVariableID, value: string): void;

  emitLiteralDouble(variable: NativeCompilerBuilderVariableID, value: string): void;

  emitLiteralString(
    variable: NativeCompilerBuilderVariableID,
    value: string,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitLiteralBool(variable: NativeCompilerBuilderVariableID, value: boolean): void;

  emitUnaryOp(
    operator: NativeCompilerIR.UnaryOperator,
    variable: NativeCompilerBuilderVariableID,
    operand: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitBinaryOp(
    variable: NativeCompilerBuilderVariableID,
    operator: NativeCompilerIR.BinaryOperator,
    left: NativeCompilerBuilderVariableID,
    right: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitNewObject(variable: NativeCompilerBuilderVariableID, exceptionTarget: NativeCompilerBuilderJumpTargetID): void;

  emitNewArray(variable: NativeCompilerBuilderVariableID, exceptionTarget: NativeCompilerBuilderJumpTargetID): void;

  emitNewArrowFunctionValue(
    variable: NativeCompilerBuilderVariableID,
    functionName: string,
    argc: number,
    closureVariables: NativeCompilerBuilderVariableID[],
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
    stackOnHeap: boolean,
  ): void;

  emitNewFunctionValue(
    variable: NativeCompilerBuilderVariableID,
    functionName: string,
    name: NativeCompilerBuilderAtomID | undefined,
    argc: number,
    closureVariables: NativeCompilerBuilderVariableID[],
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitNewClassValue(
    variable: NativeCompilerBuilderVariableID,
    functionName: string,
    constructorName: NativeCompilerBuilderAtomID | undefined,
    argc: number,
    parentClass: NativeCompilerBuilderVariableID,
    closureVariables: NativeCompilerBuilderVariableID[],
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitSetClassMethod(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderAtomID,
    method: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitSetClassMethodValue(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderVariableID,
    method: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitSetClassPropertyGetter(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderAtomID,
    getter: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitSetClassPropertyGetterValue(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderVariableID,
    getter: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitSetClassPropertySetter(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderAtomID,
    setter: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitSetClassPropertySetterValue(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderVariableID,
    setter: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitRetain(value: NativeCompilerBuilderVariableID): void;

  emitFree(value: NativeCompilerBuilderVariableID): void;
  emitFreeV(values: NativeCompilerBuilderVariableID[]): void;

  emitIntrinsicCall(
    variable: NativeCompilerBuilderVariableID,
    func: string,
    args: Array<NativeCompilerBuilderVariableID>,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitFunctionInvocation(
    variable: NativeCompilerBuilderVariableID,
    func: NativeCompilerBuilderVariableID,
    args: Array<NativeCompilerBuilderVariableID>,
    obj: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitFunctionInvocationWithArgsArray(
    variable: NativeCompilerBuilderVariableID,
    func: NativeCompilerBuilderVariableID,
    args: NativeCompilerBuilderVariableID,
    obj: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitFunctionInvocationWithForwardedArgs(
    variable: NativeCompilerBuilderVariableID,
    func: NativeCompilerBuilderVariableID,
    obj: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitConstructorInvocation(
    variable: NativeCompilerBuilderVariableID,
    func: NativeCompilerBuilderVariableID,
    new_target: NativeCompilerBuilderVariableID,
    args: Array<NativeCompilerBuilderVariableID>,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitConstructorInvocationWithArgsArray(
    variable: NativeCompilerBuilderVariableID,
    func: NativeCompilerBuilderVariableID,
    new_target: NativeCompilerBuilderVariableID,
    args: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitBindJumpTarget(target: NativeCompilerBuilderJumpTargetID): void;

  emitJump(target: NativeCompilerBuilderJumpTargetID): void;

  emitBranch(
    conditionVariable: NativeCompilerBuilderVariableID,
    branchType: NativeCompilerBuilderBranchType,
    trueTarget: NativeCompilerBuilderJumpTargetID,
    falseTarget: NativeCompilerBuilderJumpTargetID | undefined,
  ): void;

  emitAssignment(left: NativeCompilerBuilderVariableID, right: NativeCompilerBuilderVariableID): void;

  emitThrow(value: NativeCompilerBuilderVariableID, target: NativeCompilerBuilderJumpTargetID): void;

  emitComments(text: string): void;

  emitProgramCounterInfo(lineNumber: number, columnNumber: number): void;

  emitNewVariableRef(value: NativeCompilerBuilderVariableID, exceptionTarget: NativeCompilerBuilderJumpTargetID): void;
  emitLoadVariableRef(
    variable: NativeCompilerBuilderVariableID,
    variableRefId: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;
  emitSetVariableRef(value: NativeCompilerBuilderVariableID, variableRefId: NativeCompilerBuilderVariableID): void;

  emitIterator(
    value: NativeCompilerBuilderVariableID,
    output: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;
  emitKeysIterator(
    value: NativeCompilerBuilderVariableID,
    output: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;
  emitIteratorNext(
    iterator: NativeCompilerBuilderVariableID,
    output: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;
  emitKeysIteratorNext(
    iterator: NativeCompilerBuilderVariableID,
    output: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;

  emitGenerator(
    arg: NativeCompilerBuilderVariableID,
    variable: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void;
  emitResume(variable: NativeCompilerBuilderVariableID, exceptionTarget: NativeCompilerBuilderJumpTargetID): void;
  emitReentry(resumePoints: NativeCompilerBuilderJumpTargetID[]): void;
  emitSetResumePoint(resumePoint: NativeCompilerBuilderJumpTargetID): void;

  finalize(): NativeCompilerEmitterOutput;
}

export interface NativeCompilerEmitterOutput {
  source: string;
}

export function emitterWalk(emitter: INativeCompilerIREmitter, ir: Array<NativeCompilerIR.Base>): void {
  ir.forEach((ir_) => {
    switch (ir_.kind) {
      case NativeCompilerIR.Kind.BoilerplatePrologue: {
        const ir = ir_ as NativeCompilerIR.BoilerplatePrologue;
        emitter.emitBoilerplatePrologue(ir.atoms);
        break;
      }
      case NativeCompilerIR.Kind.BoilerplateEpilogue: {
        const ir = ir_ as NativeCompilerIR.BoilerplateEpilogue;
        emitter.emitBoilerplateEpilogue(
          ir.moduleName,
          ir.modulePath,
          ir.moduleInitFunctionName,
          ir.atoms,
          ir.stringConstants,
          ir.propCacheSlots,
        );
        break;
      }
      case NativeCompilerIR.Kind.StartFunction: {
        const ir = ir_ as NativeCompilerIR.StartFunction;
        emitter.emitStartFunction(
          ir.name,
          ir.type,
          ir.exceptionJumpTarget,
          ir.returnJumpTarget,
          ir.returnVariable,
          ir.localVars,
        );
        break;
      }
      case NativeCompilerIR.Kind.EndFunction: {
        const ir = ir_ as NativeCompilerIR.EndFunction;
        emitter.emitEndFunction(ir.returnVariable);
        break;
      }
      case NativeCompilerIR.Kind.Slot: {
        const ir = ir_ as NativeCompilerIR.Slot;
        emitter.emitSlot(ir.value);
        break;
      }
      case NativeCompilerIR.Kind.Global: {
        const ir = ir_ as NativeCompilerIR.Global;
        emitter.emitGlobal(ir.variable);
        break;
      }
      case NativeCompilerIR.Kind.Keyword: {
        const ir = ir_ as NativeCompilerIR.Keyword;
        switch (ir.keyword) {
          case NativeCompilerIR.KeywordKind.Undefined:
            emitter.emitUndefined(ir.variable);
            break;
          case NativeCompilerIR.KeywordKind.Null:
            emitter.emitNull(ir.variable);
            break;
          case NativeCompilerIR.KeywordKind.This:
            emitter.emitThis(ir.variable);
            break;
          case NativeCompilerIR.KeywordKind.NewTarget:
            emitter.emitNewTarget(ir.variable);
            break;
        }
        break;
      }
      case NativeCompilerIR.Kind.Exception: {
        const ir = ir_ as NativeCompilerIR.Exception;
        emitter.emitException(ir.variable);
        break;
      }
      case NativeCompilerIR.Kind.SetProperty: {
        const ir = ir_ as NativeCompilerIR.SetProperty;
        emitter.emitSetProperty(ir.object, ir.properties, ir.values, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.SetPropertyValue: {
        const ir = ir_ as NativeCompilerIR.SetPropertyValue;
        emitter.emitSetPropertyValue(ir.object, ir.property, ir.value, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.SetPropertyIndex: {
        const ir = ir_ as NativeCompilerIR.SetPropertyIndex;
        emitter.emitSetPropertyIndex(ir.object, ir.index, ir.value, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.CopyPropertiesFrom: {
        const ir = ir_ as NativeCompilerIR.CopyPropertiesFrom;
        emitter.emitCopyPropertiesFrom(
          ir.object,
          ir.value,
          ir.propertiesToIgnore,
          ir.copiedPropertiesCount,
          ir.exceptionTarget,
        );
        break;
      }
      case NativeCompilerIR.Kind.GetProperty: {
        const ir = ir_ as NativeCompilerIR.GetProperty;
        emitter.emitGetProperty(
          ir.variable,
          ir.object,
          ir.property,
          ir.thisObject,
          ir.propCacheSlot,
          ir.exceptionTarget,
        );
        break;
      }
      case NativeCompilerIR.Kind.GetPropertyFree: {
        const ir = ir_ as NativeCompilerIR.GetPropertyFree;
        emitter.emitGetPropertyFree(
          ir.variable,
          ir.object,
          ir.property,
          ir.thisObject,
          ir.propCacheSlot,
          ir.exceptionTarget,
        );
        break;
      }
      case NativeCompilerIR.Kind.GetPropertyValue: {
        const ir = ir_ as NativeCompilerIR.GetPropertyValue;
        emitter.emitGetPropertyValue(ir.variable, ir.object, ir.property, ir.thisObject, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.DeleteProperty: {
        const ir = ir_ as NativeCompilerIR.DeleteProperty;
        emitter.emitDeleteProperty(ir.variable, ir.object, ir.property, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.DeletePropertyValue: {
        const ir = ir_ as NativeCompilerIR.DeletePropertyValue;
        emitter.emitDeletePropertyValue(ir.variable, ir.object, ir.property, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.GetFunctionArg: {
        const ir = ir_ as NativeCompilerIR.GetFunctionArg;
        emitter.emitGetFunctionArg(ir.variable, ir.index);
        break;
      }
      case NativeCompilerIR.Kind.GetFunctionArgumentsObject: {
        const ir = ir_ as NativeCompilerIR.GetFunctionArgumentsObject;
        emitter.emitGetFunctionArgumentsObject(ir.variable, ir.startIndex, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.GetSuper: {
        const ir = ir_ as NativeCompilerIR.GetSuper;
        emitter.emitGetSuper(ir.variable, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.GetSuperConstructor: {
        const ir = ir_ as NativeCompilerIR.GetSuperConstructor;
        emitter.emitGetSuperConstructor(ir.variable, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.GetClosureArg: {
        const ir = ir_ as NativeCompilerIR.GetClosureArg;
        emitter.emitGetClosureArg(ir.variable, ir.index);
        break;
      }
      case NativeCompilerIR.Kind.GetModuleConst: {
        const ir = ir_ as NativeCompilerIR.GetModuleConst;
        emitter.emitGetModuleConst(ir.variable, ir.index);
        break;
      }
      case NativeCompilerIR.Kind.LiteralInteger: {
        const ir = ir_ as NativeCompilerIR.LiteralInteger;
        emitter.emitLiteralInteger(ir.variable, ir.value);
        break;
      }
      case NativeCompilerIR.Kind.LiteralLong: {
        const ir = ir_ as NativeCompilerIR.LiteralInteger;
        emitter.emitLiteralLong(ir.variable, ir.value);
        break;
      }
      case NativeCompilerIR.Kind.LiteralDouble: {
        const ir = ir_ as NativeCompilerIR.LiteralDouble;
        emitter.emitLiteralDouble(ir.variable, ir.value);
        break;
      }
      case NativeCompilerIR.Kind.LiteralString: {
        const ir = ir_ as NativeCompilerIR.LiteralString;
        emitter.emitLiteralString(ir.variable, ir.value, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.LiteralBool: {
        const ir = ir_ as NativeCompilerIR.LiteralBool;
        emitter.emitLiteralBool(ir.variable, ir.value);
        break;
      }
      case NativeCompilerIR.Kind.UnaryOp: {
        const ir = ir_ as NativeCompilerIR.UnaryOp;
        emitter.emitUnaryOp(ir.operator, ir.variable, ir.operand, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.BinaryOp: {
        const ir = ir_ as NativeCompilerIR.BinaryOp;
        emitter.emitBinaryOp(ir.variable, ir.operator, ir.left, ir.right, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.NewObject: {
        const ir = ir_ as NativeCompilerIR.NewObject;
        emitter.emitNewObject(ir.variable, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.NewArray: {
        const ir = ir_ as NativeCompilerIR.NewArray;
        emitter.emitNewArray(ir.variable, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.NewArrowFunctionValue: {
        const ir = ir_ as NativeCompilerIR.NewArrowFunctionValue;
        emitter.emitNewArrowFunctionValue(
          ir.variable,
          ir.functionName,
          ir.argc,
          ir.closureArgs,
          ir.exceptionTarget,
          ir.stackOnHeap,
        );
        break;
      }
      case NativeCompilerIR.Kind.NewFunctionValue: {
        const ir = ir_ as NativeCompilerIR.NewFunctionValue;
        emitter.emitNewFunctionValue(
          ir.variable,
          ir.functionName,
          ir.name,
          ir.argc,
          ir.closureArgs,
          ir.exceptionTarget,
        );
        break;
      }
      case NativeCompilerIR.Kind.NewClassValue: {
        const ir = ir_ as NativeCompilerIR.NewClassValue;
        emitter.emitNewClassValue(
          ir.variable,
          ir.functionName,
          ir.name,
          ir.argc,
          ir.parentClass,
          ir.closureArgs,
          ir.exceptionTarget,
        );
        break;
      }
      case NativeCompilerIR.Kind.SetClassElement: {
        const ir = ir_ as NativeCompilerIR.SetClassElement;
        switch (ir.type) {
          case NativeCompilerIR.ClassElementType.Method:
            emitter.emitSetClassMethod(ir.cls, ir.name, ir.value, ir.static, ir.exceptionTarget);
            break;
          case NativeCompilerIR.ClassElementType.Getter:
            emitter.emitSetClassPropertyGetter(ir.cls, ir.name, ir.value, ir.static, ir.exceptionTarget);
            break;
          case NativeCompilerIR.ClassElementType.Setter:
            emitter.emitSetClassPropertySetter(ir.cls, ir.name, ir.value, ir.static, ir.exceptionTarget);
            break;
        }
        break;
      }
      case NativeCompilerIR.Kind.SetClassElementValue: {
        const ir = ir_ as NativeCompilerIR.SetClassElementValue;
        switch (ir.type) {
          case NativeCompilerIR.ClassElementType.Method:
            emitter.emitSetClassMethodValue(ir.cls, ir.name, ir.value, ir.static, ir.exceptionTarget);
            break;
          case NativeCompilerIR.ClassElementType.Getter:
            emitter.emitSetClassPropertyGetterValue(ir.cls, ir.name, ir.value, ir.static, ir.exceptionTarget);
            break;
          case NativeCompilerIR.ClassElementType.Setter:
            emitter.emitSetClassPropertySetterValue(ir.cls, ir.name, ir.value, ir.static, ir.exceptionTarget);
            break;
        }
        break;
      }
      case NativeCompilerIR.Kind.Retain: {
        const ir = ir_ as NativeCompilerIR.Retain;
        emitter.emitRetain(ir.value);
        break;
      }

      case NativeCompilerIR.Kind.Free: {
        const ir = ir_ as NativeCompilerIR.Free;
        emitter.emitFree(ir.value);
        break;
      }
      case NativeCompilerIR.Kind.FreeV: {
        const ir = ir_ as NativeCompilerIR.FreeV;
        emitter.emitFreeV(ir.values);
        break;
      }
      case NativeCompilerIR.Kind.IntrinsicCall: {
        const ir = ir_ as NativeCompilerIR.IntrinsicCall;
        emitter.emitIntrinsicCall(ir.variable, ir.func, ir.args, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.FunctionInvocation: {
        const ir = ir_ as NativeCompilerIR.FunctionInvocation;
        switch (ir.argsType) {
          case NativeCompilerIR.FunctionArgumentsType.Direct:
            emitter.emitFunctionInvocation(ir.variable, ir.func, ir.args, ir.obj, ir.exceptionTarget);
            break;
          case NativeCompilerIR.FunctionArgumentsType.Indirect:
            emitter.emitFunctionInvocationWithArgsArray(ir.variable, ir.func, ir.args[0], ir.obj, ir.exceptionTarget);
            break;
          case NativeCompilerIR.FunctionArgumentsType.ForwardFromCurrentCall:
            emitter.emitFunctionInvocationWithForwardedArgs(ir.variable, ir.func, ir.obj, ir.exceptionTarget);
            break;
        }
        break;
      }
      case NativeCompilerIR.Kind.ConstructorInvocation: {
        const ir = ir_ as NativeCompilerIR.ConstructorInvocation;

        switch (ir.argsType) {
          case NativeCompilerIR.FunctionArgumentsType.Direct:
            emitter.emitConstructorInvocation(ir.variable, ir.func, ir.new_target, ir.args, ir.exceptionTarget);
            break;
          case NativeCompilerIR.FunctionArgumentsType.Indirect:
            emitter.emitConstructorInvocationWithArgsArray(
              ir.variable,
              ir.func,
              ir.new_target,
              ir.args[0],
              ir.exceptionTarget,
            );
            break;
          case NativeCompilerIR.FunctionArgumentsType.ForwardFromCurrentCall:
            throw new Error('Unexpectedly got constructor invocation with forwarded args');
        }
        break;
      }
      case NativeCompilerIR.Kind.BindJumpTarget: {
        const ir = ir_ as NativeCompilerIR.BindJumpTarget;
        emitter.emitBindJumpTarget(ir.target);
        break;
      }
      case NativeCompilerIR.Kind.Jump: {
        const ir = ir_ as NativeCompilerIR.Jump;
        emitter.emitJump(ir.target);
        break;
      }
      case NativeCompilerIR.Kind.Branch: {
        const ir = ir_ as NativeCompilerIR.Branch;
        emitter.emitBranch(ir.conditionVariable, ir.type, ir.trueTarget, ir.falseTarget);
        break;
      }
      case NativeCompilerIR.Kind.Assignment: {
        const ir = ir_ as NativeCompilerIR.Assignment;
        emitter.emitAssignment(ir.left, ir.right);
        break;
      }
      case NativeCompilerIR.Kind.Throw: {
        const ir = ir_ as NativeCompilerIR.Throw;
        emitter.emitThrow(ir.value, ir.target);
        break;
      }
      case NativeCompilerIR.Kind.GetException: {
        const ir = ir_ as NativeCompilerIR.GetException;
        emitter.emitGetException(ir.variable);
        break;
      }
      case NativeCompilerIR.Kind.CheckException: {
        const ir = ir_ as NativeCompilerIR.CheckException;
        emitter.emitCheckException(ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.Comments: {
        const ir = ir_ as NativeCompilerIR.Comments;
        emitter.emitComments(ir.text);
        break;
      }
      case NativeCompilerIR.Kind.ProgramCounterInfo: {
        const ir = ir_ as NativeCompilerIR.ProgramCounterInfo;
        emitter.emitProgramCounterInfo(ir.lineNumber, ir.columnNumber);
        break;
      }
      case NativeCompilerIR.Kind.NewVariableRef: {
        const ir = ir_ as NativeCompilerIR.NewVariableRef;
        emitter.emitNewVariableRef(ir.variable, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.LoadVariableRef: {
        const ir = ir_ as NativeCompilerIR.LoadVariableRef;
        emitter.emitLoadVariableRef(ir.variable, ir.target, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.SetVariableRef: {
        const ir = ir_ as NativeCompilerIR.SetVariableRef;
        emitter.emitSetVariableRef(ir.value, ir.target);
        break;
      }
      case NativeCompilerIR.Kind.Iterator: {
        const ir = ir_ as NativeCompilerIR.Iterator;
        emitter.emitIterator(ir.arg, ir.variable, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.KeysIterator: {
        const ir = ir_ as NativeCompilerIR.KeysIterator;
        emitter.emitKeysIterator(ir.arg, ir.variable, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.IteratorNext: {
        const ir = ir_ as NativeCompilerIR.IteratorNext;
        emitter.emitIteratorNext(ir.iterator, ir.variable, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.KeysIteratorNext: {
        const ir = ir_ as NativeCompilerIR.KeysIteratorNext;
        emitter.emitKeysIteratorNext(ir.iterator, ir.variable, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.Generator: {
        const ir = ir_ as NativeCompilerIR.Generator;
        emitter.emitGenerator(ir.arg, ir.variable, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.Resume: {
        const ir = ir_ as NativeCompilerIR.Resume;
        emitter.emitResume(ir.variable, ir.exceptionTarget);
        break;
      }
      case NativeCompilerIR.Kind.Reentry: {
        const ir = ir_ as NativeCompilerIR.Reentry;
        emitter.emitReentry(ir.resumePoints);
        break;
      }
      case NativeCompilerIR.Kind.SetResumePoint: {
        const ir = ir_ as NativeCompilerIR.SetResumePoint;
        emitter.emitSetResumePoint(ir.resumePoint);
        break;
      }
      default:
        throw new Error(`Not all IRs are supported: ${ir_.kind}`);
    }
  });
}
