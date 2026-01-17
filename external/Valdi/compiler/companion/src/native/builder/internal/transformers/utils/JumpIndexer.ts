import { NativeCompilerIR } from '../../../NativeCompilerBuilderIR';
import { visitJump } from './IRVisitors';

export class JumpIndexer {
  private irIndexByJumpTarget: { [key: number]: number } = {};

  constructor(readonly irs: NativeCompilerIR.Base[]) {
    irs.forEach((ir, rIndex) => {
      if (ir.kind === NativeCompilerIR.Kind.BindJumpTarget) {
        const jumpTarget = ir as NativeCompilerIR.BindJumpTarget;
        this.irIndexByJumpTarget[jumpTarget.target.label] = rIndex;
      }
    });
  }

  /**
   * Call the given callback for each jump to a jump target to an instruction
   * that is before the jump instruction.
   * @param cb
   */
  forEachBackwardJump(cb: (jumpIRIndex: number, irIndex: number) => void) {
    this.irs.forEach((ir, irIndex) => {
      visitJump(ir, (jump) => {
        const jumpIRIndex = this.irIndexByJumpTarget[jump.label]!;
        if (jumpIRIndex > irIndex) {
          // Forward Jump
          return;
        }

        cb(jumpIRIndex, irIndex);
      });
    });
  }
}
