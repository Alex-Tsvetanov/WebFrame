import { NativeCompilerBuilderVariableID } from '../../INativeCompilerBuilder';
import { NativeCompilerIR } from '../../NativeCompilerBuilderIR';
import { JumpIndexer } from './utils/JumpIndexer';
import { isBaseWithReturn } from './utils/IRVisitors';

function insertFreeIfNeeded(
  value: NativeCompilerBuilderVariableID,
  lastIR: NativeCompilerIR.Base | undefined,
  outputIRs: NativeCompilerIR.Base[],
): boolean {
  if (!value.isRetainable()) {
    return false;
  }

  if (lastIR && lastIR.kind === NativeCompilerIR.Kind.Free && (lastIR as NativeCompilerIR.Free).value === value) {
    // We just had a free before, no need to insert a release it
    return false;
  }

  const release: NativeCompilerIR.Free = {
    kind: NativeCompilerIR.Kind.Free,
    value: value,
  };
  outputIRs.push(release);
  return true;
}

export namespace NativeCompilerTransformerInsertRetainRelease {
  export function transform(
    startFunctionIR: NativeCompilerIR.StartFunction,
    irs: NativeCompilerIR.Base[],
    endFunctionIR: NativeCompilerIR.EndFunction,
  ): NativeCompilerIR.Base[] {
    // Keep an array of boolean where each index tells
    // whether the IR is reentrant, meaning that it is
    // possible that the IR gets evaluated multiple times
    // at runtime. We use this to know whether we should insert an additional
    // free before the instruction.
    const reentrantIRIndexes: boolean[] = [];

    // Keep track of assigned variables
    // There is no need to emit a release before the first assignment
    const assignedVariables: number[] = [];

    for (let i = 0; i < irs.length; i++) {
      reentrantIRIndexes.push(false);
    }

    const jumpIndexer = new JumpIndexer(irs);
    jumpIndexer.forEachBackwardJump((jumpIRIndex, irIndex) => {
      for (let i = jumpIRIndex; i < irIndex; i++) {
        reentrantIRIndexes![i] = true;
      }
    });

    let retval: NativeCompilerIR.Base[] = [];
    const releaseIRs: NativeCompilerIR.Free[] = [];

    let lastIR: NativeCompilerIR.Base | undefined;
    let irIndex = 0;
    for (const ir of irs) {
      switch (ir.kind) {
        case NativeCompilerIR.Kind.Slot:
          {
            let typedIR = ir as NativeCompilerIR.Slot;

            if (typedIR.value.isRetainable()) {
              const release: NativeCompilerIR.Free = {
                kind: NativeCompilerIR.Kind.Free,
                value: typedIR.value,
              };
              releaseIRs.push(release);
            }

            retval.push(ir);
          }
          break;
        case NativeCompilerIR.Kind.Assignment:
          {
            let typedIR = ir as NativeCompilerIR.Assignment;

            const assigned = typedIR.left.variable in assignedVariables;
            if (!assigned) {
              // record first assign
              assignedVariables.push(typedIR.left.variable);
            }
            // skip release when safe
            // 1. not assigned
            // 2. not in reentrant region
            if (assigned || reentrantIRIndexes[irIndex]) {
              insertFreeIfNeeded(typedIR.left, lastIR, retval);
            }

            retval.push(typedIR);

            if (typedIR.right.isRetainable()) {
              const retain: NativeCompilerIR.Retain = {
                kind: NativeCompilerIR.Kind.Retain,
                value: typedIR.left,
              };
              retval.push(retain);
            }
          }
          break;
        default: {
          if (isBaseWithReturn(ir) && !(ir.variable.variable in assignedVariables)) {
            // record first assign
            assignedVariables.push(ir.variable.variable);
          }
          if (reentrantIRIndexes[irIndex] && isBaseWithReturn(ir)) {
            // Our IR is re-entrant, so we should insert a free before
            // assigning the variable
            insertFreeIfNeeded(ir.variable, lastIR, retval);
          }

          retval.push(ir);
        }
      }

      irIndex++;
      lastIR = ir;
    }

    // Release in the reverse order in which they were declared
    releaseIRs.reverse();

    return [...retval, ...releaseIRs];
  }
}
