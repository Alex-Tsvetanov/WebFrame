import { NativeCompilerIR } from '../../NativeCompilerBuilderIR';
import { visitVariables, isBaseWithReturn } from './utils/IRVisitors';

/**
 * This is a peephole optimization that looks for the pattern:
 *   X <= foo()
 *   Y <= X
 * and then X is never used again in the function body. This is often caused by
 * optimizing VariableRefs into Variables.
 *
 * In this case we can replace the sequence with just
 *  Y <= foo()
 * This often saves a retain/release pair too.
 */
export namespace NativeCompilerTransformerOptimizeAssignments {
  export function transform(irs: NativeCompilerIR.Base[]): NativeCompilerIR.Base[] {
    const functionBodyIR = [...irs];
    let window: NativeCompilerIR.Base[] = [];
    const candidates = new Map<number, number>();
    functionBodyIR.forEach((ir, irIndex) => {
      visitVariables(ir, false, (v) => {
        if (candidates.has(v.variable)) {
          candidates.delete(v.variable);
        }
        return v;
      });
      window.push(ir);
      if (window.length > 2) {
        window.shift();
      }
      if (window.length == 2) {
        if (!isBaseWithReturn(window[0])) {
          return;
        }
        const loadTarget = window[0].variable;
        if (window[1].kind != NativeCompilerIR.Kind.Assignment) {
          return;
        }
        const assignment = window[1] as NativeCompilerIR.Assignment;
        if (loadTarget != assignment.right) {
          return;
        }
        if (assignment.left.isRetainable()) {
          // don't optimize assignment to retainables. after optimization assignments are turned into BaseWithReturn and
          // the insert-retain-release pass does not insert a release for BaseWithReturn IRs unless in reentrant code.
          return;
        }
        candidates.set(loadTarget.variable, irIndex - 1);
      }
    });
    candidates.forEach((i, v) => {
      const ir = functionBodyIR[i] as NativeCompilerIR.BaseWithReturn;
      const newIR1 = visitVariables(ir, true, (irv) => {
        if (irv.variable == v) {
          const assignmentOnNextLine = functionBodyIR[i + 1] as NativeCompilerIR.Assignment;
          return assignmentOnNextLine.left;
        }
        return irv;
      });
      const newIR2: NativeCompilerIR.Comments = {
        kind: NativeCompilerIR.Kind.Comments,
        text: 'eliminated assignment',
      };
      functionBodyIR[i] = newIR1;
      functionBodyIR[i + 1] = newIR2;
    });
    return functionBodyIR;
  }
}
