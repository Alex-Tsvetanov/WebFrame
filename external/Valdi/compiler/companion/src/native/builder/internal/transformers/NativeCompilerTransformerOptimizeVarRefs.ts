import { VariableIdMap } from '../../../utils/VariableIdMap';
import { NativeCompilerBuilderVariableID, NativeCompilerBuilderVariableType } from '../../INativeCompilerBuilder';
import { NativeCompilerIR } from '../../NativeCompilerBuilderIR';
import { visitVariables } from './utils/IRVisitors';

/**
 * Transformer that eliminates variable references for variables that end up not being
 * captured in closures.
 */
export namespace NativeCompilerTransformerOptimizeVarRefs {
  export function transform(irs: NativeCompilerIR.Base[]): NativeCompilerIR.Base[] {
    // map of variable ref id to variable id replacement.
    const variableRefsToEliminate = new VariableIdMap<NativeCompilerBuilderVariableID>();
    // map of getfnarg result to var ref. Used to eliminate unnecessary assigns for function parameters
    const fnArgToVRef = new VariableIdMap<NativeCompilerBuilderVariableID | undefined>();

    // getfnarg 0 @3
    // newvref 'require' @4
    // setvref @3 @4
    // getfnarg 1 @5
    // newvref 'module' @6
    // setvref @5 @6

    // Step 1: We go over all the variable refs and see which ones are not used in closures
    for (const ir of irs) {
      switch (ir.kind) {
        case NativeCompilerIR.Kind.NewVariableRef: {
          const typedIR = ir as NativeCompilerIR.NewVariableRef;
          variableRefsToEliminate.set(
            typedIR.variable,
            new NativeCompilerBuilderVariableID(
              typedIR.variable.functionId,
              typedIR.variable.variable,
              NativeCompilerBuilderVariableType.Empty,
              true,
            ),
          );
          break;
        }
        case NativeCompilerIR.Kind.GetFunctionArg: {
          const typedIR = ir as NativeCompilerIR.GetFunctionArg;
          fnArgToVRef.set(typedIR.variable, undefined);
          break;
        }
        case NativeCompilerIR.Kind.SetVariableRef: {
          const typedIR = ir as NativeCompilerIR.SetVariableRef;

          if (variableRefsToEliminate.has(typedIR.target)) {
            const variableReplacement = variableRefsToEliminate.get(typedIR.target)!;
            if (fnArgToVRef.has(typedIR.value)) {
              variableRefsToEliminate.set(typedIR.target, typedIR.value);
            } else {
              variableReplacement.addType(typedIR.value.type);
            }
          }

          break;
        }
        case NativeCompilerIR.Kind.NewArrowFunctionValue:
        case NativeCompilerIR.Kind.NewFunctionValue:
        case NativeCompilerIR.Kind.NewClassValue: {
          const typedIR = ir as NativeCompilerIR.NewFunctionValueBase;
          for (const closureArg of typedIR.closureArgs) {
            variableRefsToEliminate.delete(closureArg);
          }
          break;
        }
        default:
          break;
      }
    }

    if (variableRefsToEliminate.size === 0) {
      // nothing to do
      return irs;
    }

    const out: NativeCompilerIR.Base[] = [];

    // Step 2: We eliminate the NewVariableRef, LoadVariableRef, SetVariableRef

    const variableIdsToReplace = new VariableIdMap<NativeCompilerBuilderVariableID>();

    for (const ir of irs) {
      switch (ir.kind) {
        case NativeCompilerIR.Kind.NewVariableRef: {
          const typedIR = ir as NativeCompilerIR.NewVariableRef;
          if (variableRefsToEliminate.has(typedIR.variable)) {
            const variableIdReplacement =
              variableRefsToEliminate.get(typedIR.variable) ??
              new NativeCompilerBuilderVariableID(
                typedIR.variable.functionId,
                typedIR.variable.variable,
                NativeCompilerBuilderVariableType.Empty,
                true,
              );
            variableIdsToReplace.set(typedIR.variable, variableIdReplacement);
            continue;
          }
          break;
        }
        case NativeCompilerIR.Kind.LoadVariableRef: {
          const typedIR = ir as NativeCompilerIR.LoadVariableRef;
          if (variableRefsToEliminate.has(typedIR.target)) {
            const variableIdReplacement = variableIdsToReplace.get(typedIR.target)!;

            // Also make sure that the output of this LoadVariableRef maps to our replacement
            variableIdReplacement.addType(typedIR.variable.type);
            variableIdsToReplace.set(typedIR.variable, variableIdReplacement);
            continue;
          }
          break;
        }
        case NativeCompilerIR.Kind.SetVariableRef: {
          const typedIR = ir as NativeCompilerIR.SetVariableRef;

          if (variableRefsToEliminate.has(typedIR.target)) {
            // Replace to assignment

            const assignmentLeft = variableIdsToReplace.get(typedIR.target);
            if (!assignmentLeft) {
              throw new Error(
                `Invalid state: ${typedIR.target.toString()} was not found in map: ${variableIdsToReplace.toString()}`,
              );
            }
            const assignmentRight = variableIdsToReplace.get(typedIR.value) ?? typedIR.value;

            if (assignmentLeft.variable !== typedIR.value.variable) {
              const assignmentIR: NativeCompilerIR.Assignment = {
                kind: NativeCompilerIR.Kind.Assignment,
                left: assignmentLeft,
                right: assignmentRight,
              };

              out.push(assignmentIR);
            }

            continue;
          }
          break;
        }
        default:
          break;
      }

      const newIR = visitVariables(ir, true, (variable) => {
        return variableIdsToReplace.get(variable) ?? variable;
      });

      out.push(newIR);
    }

    return out;
  }
}
