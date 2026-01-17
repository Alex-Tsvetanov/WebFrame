import {
  INativeCompilerBlockBuilder,
  NativeCompilerBuilderVariableID,
  NativeCompilerBuilderAtomID,
  NativeCompilerBuilderJumpTargetID,
  INativeCompilerJumpTargetBuilder,
  INativeCompilerLoopBuilder,
  INativeCompilerFunctionBuilder,
  INativeCompilerTryCatchBuilder,
  NativeCompilerBuilderVariableType,
  NativeCompilerBuilderBranchType,
  ConstValue,
  NativeCompilerBuilderVariableRef,
  VariableRefScope,
  IVariableDelegate,
} from './INativeCompilerBuilder';
import {
  INativeCompilerBlockBuilderFunctionContextListener,
  NativeCompilerBlockBuilderFunctionContext,
  NativeCompilerBlockBuilderScopeContext,
} from './internal/NativeCompilerBlockBuilderContext';
import { NativeCompilerIR } from './NativeCompilerBuilderIR';
import { NativeCompilerTransformerResolveBuilderStubs } from './internal/transformers/NativeCompilerTransformerResolveBuilderStubs';
import { NativeCompilerTransformerResolveSlots } from './internal/transformers/NativeCompilerTransformerResolveSlots';
import { NativeCompilerTransformerInsertRetainRelease } from './internal/transformers/NativeCompilerTransformerInsertRetainRelease';
import { NativeCompilerOptions } from '../NativeCompilerOptions';
import { NativeCompilerTransformerOptimizeVarRefs } from './internal/transformers/NativeCompilerTransformerOptimizeVarRefs';
import { NativeCompilerTransformerOptimizeLoads } from './internal/transformers/NativeCompilerTransformerOptimizeLoads';
import { NativeCompilerTransformerMergeRelease } from './internal/transformers/NativeCompilerTransformerMergeRelease';
import { NativeCompilerTransformerOptimizeAssignments } from './internal/transformers/NativeCompilerTransformerOptimizeAssignments';
import { NativeCompilerTransformerConstantFolding } from './internal/transformers/NativeCompilerTransformerConstantFolding';
import { NativeCompilerTransformerAutoRelease } from './internal/transformers/NativeCompilerTransformerAutoRelease';

import {
  VariableContext,
  VariableResolvingMode,
  variableResultIsVariableDelegate,
  variableResultIsVariableRef,
} from './VariableContext';

const intrinsicFunctionNames = new Map<string, string>([
  ['Math.log', 'tsn_math_log'],
  ['Math.pow', 'tsn_math_pow'],
  ['Math.floor', 'tsn_math_floor'],
  // ['Math.max', 'tsn_math_max_v'],
]);

const intrinsicConstants = new Map<string, number | string>([
  ['Math.PI', Math.PI],
  ['Math.E', Math.E],
  ['Math.LN10', Math.LN10],
  ['Math.LN2', Math.LN2],
  ['Math.LOG10E', Math.LOG10E],
  ['Math.LOG2E', Math.LOG2E],
  ['Math.SQRT1_2', Math.SQRT1_2],
  ['Math.SQRT2', Math.SQRT2],
]);

//TODO(5174):Should make variables read only this will allow for optimizations down the road
export class NativeCompilerBlockBuilder implements INativeCompilerBlockBuilder {
  private ir = new Array<NativeCompilerIR.Base>();
  private subbuilder = new Array<NativeCompilerBlockBuilder>();

  private globalVariable: NativeCompilerBuilderVariableID | undefined;
  private variableRefIRInsertionIndex = 0;
  private lastLineNumber = 0;

  constructor(
    private readonly context: NativeCompilerBlockBuilderFunctionContext,
    readonly scopeContext: NativeCompilerBlockBuilderScopeContext,
    readonly variableContext: VariableContext,
    public closureBlock: INativeCompilerBlockBuilder | undefined,
  ) {}

  private internalBuildSubBuilder(
    parentScope: NativeCompilerBlockBuilderScopeContext,
    variableContext: VariableContext,
  ): NativeCompilerBlockBuilder {
    const retval = new NativeCompilerBlockBuilder(this.context, parentScope, variableContext, this.closureBlock);
    const ir: NativeCompilerIR.BuilderStub = {
      kind: NativeCompilerIR.Kind.BuilderStub,
      index: this.subbuilder.length,
    };
    this.ir.push(ir);
    this.subbuilder.push(retval);
    return retval;
  }

  registerExceptionTargetInScope(target: NativeCompilerBuilderJumpTargetID): void {
    this.scopeContext.registerExceptionTargetInScope(target);
  }

  resolveExceptionTargetInScope(): NativeCompilerBuilderJumpTargetID | undefined {
    return this.scopeContext.resolveExceptionTargetInScope();
  }

  resolveExceptionTargetInScopeOrThrow(): NativeCompilerBuilderJumpTargetID {
    const jumpTarget = this.resolveExceptionTargetInScope();
    if (jumpTarget === undefined) {
      throw new Error('Could not resolve exception jump target');
    }

    return jumpTarget;
  }

  registerLazyVariableRef(
    variableName: string,
    scope: VariableRefScope,
    doBuild: (builder: INativeCompilerBlockBuilder) => NativeCompilerBuilderVariableRef,
  ): void {
    const irEnd = this.ir.length;

    this.variableContext.registerLazyVariableRef(variableName, () => {
      return this.buildWithIRInsertionIndex(irEnd, doBuild);
    });
  }

  private buildWithIRInsertionIndex<T>(
    insertionIndex: number,
    doBuild: (builder: INativeCompilerBlockBuilder) => T,
  ): T {
    const savedIR = this.ir;
    this.ir = [];
    const result = doBuild(this);
    // Restore IR
    const newIR = this.ir;
    this.ir = [...savedIR.slice(0, insertionIndex), ...newIR, ...savedIR.slice(insertionIndex)];

    return result;
  }

  private checkVariableId(variableId: NativeCompilerBuilderVariableID) {
    if (variableId.functionId !== this.context.functionId) {
      throw new Error('Attempting to manipulate variables from a different function id');
    }
  }

  private checkVariableRef(variableRef: NativeCompilerBuilderVariableRef) {
    this.checkVariableId(variableRef.variableId);
  }

  buildUninitializedVariableRef(
    variableName: string,
    type: NativeCompilerBuilderVariableType,
    constValue?: ConstValue,
  ): NativeCompilerBuilderVariableRef {
    const variableRef = new NativeCompilerBuilderVariableRef(
      variableName,
      type,
      this.context.registerVariable(NativeCompilerBuilderVariableType.VariableRef),
      constValue,
    );

    this.variableContext.registerVariableRefInScope(variableRef);

    return variableRef;
  }

  private doBuildVariableRef(
    variableName: string,
    type: NativeCompilerBuilderVariableType,
    isConst?: boolean,
  ): NativeCompilerBuilderVariableRef {
    const variableRef = this.buildUninitializedVariableRef(
      variableName,
      type,
      isConst ? new ConstValue(type) : undefined,
    );

    const ir: NativeCompilerIR.NewVariableRef = {
      kind: NativeCompilerIR.Kind.NewVariableRef,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      variable: variableRef.variableId!,
      name: variableName,
      constValue: variableRef.constValue,
    };
    this.ir.push(ir);

    return variableRef;
  }

  buildVariableRef(
    variableName: string,
    type: NativeCompilerBuilderVariableType,
    scope: VariableRefScope,
    isConst?: boolean,
  ): NativeCompilerBuilderVariableRef {
    switch (scope) {
      case VariableRefScope.Block: {
        return this.doBuildVariableRef(variableName, type, isConst);
      }
      case VariableRefScope.Function: {
        if (this.closureBlock) {
          return this.closureBlock.buildVariableRef(variableName, type, scope, isConst);
        }

        const irLengthBefore = this.ir.length;
        const variableRef = this.buildWithIRInsertionIndex(this.variableRefIRInsertionIndex, () => {
          // 'var' declarations can be re-declared. We should re-use the existing variable in that case.
          let existingVariable = this.doResolveVariableRefOrDelegate(variableName, VariableResolvingMode.shallow);
          if (existingVariable instanceof NativeCompilerBuilderVariableRef) {
            return existingVariable;
          }

          return this.doBuildVariableRef(variableName, type, isConst);
        });

        this.variableRefIRInsertionIndex += this.ir.length - irLengthBefore;

        return variableRef;
      }
    }
  }

  buildInitializedVariableRef(
    variableName: string,
    value: NativeCompilerBuilderVariableID,
    scope: VariableRefScope,
  ): NativeCompilerBuilderVariableRef {
    this.checkVariableId(value);
    const variableRef = this.buildVariableRef(variableName, value.type, scope);
    this.buildSetVariableRef(variableRef, value);
    return variableRef;
  }

  buildLoadVariableRef(variableRef: NativeCompilerBuilderVariableRef): NativeCompilerBuilderVariableID {
    this.checkVariableRef(variableRef);

    const variable = this.context.registerVariable(variableRef.type);
    const ir: NativeCompilerIR.LoadVariableRef = {
      kind: NativeCompilerIR.Kind.LoadVariableRef,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      variable,
      target: variableRef.variableId,
    };
    this.ir.push(ir);
    this.variableContext.registerVariableRefAlias(variableRef, variable);
    return variable;
  }

  buildSetVariableRef(variableRef: NativeCompilerBuilderVariableRef, value: NativeCompilerBuilderVariableID): void {
    this.checkVariableRef(variableRef);

    variableRef.addType(value.type);

    const ir: NativeCompilerIR.SetVariableRef = {
      kind: NativeCompilerIR.Kind.SetVariableRef,
      target: variableRef.variableId,
      value: value,
    };

    this.ir.push(ir);
  }

  registerVariableDelegate(variableName: string, delegate: IVariableDelegate): void {
    this.variableContext.registerVariableDelegate(variableName, delegate);
  }

  private doResolveVariableRefOrDelegate(
    variableName: string,
    scope: VariableResolvingMode,
  ): NativeCompilerBuilderVariableRef | IVariableDelegate | undefined {
    return this.variableContext.resolveVariable(variableName, scope);
  }

  resolveVariableRefOrDelegateInScope(
    variableName: string,
  ): NativeCompilerBuilderVariableRef | IVariableDelegate | undefined {
    return this.doResolveVariableRefOrDelegate(variableName, VariableResolvingMode.full);
  }

  resolveVariableInScope(variableName: string): NativeCompilerBuilderVariableID | undefined {
    const variableRefOrDelegate = this.resolveVariableRefOrDelegateInScope(variableName);
    if (!variableRefOrDelegate) {
      return undefined;
    }

    if (variableRefOrDelegate instanceof NativeCompilerBuilderVariableRef) {
      return this.buildLoadVariableRef(variableRefOrDelegate);
    } else {
      return variableRefOrDelegate.onGetValue(this);
    }
  }

  registerExitTargetInScope(target: NativeCompilerBuilderJumpTargetID): void {
    this.scopeContext.registerExitTargetInScope(target);
  }

  resolveExitTargetInScope(): NativeCompilerBuilderJumpTargetID | undefined {
    return this.scopeContext.resolveExitTargetInScope();
  }

  registerContinueTargetInScope(target: NativeCompilerBuilderJumpTargetID): void {
    this.scopeContext.registerContinueTargetInScope(target);
  }

  resolveContinueTargetInScope(): NativeCompilerBuilderJumpTargetID | undefined {
    return this.scopeContext.resolveContinueTargetInScope();
  }

  buildFunctionBodySubBuilder(closureBlockBuilder: INativeCompilerBlockBuilder): NativeCompilerBlockBuilder {
    const ret = this.buildSubBuilder(false);
    ret.closureBlock = closureBlockBuilder;

    return ret;
  }

  buildSubBuilder(scoped: boolean): NativeCompilerBlockBuilder {
    const parentScope = this.scopeContext;
    let retval: NativeCompilerBlockBuilder;
    if (scoped) {
      retval = this.internalBuildSubBuilder(parentScope.createSubScopeContext(), this.variableContext.scoped());
    } else {
      retval = this.internalBuildSubBuilder(parentScope, this.variableContext);
    }
    return retval;
  }

  buildJumpTarget(tag: string): INativeCompilerJumpTargetBuilder {
    const jumpTarget = this.context.registerJumpTarget(tag);
    const ir: NativeCompilerIR.BindJumpTarget = {
      kind: NativeCompilerIR.Kind.BindJumpTarget,
      target: jumpTarget,
    };
    this.ir.push(ir);
    return { target: jumpTarget, builder: this.buildSubBuilder(false) };
  }

  buildJump(jumpTarget: NativeCompilerBuilderJumpTargetID): void {
    const ir: NativeCompilerIR.Jump = {
      kind: NativeCompilerIR.Kind.Jump,
      target: jumpTarget,
    };
    this.ir.push(ir);
  }

  buildBranch(
    variable: NativeCompilerBuilderVariableID,
    type: NativeCompilerBuilderBranchType,
    trueTarget: NativeCompilerBuilderJumpTargetID,
    falseTarget: NativeCompilerBuilderJumpTargetID | undefined,
  ): void {
    this.checkVariableId(variable);
    const ir: NativeCompilerIR.Branch = {
      kind: NativeCompilerIR.Kind.Branch,
      conditionVariable: variable,
      type: type,
      trueTarget: trueTarget,
      falseTarget: falseTarget,
    };
    this.ir.push(ir);
  }

  buildThrow(value: NativeCompilerBuilderVariableID): void {
    this.checkVariableId(value);
    const ir: NativeCompilerIR.Throw = {
      kind: NativeCompilerIR.Kind.Throw,
      target: this.resolveExceptionTargetInScopeOrThrow(),
      value: value,
    };
    this.ir.push(ir);
  }

  buildSetProperty(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
    value: NativeCompilerBuilderVariableID,
  ): void {
    this.checkVariableId(object);
    this.checkVariableId(value);

    const ir: NativeCompilerIR.SetProperty = {
      kind: NativeCompilerIR.Kind.SetProperty,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      object: object,
      properties: [property],
      values: [value],
    };
    this.ir.push(ir);
  }

  buildSetProperties(
    object: NativeCompilerBuilderVariableID,
    properties: NativeCompilerBuilderAtomID[],
    values: NativeCompilerBuilderVariableID[],
  ): void {
    this.checkVariableId(object);
    values.forEach((v) => this.checkVariableId(v));

    const ir: NativeCompilerIR.SetProperty = {
      kind: NativeCompilerIR.Kind.SetProperty,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      object: object,
      properties: properties,
      values: values,
    };
    this.ir.push(ir);
  }

  buildSetPropertyValue(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
    value: NativeCompilerBuilderVariableID,
  ): void {
    this.checkVariableId(object);
    this.checkVariableId(property);
    this.checkVariableId(value);

    const ir: NativeCompilerIR.SetPropertyValue = {
      kind: NativeCompilerIR.Kind.SetPropertyValue,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      object: object,
      property: property,
      value: value,
    };
    this.ir.push(ir);
  }

  buildSetPropertyIndex(
    object: NativeCompilerBuilderVariableID,
    index: number,
    value: NativeCompilerBuilderVariableID,
  ): void {
    this.checkVariableId(object);
    this.checkVariableId(value);

    const ir: NativeCompilerIR.SetPropertyIndex = {
      kind: NativeCompilerIR.Kind.SetPropertyIndex,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      object: object,
      index,
      value: value,
    };
    this.ir.push(ir);
  }

  buildCopyPropertiesFrom(
    destObject: NativeCompilerBuilderVariableID,
    sourceObject: NativeCompilerBuilderVariableID,
    propertiesToIgnore: NativeCompilerBuilderAtomID[] | undefined,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(destObject);
    this.checkVariableId(sourceObject);

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Number);
    const ir: NativeCompilerIR.CopyPropertiesFrom = {
      kind: NativeCompilerIR.Kind.CopyPropertiesFrom,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      object: destObject,
      value: sourceObject,
      propertiesToIgnore,
      copiedPropertiesCount: variable,
    };
    this.ir.push(ir);
    return variable;
  }

  buildGetPropertyValue(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
    thisObject: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(object);
    this.checkVariableId(property);
    this.checkVariableId(thisObject);

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.GetPropertyValue = {
      kind: NativeCompilerIR.Kind.GetPropertyValue,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      object: object,
      property: property,
      variable: variable,
      thisObject: thisObject,
    };
    this.ir.push(ir);
    return variable;
  }

  buildDeleteProperty(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(object);

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Bool);
    const ir: NativeCompilerIR.DeleteProperty = {
      kind: NativeCompilerIR.Kind.DeleteProperty,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      object: object,
      property: property,
      variable,
    };

    this.ir.push(ir);

    return variable;
  }

  buildDeletePropertyValue(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(object);
    this.checkVariableId(property);

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Bool);
    const ir: NativeCompilerIR.DeletePropertyValue = {
      kind: NativeCompilerIR.Kind.DeletePropertyValue,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      object: object,
      property: property,
      variable,
    };

    this.ir.push(ir);

    return variable;
  }

  buildGetClosureVariableRef(
    variableName: string,
    closureIndex: number,
    source: NativeCompilerBuilderVariableRef,
  ): NativeCompilerBuilderVariableRef {
    const variableRef = this.buildUninitializedVariableRef(
      variableName,
      // leave type as empty for const var ref so that we can add type later
      source.constValue ? NativeCompilerBuilderVariableType.Empty : NativeCompilerBuilderVariableType.Object,
      // inherit the source's const definition
      source.constValue,
    );

    const ir: NativeCompilerIR.GetClosureArg = {
      kind: NativeCompilerIR.Kind.GetClosureArg,
      index: closureIndex,
      variable: variableRef.variableId!,
      constValue: source.constValue,
    };
    this.ir.push(ir);

    return variableRef;
  }

  buildGetModuleConst(index: number): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);

    const ir: NativeCompilerIR.GetModuleConst = {
      kind: NativeCompilerIR.Kind.GetModuleConst,
      index,
      variable,
    };

    this.ir.push(ir);

    return variable;
  }

  buildGetProperty(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
    thisObject: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(object);
    this.checkVariableId(thisObject);

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.GetProperty = {
      kind: NativeCompilerIR.Kind.GetProperty,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      object: object,
      property: property,
      variable: variable,
      thisObject: thisObject,
      propCacheSlot: 0,
    };
    this.ir.push(ir);
    return variable;
  }

  getIntrinsicConstant(name: string): number | string | undefined {
    return intrinsicConstants.get(name);
  }

  hasIntrinsicFunction(func: string): boolean {
    return intrinsicFunctionNames.has(func);
  }

  buildIntrinsicCall(func: string, args: Array<NativeCompilerBuilderVariableID>): NativeCompilerBuilderVariableID {
    for (const arg of args) {
      this.checkVariableId(arg);
    }
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Number);
    const ir: NativeCompilerIR.IntrinsicCall = {
      kind: NativeCompilerIR.Kind.IntrinsicCall,
      func: intrinsicFunctionNames.get(func)!,
      args: args,
      variable: variable,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
    };
    this.ir.push(ir);
    return variable;
  }

  buildFunctionInvocation(
    func: NativeCompilerBuilderVariableID,
    args: Array<NativeCompilerBuilderVariableID>,
    obj: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(func);
    this.checkVariableId(obj);

    for (const arg of args) {
      this.checkVariableId(arg);
    }

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.FunctionInvocation = {
      kind: NativeCompilerIR.Kind.FunctionInvocation,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      func: func,
      args: args,
      obj: obj,
      variable: variable,
      argsType: NativeCompilerIR.FunctionArgumentsType.Direct,
    };
    this.ir.push(ir);
    return variable;
  }

  buildFunctionInvocationWithArgsArray(
    func: NativeCompilerBuilderVariableID,
    args: NativeCompilerBuilderVariableID,
    obj: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(func);
    this.checkVariableId(args);
    this.checkVariableId(obj);

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.FunctionInvocation = {
      kind: NativeCompilerIR.Kind.FunctionInvocation,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      func: func,
      args: [args],
      obj: obj,
      variable: variable,
      argsType: NativeCompilerIR.FunctionArgumentsType.Indirect,
    };
    this.ir.push(ir);
    return variable;
  }

  buildConstructorInvocation(
    func: NativeCompilerBuilderVariableID,
    new_target: NativeCompilerBuilderVariableID,
    args: Array<NativeCompilerBuilderVariableID>,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(func);
    for (const arg of args) {
      this.checkVariableId(arg);
    }

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.ConstructorInvocation = {
      kind: NativeCompilerIR.Kind.ConstructorInvocation,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      func: func,
      new_target: new_target,
      args: args,
      variable: variable,
      argsType: NativeCompilerIR.FunctionArgumentsType.Direct,
    };
    this.ir.push(ir);
    return variable;
  }

  buildConstructorInvocationWithArgsArray(
    func: NativeCompilerBuilderVariableID,
    new_target: NativeCompilerBuilderVariableID,
    args: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(func);
    this.checkVariableId(args);

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.ConstructorInvocation = {
      kind: NativeCompilerIR.Kind.ConstructorInvocation,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      func: func,
      new_target: new_target,
      args: [args],
      variable: variable,
      argsType: NativeCompilerIR.FunctionArgumentsType.Indirect,
    };
    this.ir.push(ir);
    return variable;
  }

  buildGetFunctionArg(index: number): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.GetFunctionArg = {
      kind: NativeCompilerIR.Kind.GetFunctionArg,
      index: index,
      variable: variable,
    };
    this.ir.push(ir);
    return variable;
  }

  buildGetFunctionArgumentsObject(startIndex: number): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.GetFunctionArgumentsObject = {
      kind: NativeCompilerIR.Kind.GetFunctionArgumentsObject,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      startIndex,
      variable: variable,
    };
    this.ir.push(ir);
    return variable;
  }

  buildKeyword(keywordKind: NativeCompilerIR.KeywordKind) {
    let variableType: NativeCompilerBuilderVariableType;
    switch (keywordKind) {
      case NativeCompilerIR.KeywordKind.Undefined:
        variableType = NativeCompilerBuilderVariableType.Undefined;
        break;
      case NativeCompilerIR.KeywordKind.Null:
        variableType = NativeCompilerBuilderVariableType.Null;
        break;
      case NativeCompilerIR.KeywordKind.This:
        variableType = NativeCompilerBuilderVariableType.Object;
        break;
      case NativeCompilerIR.KeywordKind.NewTarget:
        variableType = NativeCompilerBuilderVariableType.Object;
        break;
    }

    const variable = this.context.registerVariable(variableType, false);
    const ir: NativeCompilerIR.Keyword = {
      kind: NativeCompilerIR.Kind.Keyword,
      variable: variable,
      keyword: keywordKind,
    };
    this.ir.push(ir);
    return variable;
  }

  buildGetSuper(): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(
      NativeCompilerBuilderVariableType.Object | NativeCompilerBuilderVariableType.Super,
    );

    const ir: NativeCompilerIR.GetSuper = {
      kind: NativeCompilerIR.Kind.GetSuper,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      variable: variable,
    };
    this.ir.push(ir);
    return variable;
  }

  buildGetSuperConstructor(): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(
      NativeCompilerBuilderVariableType.Object | NativeCompilerBuilderVariableType.Super,
    );

    const ir: NativeCompilerIR.GetSuperConstructor = {
      kind: NativeCompilerIR.Kind.GetSuperConstructor,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      variable: variable,
    };
    this.ir.push(ir);
    return variable;
  }

  buildAssignableUndefined(): NativeCompilerBuilderVariableID {
    return this.context.registerVariable(NativeCompilerBuilderVariableType.Undefined);
  }

  buildUndefined(): NativeCompilerBuilderVariableID {
    if (!this.context.undefinedVariable) {
      this.context.undefinedVariable = this.context.registerVariable(
        NativeCompilerBuilderVariableType.Undefined,
        false,
      );
    }
    return this.context.undefinedVariable;
  }

  buildNull(): NativeCompilerBuilderVariableID {
    if (!this.context.nullVariable) {
      this.context.nullVariable = this.context.registerVariable(NativeCompilerBuilderVariableType.Null, false);
    }
    return this.context.nullVariable;
  }

  buildThis(): NativeCompilerBuilderVariableID {
    return this.buildKeyword(NativeCompilerIR.KeywordKind.This);
  }

  buildNewTarget(): NativeCompilerBuilderVariableID {
    return this.buildKeyword(NativeCompilerIR.KeywordKind.NewTarget);
  }

  buildCopy(value: NativeCompilerBuilderVariableID): NativeCompilerBuilderVariableID {
    this.checkVariableId(value);

    const variable = this.context.registerVariable(value.type);
    this.buildAssignment(variable, value);

    return variable;
  }

  buildException(retVar?: NativeCompilerBuilderVariableID) {
    const variable = retVar ?? this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.Exception = {
      kind: NativeCompilerIR.Kind.Exception,
      variable: variable,
    };
    this.ir.push(ir);
    return variable;
  }

  buildLiteralInteger(value: string): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Number);
    const ir: NativeCompilerIR.LiteralInteger = {
      kind: NativeCompilerIR.Kind.LiteralInteger,
      variable: variable,
      value: value,
    };
    this.ir.push(ir);
    return variable;
  }

  buildLiteralLong(value: string): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Number);
    const ir: NativeCompilerIR.LiteralLong = {
      kind: NativeCompilerIR.Kind.LiteralLong,
      variable: variable,
      value: value,
    };
    this.ir.push(ir);
    return variable;
  }

  buildLiteralDouble(value: string): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Number);
    const ir: NativeCompilerIR.LiteralDouble = {
      kind: NativeCompilerIR.Kind.LiteralDouble,
      variable: variable,
      value: value,
    };
    this.ir.push(ir);
    return variable;
  }

  buildLiteralString(value: string): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.LiteralString = {
      kind: NativeCompilerIR.Kind.LiteralString,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      variable: variable,
      value: value,
    };
    this.ir.push(ir);
    return variable;
  }

  buildLiteralBool(value: boolean): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Bool);
    const ir: NativeCompilerIR.LiteralBool = {
      kind: NativeCompilerIR.Kind.LiteralBool,
      variable: variable,
      value: value,
    };
    this.ir.push(ir);
    return variable;
  }

  buildGetException(): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.GetException = {
      kind: NativeCompilerIR.Kind.GetException,
      variable: variable,
    };
    this.ir.push(ir);
    return variable;
  }

  buildCheckException(jumpTarget: NativeCompilerBuilderJumpTargetID): void {
    const ir: NativeCompilerIR.CheckException = {
      kind: NativeCompilerIR.Kind.CheckException,
      exceptionTarget: jumpTarget,
    };
    this.ir.push(ir);
  }

  buildGlobal(): NativeCompilerBuilderVariableID {
    if (this.globalVariable === undefined) {
      const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
      const ir: NativeCompilerIR.Global = {
        kind: NativeCompilerIR.Kind.Global,
        variable: variable,
      };
      this.ir.push(ir);
      this.globalVariable = variable;
    }
    return this.globalVariable;
  }

  buildBinaryOp(
    operator: NativeCompilerIR.BinaryOperator,
    left: NativeCompilerBuilderVariableID,
    right: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(left);
    this.checkVariableId(right);

    let variableType: NativeCompilerBuilderVariableType;
    switch (operator) {
      case NativeCompilerIR.BinaryOperator.Mult:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.BinaryOperator.Add:
        if (
          left.type & NativeCompilerBuilderVariableType.Object ||
          right.type & NativeCompilerBuilderVariableType.Object
        ) {
          variableType = NativeCompilerBuilderVariableType.Object;
        } else {
          variableType = NativeCompilerBuilderVariableType.Number;
        }
        break;
      case NativeCompilerIR.BinaryOperator.Sub:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.BinaryOperator.Div:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.BinaryOperator.LeftShift:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.BinaryOperator.RightShift:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.BinaryOperator.UnsignedRightShift:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.BinaryOperator.BitwiseXOR:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.BinaryOperator.BitwiseAND:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.BinaryOperator.BitwiseOR:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.BinaryOperator.LessThan:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.BinaryOperator.LessThanOrEqual:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.BinaryOperator.LessThanOrEqualEqual:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.BinaryOperator.GreaterThan:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.BinaryOperator.GreaterThanOrEqual:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.BinaryOperator.GreaterThanOrEqualEqual:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.BinaryOperator.EqualEqual:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.BinaryOperator.EqualEqualEqual:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.BinaryOperator.DifferentThan:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.BinaryOperator.DifferentThanStrict:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.BinaryOperator.Modulo:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.BinaryOperator.Exponentiation:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.BinaryOperator.InstanceOf:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.BinaryOperator.In:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
    }

    const variable = this.context.registerVariable(variableType);
    const ir: NativeCompilerIR.BinaryOp = {
      kind: NativeCompilerIR.Kind.BinaryOp,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      operator,
      variable: variable,
      left: left,
      right: right,
    };
    this.ir.push(ir);
    return variable;
  }

  buildReturn(value: NativeCompilerBuilderVariableID): void {
    this.checkVariableId(value);
    this.buildAssignment(this.context.returnVariable, value);
    this.buildJump(this.context.returnJumpTarget);
  }

  buildNewObject() {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.NewObject = {
      kind: NativeCompilerIR.Kind.NewObject,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      variable: variable,
    };
    this.ir.push(ir);
    return variable;
  }

  buildNewArray(): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.NewArray = {
      kind: NativeCompilerIR.Kind.NewArray,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      variable: variable,
    };
    this.ir.push(ir);
    return variable;
  }

  buildLoop(): INativeCompilerLoopBuilder {
    const builderLoop = this.buildSubBuilder(true);

    const builderLoopInitializer = builderLoop.buildSubBuilder(false);
    const builderLoopMain = builderLoop.buildSubBuilder(true);
    const builderLoopMainCondition = builderLoopMain.buildJumpTarget('cond');
    const builderLoopMainBody = builderLoopMain.buildJumpTarget('body');
    builderLoop.buildJump(builderLoopMainCondition.target);
    const exitBuilder = builderLoop.buildJumpTarget('exit');

    builderLoopMainBody.builder.registerExitTargetInScope(exitBuilder.target);
    builderLoopMainBody.builder.registerContinueTargetInScope(builderLoopMainCondition.target);

    return {
      initBuilder: builderLoopInitializer,
      condBuilder: builderLoopMainCondition.builder,
      bodyJumpTargetBuilder: {
        builder: builderLoopMainBody.builder,
        target: builderLoopMainBody.target,
      },
      exitTarget: exitBuilder.target,
    };
  }

  buildIterator(value: NativeCompilerBuilderVariableID): NativeCompilerBuilderVariableID {
    this.checkVariableId(value);

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Iterator);
    const ir: NativeCompilerIR.Iterator = {
      kind: NativeCompilerIR.Kind.Iterator,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      arg: value,
      variable: variable,
    };
    this.ir.push(ir);

    return variable;
  }

  buildKeysIterator(value: NativeCompilerBuilderVariableID): NativeCompilerBuilderVariableID {
    this.checkVariableId(value);

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Iterator);
    const ir: NativeCompilerIR.KeysIterator = {
      kind: NativeCompilerIR.Kind.KeysIterator,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      arg: value,
      variable: variable,
    };
    this.ir.push(ir);

    return variable;
  }

  buildIteratorNext(
    value: NativeCompilerBuilderVariableID,
    type: NativeCompilerBuilderVariableType,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(value);

    const variable = this.context.registerVariable(type);

    const ir: NativeCompilerIR.IteratorNext = {
      kind: NativeCompilerIR.Kind.IteratorNext,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      iterator: value,
      variable: variable,
    };
    this.ir.push(ir);

    return variable;
  }

  buildKeysIteratorNext(
    value: NativeCompilerBuilderVariableID,
    type: NativeCompilerBuilderVariableType,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(value);

    const variable = this.context.registerVariable(type);

    const ir: NativeCompilerIR.KeysIteratorNext = {
      kind: NativeCompilerIR.Kind.KeysIteratorNext,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      iterator: value,
      variable: variable,
    };
    this.ir.push(ir);

    return variable;
  }

  buildGenerator(closure: NativeCompilerBuilderVariableID): NativeCompilerBuilderVariableID {
    this.checkVariableId(closure);
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.Generator = {
      kind: NativeCompilerIR.Kind.Generator,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      arg: closure,
      variable: variable,
    };
    this.ir.push(ir);
    return variable;
  }

  buildResume(): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.Resume = {
      kind: NativeCompilerIR.Kind.Resume,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      variable: variable,
    };
    this.ir.push(ir);
    return variable;
  }

  buildSetResumePoint(resumePoint?: NativeCompilerBuilderJumpTargetID): void {
    const ir: NativeCompilerIR.SetResumePoint = {
      kind: NativeCompilerIR.Kind.SetResumePoint,
      resumePoint: resumePoint || new NativeCompilerBuilderJumpTargetID(-1, 'end_of_func'),
    };
    this.ir.push(ir);
  }

  buildSuspend(value: NativeCompilerBuilderVariableID, resumePoint: NativeCompilerBuilderJumpTargetID): void {
    this.checkVariableId(value);
    this.buildSetResumePoint(resumePoint);
    this.buildAssignment(this.context.returnVariable, value);
    this.buildJump(this.context.yieldJumpTarget);
  }

  buildTryCatch(hasCatchBlock: boolean, hasFinallyBlock: boolean): INativeCompilerTryCatchBuilder {
    const tryJumpTarget = this.buildJumpTarget('try');
    const catchJumpTarget = hasCatchBlock ? this.buildJumpTarget('catch') : undefined;
    const finallyJumpTarget = this.buildJumpTarget('finally');

    const tryBuilder = tryJumpTarget.builder.buildSubBuilder(true);
    if (catchJumpTarget) {
      // If we have a catch block, we will jump to our catch block within ou try statement
      // when an exception is thrown
      tryBuilder.registerExceptionTargetInScope(catchJumpTarget.target);
    } else if (hasFinallyBlock) {
      // Otherwise, we will jump to the finally block if it's provided
      tryBuilder.registerExceptionTargetInScope(finallyJumpTarget.target);
    }
    // Jump to finally at the end of the try
    tryJumpTarget.builder.buildJump(finallyJumpTarget.target);

    const catchBuilder = catchJumpTarget && catchJumpTarget.builder.buildSubBuilder(true);
    if (catchBuilder) {
      if (hasFinallyBlock) {
        // On nested exceptions thrown within the catch, we jump to the finally block
        catchBuilder.registerExceptionTargetInScope(finallyJumpTarget.target);
      }
      catchJumpTarget.builder.buildJump(finallyJumpTarget.target);
    }

    let finallyBuilder: INativeCompilerBlockBuilder | undefined;
    if (hasFinallyBlock) {
      const finallyBuilderOuter = finallyJumpTarget.builder.buildSubBuilder(true);
      finallyBuilder = finallyBuilderOuter.buildSubBuilder(false);
      const finallyBuilderPostCheckBuilder = finallyBuilderOuter.buildSubBuilder(false);
      // Handle case where exception was not handled, or a nested exception was thrown inside the catch.
      // In this case we want to jump to our outer exception target at the end of the finally block
      finallyBuilderPostCheckBuilder.buildCheckException(this.resolveExceptionTargetInScopeOrThrow());
    }

    return {
      tryBuilder,
      catchBuilder,
      finallyBuilder,
    };
  }

  buildComments(text: string): void {
    const ir: NativeCompilerIR.Comments = {
      kind: NativeCompilerIR.Kind.Comments,
      text,
    };
    this.ir.push(ir);
  }

  buildProgramCounterInfo(lineNumber: number, columnNumber: number): void {
    if (this.lastLineNumber === lineNumber) {
      // Only emit when line number is changing
      return;
    }

    this.lastLineNumber = lineNumber;

    const ir: NativeCompilerIR.ProgramCounterInfo = {
      kind: NativeCompilerIR.Kind.ProgramCounterInfo,
      lineNumber: lineNumber,
      columnNumber: columnNumber,
    };

    this.ir.push(ir);
  }

  buildNewFunctionValue(
    functionName: string,
    name: NativeCompilerBuilderAtomID,
    argc: number,
    closureArguments: NativeCompilerBuilderVariableRef[],
  ): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.NewFunctionValue = {
      kind: NativeCompilerIR.Kind.NewFunctionValue,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      variable: variable,
      functionName: functionName,
      name,
      argc,
      closureArgs: this.resolveClosureArguments(closureArguments),
    };
    this.ir.push(ir);
    return variable;
  }

  buildNewClassValue(
    functionName: string,
    constructorName: NativeCompilerBuilderAtomID,
    argc: number,
    parentClass: NativeCompilerBuilderVariableID,
    closureArguments: NativeCompilerBuilderVariableRef[],
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(parentClass);

    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.NewClassValue = {
      kind: NativeCompilerIR.Kind.NewClassValue,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      variable: variable,
      functionName: functionName,
      argc,
      parentClass,
      name: constructorName,
      closureArgs: this.resolveClosureArguments(closureArguments),
    };
    this.ir.push(ir);
    return variable;
  }

  buildSetClassElement(
    type: NativeCompilerIR.ClassElementType,
    classVariable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderAtomID,
    value: NativeCompilerBuilderVariableID,
    isStatic: boolean,
  ): void {
    this.checkVariableId(classVariable);
    this.checkVariableId(value);

    const ir: NativeCompilerIR.SetClassElement = {
      kind: NativeCompilerIR.Kind.SetClassElement,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      type,
      cls: classVariable,
      name,
      value,
      static: isStatic,
    };
    this.ir.push(ir);
  }

  buildSetClassElementValue(
    type: NativeCompilerIR.ClassElementType,
    classVariable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderVariableID,
    value: NativeCompilerBuilderVariableID,
    isStatic: boolean,
  ): void {
    this.checkVariableId(classVariable);
    this.checkVariableId(name);
    this.checkVariableId(value);

    const ir: NativeCompilerIR.SetClassElementValue = {
      kind: NativeCompilerIR.Kind.SetClassElementValue,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      type,
      cls: classVariable,
      name,
      value,
      static: isStatic,
    };
    this.ir.push(ir);
  }

  private resolveClosureArguments(
    closureArguments: NativeCompilerBuilderVariableRef[],
  ): NativeCompilerBuilderVariableID[] {
    const resolvedClosureArguments: NativeCompilerBuilderVariableID[] = [];
    for (const reference of closureArguments) {
      const variableRef = this.resolveVariableRefOrDelegateInScope(reference.name);
      if (variableRef instanceof NativeCompilerBuilderVariableRef) {
        resolvedClosureArguments.push(variableRef.variableId);
        // Reconcile types from captured variable ref
        variableRef.addType(reference.type);
      } else {
        throw new Error(`Unrecognized local variable ${reference}`);
      }
    }

    return resolvedClosureArguments;
  }

  buildNewArrowFunctionValue(
    functionName: string,
    argc: number,
    closureArguments: NativeCompilerBuilderVariableRef[],
    stackOnHeap: boolean,
  ): NativeCompilerBuilderVariableID {
    const variable = this.context.registerVariable(NativeCompilerBuilderVariableType.Object);
    const ir: NativeCompilerIR.NewArrowFunctionValue = {
      kind: NativeCompilerIR.Kind.NewArrowFunctionValue,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      variable: variable,
      functionName: functionName,
      argc,
      closureArgs: this.resolveClosureArguments(closureArguments),
      stackOnHeap: stackOnHeap,
    };
    this.ir.push(ir);
    return variable;
  }

  buildAssignment(left: NativeCompilerBuilderVariableID, right: NativeCompilerBuilderVariableID): void {
    //TODO(rjaber): Get rid of this on phi instruction brah
    this.checkVariableId(left);
    this.checkVariableId(right);

    if (!left.assignable) {
      throw new Error(`Variable ${left.toString()} is not assignable`);
    }

    left.addType(right.type);

    const variableRef = this.variableContext.getVariableRefForAlias(left);
    if (variableRef) {
      // We are assigning to an alias of a variable ref. Replace to a SetVariableRef instead
      this.buildSetVariableRef(variableRef, right);
    } else {
      const ir: NativeCompilerIR.Assignment = {
        kind: NativeCompilerIR.Kind.Assignment,
        left: left,
        right: right,
      };
      this.ir.push(ir);
    }
  }

  buildUnaryOp(
    operator: NativeCompilerIR.UnaryOperator,
    operand: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableID {
    this.checkVariableId(operand);

    let variableType: NativeCompilerBuilderVariableType;
    switch (operator) {
      case NativeCompilerIR.UnaryOperator.Neg:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.UnaryOperator.Plus:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.UnaryOperator.Inc:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.UnaryOperator.Dec:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.UnaryOperator.BitwiseNot:
        variableType = NativeCompilerBuilderVariableType.Number;
        break;
      case NativeCompilerIR.UnaryOperator.LogicalNot:
        variableType = NativeCompilerBuilderVariableType.Bool;
        break;
      case NativeCompilerIR.UnaryOperator.TypeOf:
        variableType = NativeCompilerBuilderVariableType.Object;
        break;
    }

    const variable = this.context.registerVariable(variableType);
    const ir: NativeCompilerIR.UnaryOp = {
      kind: NativeCompilerIR.Kind.UnaryOp,
      exceptionTarget: this.resolveExceptionTargetInScopeOrThrow(),
      operator,
      variable: variable,
      operand: operand,
    };
    this.ir.push(ir);
    return variable;
  }

  registerExportsVariableRef(variableRef: NativeCompilerBuilderVariableRef): void {
    this.checkVariableRef(variableRef);

    this.scopeContext.registerExportsVariableRef(variableRef);
  }

  resolveExportsVariableRef(): NativeCompilerBuilderVariableRef {
    const variableRef = this.scopeContext.resolveExportsVariableRef();
    if (!variableRef) {
      throw new Error('Could not resolve exports variable ref in current scope');
    }

    return variableRef;
  }

  resolveExportsVariable(): NativeCompilerBuilderVariableID {
    const variableRef = this.resolveExportsVariableRef();
    return this.buildLoadVariableRef(variableRef);
  }

  finalize(options: NativeCompilerOptions) {
    const out: NativeCompilerIR.Base[] = [];
    for (const ir of this.ir) {
      switch (ir.kind) {
        case NativeCompilerIR.Kind.LoadVariableRef: {
          const oldIR = ir as NativeCompilerIR.LoadVariableRef;
          let vref = this.variableContext.getVariableRefForAlias(oldIR.variable);
          if (vref?.constValue) {
            // patch const ref type to its original source type.
            // const variables must be initialized so the source must have it.
            let newIR = { ...oldIR };
            newIR.variable.addType(vref.constValue.type);
            if (newIR.variable.type === NativeCompilerBuilderVariableType.Empty) {
              // variable is const but not assigned to a primitive, assume Object
              newIR.variable.addType(NativeCompilerBuilderVariableType.Object);
            }
            out.push(newIR);
          } else {
            out.push(oldIR);
          }
          break;
        }
        default:
          out.push(ir);
          break;
      }
    }
    return NativeCompilerTransformerResolveBuilderStubs.transform(out, this.subbuilder, options);
  }
}

class NativeCompilerFunctionBuilder implements INativeCompilerBlockBuilderFunctionContextListener {
  private parentBlock: NativeCompilerBlockBuilder;
  private builder: INativeCompilerBlockBuilder;

  readonly context: NativeCompilerBlockBuilderFunctionContext;
  private closureBlock: INativeCompilerBlockBuilder;
  private exceptionJumpTarget: NativeCompilerBuilderJumpTargetID;

  constructor(
    functionId: number,
    public readonly name: string,
    readonly type: NativeCompilerIR.FunctionType,
    readonly parentVariableContext: VariableContext | undefined,
    isGenerator: boolean,
  ) {
    this.context = new NativeCompilerBlockBuilderFunctionContext(functionId, this.name, this, isGenerator);
    const variableContext = new VariableContext(this.context, parentVariableContext);
    this.parentBlock = new NativeCompilerBlockBuilder(
      this.context,
      new NativeCompilerBlockBuilderScopeContext(undefined),
      variableContext,
      undefined,
    );
    this.context.returnVariable = this.context.registerVariable(NativeCompilerBuilderVariableType.ReturnValue);

    const scopedBlock = this.parentBlock.buildSubBuilder(true);
    this.closureBlock = scopedBlock.buildSubBuilder(false);
    const bodyBlock = scopedBlock.buildSubBuilder(false);
    const exceptionBlock = scopedBlock.buildJumpTarget('exception');
    const returnBlock = scopedBlock.buildJumpTarget('return');

    this.builder = bodyBlock.buildFunctionBodySubBuilder(this.closureBlock);
    bodyBlock.buildJump(returnBlock.target);
    const exceptionVariable = exceptionBlock.builder.buildException();
    exceptionBlock.builder.buildAssignment(this.context.returnVariable, exceptionVariable);
    this.exceptionJumpTarget = exceptionBlock.target;
    this.closureBlock.registerExceptionTargetInScope(this.exceptionJumpTarget);
    this.context.returnJumpTarget = returnBlock.target;
    if (isGenerator) {
      // set state to 'done' before final return
      returnBlock.builder.buildSetResumePoint();
      //
      const yieldBlock = scopedBlock.buildJumpTarget('yield');
      this.context.yieldJumpTarget = yieldBlock.target;
    }
  }

  onClosureVariableRequested(
    name: string,
    index: number,
    source: NativeCompilerBuilderVariableRef,
  ): NativeCompilerBuilderVariableRef {
    return this.closureBlock.buildGetClosureVariableRef(name, index, source);
  }

  getBuilder(): INativeCompilerBlockBuilder {
    return this.builder;
  }

  finalizeReentryBlock(irs: NativeCompilerIR.Base[]) {
    const reentry: NativeCompilerIR.Reentry = {
      kind: NativeCompilerIR.Kind.Reentry,
      resumePoints: [],
    };
    // find all resume points in the function body and add to the re-entry IR
    for (const ir of irs) {
      if (ir.kind === NativeCompilerIR.Kind.BindJumpTarget) {
        const jumpTarget = ir as NativeCompilerIR.BindJumpTarget;
        if (jumpTarget.target.tag == 'resume_point') {
          reentry.resumePoints.push(jumpTarget.target);
        }
      }
    }
    irs.unshift(reentry);
  }

  // move the yield jump target to the last (after slot releases)
  finalizeYieldJumpTarget(irs: NativeCompilerIR.Base[]) {
    for (let i = irs.length - 1; i >= 0; --i) {
      if (irs[i].kind === NativeCompilerIR.Kind.BindJumpTarget) {
        const jumpTarget = irs[i] as NativeCompilerIR.BindJumpTarget;
        if (jumpTarget.target.tag == 'yield') {
          irs.splice(i, 1);
          irs.push(jumpTarget);
          return;
        }
      }
    }
  }

  finalize(options: NativeCompilerOptions, moduleBuilder: NativeCompilerModuleBuilder): NativeCompilerIR.Base[] {
    const startFunction: NativeCompilerIR.StartFunction = {
      kind: NativeCompilerIR.Kind.StartFunction,
      name: this.name,
      type: this.type,
      exceptionJumpTarget: this.exceptionJumpTarget,
      returnJumpTarget: this.context.returnJumpTarget,
      returnVariable: this.context.returnVariable,
      localVars: [],
    };
    const endFunction: NativeCompilerIR.EndFunction = {
      kind: NativeCompilerIR.Kind.EndFunction,
      returnVariable: this.context.returnVariable,
    };

    let functionBodyIR = this.parentBlock.finalize(options);

    if (options.foldConstants) {
      let changes = 0;
      do {
        const res = NativeCompilerTransformerConstantFolding.transform(functionBodyIR, moduleBuilder);
        changes = res.changes;
        functionBodyIR = res.irs;
      } while (changes > 0);
    }

    if (options.optimizeVarRefs) {
      functionBodyIR = NativeCompilerTransformerOptimizeVarRefs.transform(functionBodyIR);
    }

    if (options.optimizeVarRefLoads) {
      functionBodyIR = NativeCompilerTransformerOptimizeLoads.transform(functionBodyIR);
    }

    if (options.optimizeAssignments) {
      functionBodyIR = NativeCompilerTransformerOptimizeAssignments.transform(functionBodyIR);
    }

    functionBodyIR = NativeCompilerTransformerResolveSlots.transform(
      startFunction,
      functionBodyIR,
      endFunction,
      options.optimizeSlots,
      this.context.isGenerator,
    );

    functionBodyIR = NativeCompilerTransformerInsertRetainRelease.transform(startFunction, functionBodyIR, endFunction);

    if (options.autoRelease) {
      functionBodyIR = NativeCompilerTransformerAutoRelease.transform(functionBodyIR);
    }

    if (options.mergeReleases) {
      functionBodyIR = NativeCompilerTransformerMergeRelease.transform(functionBodyIR);
    }

    if (this.context.isGenerator) {
      this.finalizeReentryBlock(functionBodyIR);
      this.finalizeYieldJumpTarget(functionBodyIR);
    }

    return [startFunction, ...functionBodyIR, endFunction];
  }
}

class NativeCompilerInitModuleBuilder {
  private atomIdentifierToIDTable = new Map<string, NativeCompilerBuilderAtomID>();
  private stringConstantsTable = new Map<string, number>();
  private allAtoms: NativeCompilerBuilderAtomID[] = [];
  private allStringConstants: string[] = [];
  private atomIDTracker = 0;

  registerAtom(identifierName: string): NativeCompilerBuilderAtomID {
    let atomID = this.atomIdentifierToIDTable.get(identifierName);
    if (atomID === undefined) {
      atomID = new NativeCompilerBuilderAtomID(this.atomIDTracker++, identifierName);
      this.atomIdentifierToIDTable.set(identifierName, atomID);

      this.allAtoms.push(atomID);
    }
    return atomID;
  }

  registerStringConstant(value: string): number {
    let index = this.stringConstantsTable.get(value);
    if (index === undefined) {
      index = this.allStringConstants.length;
      this.stringConstantsTable.set(value, index);
      this.allStringConstants.push(value);
    }

    return index;
  }

  get atomCount() {
    return this.atomIdentifierToIDTable.size;
  }

  get atoms(): NativeCompilerBuilderAtomID[] {
    return this.allAtoms;
  }

  get stringConstants(): readonly string[] {
    return this.allStringConstants;
  }
}

export class NativeCompilerModuleBuilder {
  private moduleInitBuilder: NativeCompilerInitModuleBuilder;
  private functionBuilders = new Array<NativeCompilerFunctionBuilder>();
  private functionIdSequence = 0;
  private propCacheSlots = 0;

  constructor(private moduleName: string, private modulePath: string) {
    this.moduleInitBuilder = new NativeCompilerInitModuleBuilder();
  }

  registerAtom(identifierName: string): NativeCompilerBuilderAtomID {
    return this.moduleInitBuilder.registerAtom(identifierName);
  }

  registerStringConstant(value: string): number {
    return this.moduleInitBuilder.registerStringConstant(value);
  }

  get stringConstants(): readonly string[] {
    return this.moduleInitBuilder.stringConstants;
  }

  buildFunction(
    name: string,
    sourceBuilder: INativeCompilerBlockBuilder | undefined,
    isArrowFunction: boolean,
    isGenerator: boolean,
    isClass: boolean,
  ): INativeCompilerFunctionBuilder {
    const functionBuilder = new NativeCompilerFunctionBuilder(
      ++this.functionIdSequence,
      name,
      isClass ? NativeCompilerIR.FunctionType.ClassConstructor : NativeCompilerIR.FunctionType.ModuleGenericFunction,
      sourceBuilder?.variableContext,
      isGenerator,
    );
    this.functionBuilders.push(functionBuilder);

    if (!isArrowFunction) {
      if (isClass) {
        // 'this' is not a constant in class constructors.
        functionBuilder
          .getBuilder()
          .buildInitializedVariableRef(
            'this',
            functionBuilder.getBuilder().buildUndefined(),
            VariableRefScope.Function,
          );
      } else {
        functionBuilder.getBuilder().registerLazyVariableRef('this', VariableRefScope.Function, (builder) => {
          const thisVariable = builder.buildThis();
          return functionBuilder
            .getBuilder()
            .buildInitializedVariableRef('this', thisVariable, VariableRefScope.Function);
        });
      }
    }

    return {
      name: functionBuilder.name,
      builder: functionBuilder.getBuilder(),
      closureArguments: functionBuilder.context.requestedClosureArguments,
    };
  }

  // go through the IR sequence and allocate a slot number for each get_property call
  private assignPropertyCacheSlots(irs: NativeCompilerIR.Base[]) {
    for (let i = 0; i < irs.length; ++i) {
      const ir = irs[i];
      if (ir.kind === NativeCompilerIR.Kind.GetProperty) {
        const typedIR = ir as NativeCompilerIR.GetProperty;
        const newIR = { ...typedIR, propCacheSlot: this.propCacheSlots++ };
        irs[i] = newIR;
      } else if (ir.kind === NativeCompilerIR.Kind.GetPropertyFree) {
        const typedIR = ir as NativeCompilerIR.GetProperty;
        const newIR = { ...typedIR, propCacheSlot: this.propCacheSlots++ };
        irs[i] = newIR;
      }
    }
  }

  finalize(moduleInitFunctionName: string, options: NativeCompilerOptions): NativeCompilerIR.Base[] {
    let retval: NativeCompilerIR.Base[] = [];
    const prologue: NativeCompilerIR.BoilerplatePrologue = {
      kind: NativeCompilerIR.Kind.BoilerplatePrologue,
      atoms: this.moduleInitBuilder.atoms,
    };
    retval.push(prologue);

    this.functionBuilders.forEach((v) => {
      retval = retval.concat(v.finalize(options, this));
    });

    if (options.inlinePropertyCache) {
      this.assignPropertyCacheSlots(retval);
    }

    const epilogue: NativeCompilerIR.BoilerplateEpilogue = {
      kind: NativeCompilerIR.Kind.BoilerplateEpilogue,
      moduleName: this.moduleName,
      modulePath: this.modulePath,
      moduleInitFunctionName,
      atoms: this.moduleInitBuilder.atoms,
      stringConstants: this.moduleInitBuilder.stringConstants,
      propCacheSlots: this.propCacheSlots,
    };

    retval.push(epilogue);

    return retval;
  }
}
