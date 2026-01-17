import { NativeCompilerBuilderVariableID, NativeCompilerBuilderVariableType } from '../../INativeCompilerBuilder';
import { NativeCompilerIR } from '../../NativeCompilerBuilderIR';
import { visitVariables } from './utils/IRVisitors';
import { JumpIndexer } from './utils/JumpIndexer';

interface VariableLifetime {
  variable: NativeCompilerBuilderVariableID;
  firstUseIndex: number;
  lastUseIndex: number;
}

interface VariableLifetimeByID {
  [key: number]: VariableLifetime;
}

class VariablesLifetimeTracker {
  get allVariables(): readonly NativeCompilerBuilderVariableID[] {
    return this._allVariables;
  }

  private variableLifetimeById: VariableLifetimeByID = {};
  private _allVariables: NativeCompilerBuilderVariableID[] = [];

  constructor() {}

  addVariable(variable: NativeCompilerBuilderVariableID, index: number) {
    let lifetime = this.variableLifetimeById[variable.variable];
    if (lifetime === undefined) {
      this.variableLifetimeById[variable.variable] = { variable, firstUseIndex: index, lastUseIndex: index };
      this._allVariables.push(variable);
    } else {
      lifetime.lastUseIndex = Math.max(lifetime.lastUseIndex, index);
    }
  }

  forEachVariableInRange(
    startIndex: number,
    endIndex: number,
    cb: (variable: NativeCompilerBuilderVariableID) => void,
  ) {
    for (const variable of this._allVariables) {
      const lifetime = this.variableLifetimeById[variable.variable]!;
      if (startIndex > lifetime.lastUseIndex || endIndex < lifetime.firstUseIndex) {
        continue;
      }

      cb(variable);
    }
  }

  forEachVariableLifetime(cb: (variableLifetime: VariableLifetime) => void) {
    for (const variable of this._allVariables) {
      const lifetime = this.variableLifetimeById[variable.variable]!;
      cb(lifetime);
    }
  }
}

interface AllocatedVariable {
  variable: NativeCompilerBuilderVariableID;
}

class VariableAllocator {
  get allVariables(): readonly AllocatedVariable[] {
    return this._allVariables;
  }

  private _allVariables: AllocatedVariable[] = [];
  private _freeVariablesByType: { [type: number]: AllocatedVariable[] } = {};
  private _allocatedVariableById: { [id: number]: AllocatedVariable | undefined } = {};
  private _variableCounter = 0;

  getAllocatedVariable(variable: NativeCompilerBuilderVariableID): AllocatedVariable | undefined {
    return this._allocatedVariableById[variable.variable];
  }

  tryReuseAllocatedVariable(variable: NativeCompilerBuilderVariableID): AllocatedVariable | undefined {
    let availableVariables = this._freeVariablesByType[variable.type];
    if (!availableVariables || !availableVariables.length) {
      return undefined;
    }

    const reusedVariable = availableVariables[0];
    availableVariables.splice(0, 1);

    this._allocatedVariableById[variable.variable] = reusedVariable;

    return reusedVariable;
  }

  allocateNewVariable(variable: NativeCompilerBuilderVariableID): AllocatedVariable {
    const newVariable: AllocatedVariable = {
      variable: new NativeCompilerBuilderVariableID(
        variable.functionId,
        ++this._variableCounter,
        variable.type,
        variable.assignable,
      ),
    };

    this._allVariables.push(newVariable);
    this._allocatedVariableById[variable.variable] = newVariable;
    return newVariable;
  }

  freeAllocatedVariable(variable: NativeCompilerBuilderVariableID) {
    const allocatedVariable = this._allocatedVariableById[variable.variable];
    if (!allocatedVariable) {
      throw new Error(`Variable ${variable.toString()} was not allocated`);
    }

    let availableVariables = this._freeVariablesByType[allocatedVariable.variable.type];
    if (!availableVariables) {
      availableVariables = [];
      this._freeVariablesByType[allocatedVariable.variable.type] = availableVariables;
    }
    availableVariables.push(allocatedVariable);
    this._allocatedVariableById[variable.variable] = undefined;
  }
}

export namespace NativeCompilerTransformerResolveSlots {
  export function transform(
    startFunctionIR: NativeCompilerIR.StartFunction,
    irs: NativeCompilerIR.Base[],
    endFunctionIR: NativeCompilerIR.EndFunction,
    optimize: boolean,
    stackOnHeap: boolean,
  ): NativeCompilerIR.Base[] {
    const lifetimeTracker = new VariablesLifetimeTracker();

    // Track all of the variables
    irs.forEach((ir, rIndex) => {
      visitVariables(ir, false, (variable) => {
        lifetimeTracker.addVariable(variable, rIndex);
        return variable;
      });
    });
    lifetimeTracker.addVariable(endFunctionIR.returnVariable, irs.length);

    let allVariables: readonly NativeCompilerBuilderVariableID[];
    let bodyIRs: NativeCompilerIR.Base[];

    if (optimize) {
      // Index all of the jump targets
      const jumpIndexer = new JumpIndexer(irs);

      // Extend lifecycles of variables when processing a jump
      jumpIndexer.forEachBackwardJump((jumpIRIndex, irIndex) => {
        // We need to extend all the variables used between the jump target
        // until this IR.
        lifetimeTracker.forEachVariableInRange(jumpIRIndex, irIndex, (variable) => {
          lifetimeTracker.addVariable(variable, irIndex);
        });
      });

      // Mark the list of variables that are about to stop being used for each IR index.
      const endingVariablesByIRIndex: { [key: number]: NativeCompilerBuilderVariableID[] } = {};
      lifetimeTracker.forEachVariableLifetime((variableLifetime) => {
        let variables = endingVariablesByIRIndex[variableLifetime.lastUseIndex];
        if (!variables) {
          variables = [];
          endingVariablesByIRIndex[variableLifetime.lastUseIndex] = variables;
        }
        variables.push(variableLifetime.variable);
      });

      // Now re-allocates variables
      const variableAllocator = new VariableAllocator();

      bodyIRs = [];

      variableAllocator.allocateNewVariable(endFunctionIR.returnVariable);

      irs.forEach((ir, rIndex) => {
        const output = visitVariables(ir, true, (variable) => {
          const allocatedVariable = variableAllocator.getAllocatedVariable(variable);
          if (allocatedVariable) {
            // Already have an allocated variable, use it
            return allocatedVariable.variable;
          }

          const reusedVariable = variableAllocator.tryReuseAllocatedVariable(variable);
          if (reusedVariable) {
            // We managed to re-use a variable that was marked as unused
            // If the variable held an object, we need to release it first
            // before using it
            if (reusedVariable.variable.isRetainable()) {
              const free: NativeCompilerIR.Free = {
                kind: NativeCompilerIR.Kind.Free,
                value: reusedVariable.variable,
              };
              bodyIRs.push(free);
            }
            return reusedVariable.variable;
          }

          // No variable available, allocate a new one
          return variableAllocator.allocateNewVariable(variable).variable;
        });

        bodyIRs.push(output);

        // Release variables at this IR index
        const endingVariables = endingVariablesByIRIndex[rIndex];
        if (endingVariables) {
          for (const endingVariable of endingVariables) {
            variableAllocator.freeAllocatedVariable(endingVariable);
          }
        }
      });
      allVariables = variableAllocator.allVariables.map((allocatedVariable) => allocatedVariable.variable);
    } else {
      bodyIRs = irs;
      allVariables = lifetimeTracker.allVariables;
    }

    const slots: NativeCompilerIR.Slot[] = [];

    for (const variable of allVariables) {
      slots.push({
        kind: NativeCompilerIR.Kind.Slot,
        value: variable,
      });
      if (stackOnHeap) {
        startFunctionIR.localVars.push(variable);
      }
    }

    return [...slots, ...bodyIRs];
  }
}
