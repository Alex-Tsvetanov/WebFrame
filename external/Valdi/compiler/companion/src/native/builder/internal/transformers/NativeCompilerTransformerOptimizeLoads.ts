import { NativeCompilerBuilderVariableID } from '../../INativeCompilerBuilder';
import { NativeCompilerIR } from '../../NativeCompilerBuilderIR';
import { visitVariables } from './utils/IRVisitors';

export namespace NativeCompilerTransformerOptimizeLoads {
  export function transform(irs: NativeCompilerIR.Base[]): NativeCompilerIR.Base[] {
    // Keep track of effective aliases of VarRefs
    // VarRef => Var
    const varRefsToAliases = new Map<Number, NativeCompilerBuilderVariableID>();
    // Var => VarRef
    const aliasesToVarRefs = new Map<Number, NativeCompilerBuilderVariableID>();
    // Remember const variables
    // No need to invalidate these after calling functions
    const constVarRefs = new Set<Number>();

    const output: NativeCompilerIR.Base[] = [];

    const variableIdsToReplace = new Map<Number, NativeCompilerBuilderVariableID>();

    for (const ir of irs) {
      const newIR = visitVariables(ir, true, (variable) => {
        return variableIdsToReplace.get(variable.variable) ?? variable;
      });
      switch (newIR.kind) {
        case NativeCompilerIR.Kind.GetClosureArg: {
          const typedIR = newIR as NativeCompilerIR.GetClosureArg;
          if (typedIR.constValue) {
            constVarRefs.add(typedIR.variable.variable);
          }
          break;
        }
        case NativeCompilerIR.Kind.NewVariableRef: {
          const typedIR = newIR as NativeCompilerIR.NewVariableRef;
          if (typedIR.constValue) {
            constVarRefs.add(typedIR.variable.variable);
          }
          break;
        }
        case NativeCompilerIR.Kind.LoadVariableRef: {
          const typedIR = newIR as NativeCompilerIR.LoadVariableRef;
          if (!varRefsToAliases.has(typedIR.target.variable)) {
            // A new loadvref establishes an alias
            varRefsToAliases.set(typedIR.target.variable, typedIR.variable);
            aliasesToVarRefs.set(typedIR.variable.variable, typedIR.target);
          } else {
            // Alias exists, skip the loadref
            // And record a replacement
            variableIdsToReplace.set(typedIR.variable.variable, varRefsToAliases.get(typedIR.target.variable)!);
            continue;
          }
          break;
        }
        case NativeCompilerIR.Kind.SetVariableRef: {
          const typedIR = newIR as NativeCompilerIR.SetVariableRef;
          if (varRefsToAliases.has(typedIR.target.variable)) {
            if (constVarRefs.has(typedIR.target.variable)) {
              throw new Error(`Assigning to const VarRef ${typedIR.target.variable}`);
            }
            // Assigning to an already aliased VarRef refreshes the alias
            const existingAlias = varRefsToAliases.get(typedIR.target.variable)!;
            // check if we are assigning alias back, skip if this is the case
            if (existingAlias.variable !== typedIR.value.variable) {
              aliasesToVarRefs.delete(existingAlias.variable);
              varRefsToAliases.set(typedIR.target.variable, typedIR.value);
              aliasesToVarRefs.set(typedIR.value.variable, typedIR.target);
            } else {
              continue;
            }
          } else {
            // establish new alias if assigning to unaliased varref
            varRefsToAliases.set(typedIR.target.variable, typedIR.value);
            aliasesToVarRefs.set(typedIR.value.variable, typedIR.target);
          }
          break;
        }
        case NativeCompilerIR.Kind.Assignment: {
          // Assigning to an alias invalidates it
          const typedIR = newIR as NativeCompilerIR.Assignment;
          if (aliasesToVarRefs.has(typedIR.left.variable)) {
            const targetVarRef = aliasesToVarRefs.get(typedIR.left.variable)!;
            varRefsToAliases.delete(targetVarRef.variable);
            aliasesToVarRefs.delete(typedIR.left.variable);
          }
          break;
        }
        case NativeCompilerIR.Kind.BindJumpTarget: {
          // hitting a jumptarget invalidates all aliases
          varRefsToAliases.clear();
          aliasesToVarRefs.clear();
          break;
        }
        case NativeCompilerIR.Kind.GetProperty:
        case NativeCompilerIR.Kind.GetPropertyValue:
        case NativeCompilerIR.Kind.ConstructorInvocation:
        case NativeCompilerIR.Kind.FunctionInvocation: {
          // Calling a function invalidates all non-const aliases
          const varRefsToInvalidate = [...varRefsToAliases.keys()].filter((x) => !constVarRefs.has(x));
          for (const v of varRefsToInvalidate) {
            const alias = varRefsToAliases.get(v)!.variable;
            aliasesToVarRefs.delete(alias);
            varRefsToAliases.delete(v);
          }
          break;
        }
        default:
          break;
      }
      output.push(newIR);
    }
    return output;
  }
}
