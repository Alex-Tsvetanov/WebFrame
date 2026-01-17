import {
  INativeCompilerBlockBuilder,
  INativeCompilerBlockBuilderFunctionContext,
  NativeCompilerBuilderJumpTargetID,
  NativeCompilerBuilderVariableID,
  NativeCompilerBuilderVariableRef,
  NativeCompilerBuilderVariableType,
} from '../INativeCompilerBuilder';

export interface INativeCompilerBlockBuilderFunctionContextListener {
  onClosureVariableRequested(
    name: string,
    index: number,
    source: NativeCompilerBuilderVariableRef,
  ): NativeCompilerBuilderVariableRef;
}

export class NativeCompilerBlockBuilderFunctionContext implements INativeCompilerBlockBuilderFunctionContext {
  undefinedVariable: NativeCompilerBuilderVariableID | undefined;
  nullVariable: NativeCompilerBuilderVariableID | undefined;
  requestedClosureArguments: NativeCompilerBuilderVariableRef[] = [];

  private variableIDTracker = 0;
  private variables: NativeCompilerBuilderVariableID[] = [];
  private jumpTargetCounter = 0;
  private _returnJumpTarget: NativeCompilerBuilderJumpTargetID | undefined;
  private _returnVariable: NativeCompilerBuilderVariableID | undefined;
  private _yieldJumpTarget: NativeCompilerBuilderJumpTargetID | undefined;

  constructor(
    readonly functionId: number,
    private readonly name: string,
    private readonly listener: INativeCompilerBlockBuilderFunctionContextListener,
    readonly isGenerator: boolean,
  ) {}

  get returnVariable(): NativeCompilerBuilderVariableID {
    if (this._returnVariable === undefined) {
      throw Error('Return Variable was not defined');
    } else {
      return this._returnVariable;
    }
  }

  set returnVariable(target: NativeCompilerBuilderVariableID) {
    if (this._returnVariable === undefined) {
      this._returnVariable = target;
    } else {
      throw Error('Return Variable was defined');
    }
  }

  get returnJumpTarget(): NativeCompilerBuilderJumpTargetID {
    if (this._returnJumpTarget === undefined) {
      throw Error('Return Jump Target was not defined');
    } else {
      return this._returnJumpTarget;
    }
  }

  set returnJumpTarget(target: NativeCompilerBuilderJumpTargetID) {
    if (this._returnJumpTarget === undefined) {
      this._returnJumpTarget = target;
    } else {
      throw Error('Return Jump Target was defined');
    }
  }

  get yieldJumpTarget(): NativeCompilerBuilderJumpTargetID {
    if (this._yieldJumpTarget === undefined) {
      throw Error(`Yield Jump Target was not defined in function ${this.name}`);
    } else {
      return this._yieldJumpTarget;
    }
  }

  set yieldJumpTarget(target: NativeCompilerBuilderJumpTargetID) {
    if (this._yieldJumpTarget === undefined) {
      this._yieldJumpTarget = target;
    } else {
      throw Error('Yield Jump Target was defined');
    }
  }
  get variableCount() {
    return this.variableIDTracker;
  }

  requestClosureArgument(
    variableName: string,
    source: NativeCompilerBuilderVariableRef,
  ): NativeCompilerBuilderVariableRef {
    const index = this.requestedClosureArguments.length;
    const variableRef = this.listener.onClosureVariableRequested(variableName, index, source);
    this.requestedClosureArguments.push(variableRef);

    return variableRef;
  }

  registerVariable(
    type: NativeCompilerBuilderVariableType,
    assignable: boolean = true,
  ): NativeCompilerBuilderVariableID {
    const variable = new NativeCompilerBuilderVariableID(this.functionId, ++this.variableIDTracker, type, assignable);
    this.variables.push(variable);

    return variable;
  }

  registerJumpTarget(tag: string): NativeCompilerBuilderJumpTargetID {
    return new NativeCompilerBuilderJumpTargetID(this.jumpTargetCounter++, tag);
  }
}

export class NativeCompilerBlockBuilderScopeContext {
  private exitTarget?: NativeCompilerBuilderJumpTargetID;
  private continueTarget?: NativeCompilerBuilderJumpTargetID;
  private exceptionTarget?: NativeCompilerBuilderJumpTargetID;
  private exportsVariableRef?: NativeCompilerBuilderVariableRef;

  constructor(private readonly parentcontext: NativeCompilerBlockBuilderScopeContext | undefined) {}

  registerExitTargetInScope(exitTarget: NativeCompilerBuilderJumpTargetID) {
    this.exitTarget = exitTarget;
  }

  resolveExitTargetInScope(): NativeCompilerBuilderJumpTargetID | undefined {
    if (this.exitTarget !== undefined) {
      return this.exitTarget;
    }
    return this.parentcontext?.resolveExitTargetInScope();
  }

  registerContinueTargetInScope(continueTarget: NativeCompilerBuilderJumpTargetID) {
    if (this.continueTarget) {
      throw new Error('Already has a continue jump target in current scope');
    }
    this.continueTarget = continueTarget;
  }

  resolveContinueTargetInScope(): NativeCompilerBuilderJumpTargetID | undefined {
    if (this.continueTarget !== undefined) {
      return this.continueTarget;
    }
    return this.parentcontext?.resolveContinueTargetInScope();
  }

  registerExceptionTargetInScope(exitTarget: NativeCompilerBuilderJumpTargetID) {
    this.exceptionTarget = exitTarget;
  }

  resolveExceptionTargetInScope(): NativeCompilerBuilderJumpTargetID | undefined {
    if (this.exceptionTarget !== undefined) {
      return this.exceptionTarget;
    }

    return this.parentcontext?.resolveExceptionTargetInScope();
  }

  resolveExportsVariableRef(): NativeCompilerBuilderVariableRef | undefined {
    if (this.exportsVariableRef) {
      return this.exportsVariableRef;
    }

    return this.parentcontext?.resolveExportsVariableRef();
  }

  registerExportsVariableRef(exportsVariableRef: NativeCompilerBuilderVariableRef): void {
    if (this.exportsVariableRef) {
      throw new Error(`Exports variable already exists in scope`);
    }

    this.exportsVariableRef = exportsVariableRef;
  }

  createSubScopeContext(): NativeCompilerBlockBuilderScopeContext {
    return new NativeCompilerBlockBuilderScopeContext(this);
  }
}
