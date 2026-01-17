import { VariableIdMap } from '../utils/VariableIdMap';
import {
  INativeCompilerBlockBuilderFunctionContext,
  IVariableDelegate,
  NativeCompilerBuilderVariableID,
  NativeCompilerBuilderVariableRef,
} from './INativeCompilerBuilder';

export type LazyNativeCompilerBuilderVariableRef = () => NativeCompilerBuilderVariableRef;

export type VariableResolveResult = NativeCompilerBuilderVariableRef | IVariableDelegate;

export function variableResultIsVariableRef(result: VariableResolveResult): result is NativeCompilerBuilderVariableRef {
  return result instanceof NativeCompilerBuilderVariableRef;
}

export function variableResultIsVariableDelegate(result: VariableResolveResult): result is IVariableDelegate {
  return (result as IVariableDelegate).onGetValue !== undefined;
}

export const enum VariableResolvingMode {
  /**
   * Look up all the way to the root VariableContext to resolve the variable,
   * potentially emitting a closure argument along the way.
   */
  full,

  /**
   * Look up up to the current function scope, without emitting closure or
   * triggering lazy variable creation.
   */
  shallow,
}

export class VariableContext {
  private scopeVariable = new Map<
    string,
    NativeCompilerBuilderVariableRef | LazyNativeCompilerBuilderVariableRef | IVariableDelegate
  >();
  private variableRefByAliasVariableId = new VariableIdMap<NativeCompilerBuilderVariableRef>();

  constructor(
    readonly functionContext: INativeCompilerBlockBuilderFunctionContext,
    readonly parent: VariableContext | undefined,
  ) {}

  /**
   * Creates a new VariableContext that shares the scope of this variable context,
   * but will emit variables that are scoped to the returned variable context.
   */
  scoped(): VariableContext {
    return new VariableContext(this.functionContext, this);
  }

  /**
   * Creates a new VariableContext on a new function context, that can resolve variables as closures
   * from this variable context.
   */
  detached(functionContext: INativeCompilerBlockBuilderFunctionContext): VariableContext {
    return new VariableContext(functionContext, this);
  }

  registerVariableRefInScope(variableRef: NativeCompilerBuilderVariableRef): void {
    const existingVariableRef = this.scopeVariable.get(variableRef.name);
    if (existingVariableRef && existingVariableRef instanceof NativeCompilerBuilderVariableRef) {
      throw new Error(`Variable ${variableRef.name} identifier already exists in scope`);
    }
    this.scopeVariable.set(variableRef.name, variableRef);
  }

  registerVariableDelegate(name: string, delegate: IVariableDelegate): void {
    if (this.scopeVariable.has(name)) {
      throw new Error(`Variable ${name} identifier already exists in scope`);
    }
    this.scopeVariable.set(name, delegate);
  }

  registerLazyVariableRef(name: string, lazy: LazyNativeCompilerBuilderVariableRef): void {
    if (this.scopeVariable.has(name)) {
      throw new Error(`Variable ${name} identifier already exists in scope`);
    }
    this.scopeVariable.set(name, lazy);
  }

  resolveVariable(variableName: string, mode: VariableResolvingMode): VariableResolveResult | undefined {
    let variable = this.scopeVariable.get(variableName);
    if (variable !== undefined) {
      if (variable instanceof NativeCompilerBuilderVariableRef || typeof variable === 'object') {
        return variable;
      } else {
        if (mode === VariableResolvingMode.shallow) {
          return undefined;
        }

        const resolvedVariable = variable();
        if (this.scopeVariable.get(variableName) !== resolvedVariable) {
          throw new Error(`Variable ${variableName} should have been registered after being instantiated`);
        }
        return resolvedVariable;
      }
    }

    const parent = this.parent;
    if (!parent) {
      return undefined;
    }

    if (parent.functionContext === this.functionContext) {
      return parent.resolveVariable(variableName, mode);
    } else {
      if (mode !== VariableResolvingMode.full) {
        return undefined;
      }

      const parentResult = parent.resolveVariable(variableName, mode);

      // Our parent is from a different function context. We turn the return value as a closure here
      // if the returned value is not a variable delegate

      if (!parentResult) {
        // Was not found, this should be treated as a global variable
        return undefined;
      }

      if (variableResultIsVariableRef(parentResult)) {
        const resolvedVariable = this.functionContext.requestClosureArgument(variableName, parentResult);
        if (!this.scopeVariable.has(variableName)) {
          this.scopeVariable.set(variableName, resolvedVariable);
        }

        return resolvedVariable;
      }

      // If we are here, the returned result is a variable delegate

      return parentResult;
    }
  }

  registerVariableRefAlias(
    variableRef: NativeCompilerBuilderVariableRef,
    aliasVariableId: NativeCompilerBuilderVariableID,
  ): void {
    this.variableRefByAliasVariableId.set(aliasVariableId, variableRef);
  }

  getVariableRefForAlias(
    aliasVariableId: NativeCompilerBuilderVariableID,
  ): NativeCompilerBuilderVariableRef | undefined {
    return this.variableRefByAliasVariableId.get(aliasVariableId);
  }
}
