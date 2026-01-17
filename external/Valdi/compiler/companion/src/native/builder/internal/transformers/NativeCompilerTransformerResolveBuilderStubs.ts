import { NativeCompilerBlockBuilder } from '../../NativeCompilerBuilder';
import { NativeCompilerIR } from '../../NativeCompilerBuilderIR';
import { NativeCompilerOptions } from '../../../NativeCompilerOptions';

export namespace NativeCompilerTransformerResolveBuilderStubs {
  export function transform(
    ir: Array<NativeCompilerIR.Base>,
    subbuilder: Array<NativeCompilerBlockBuilder>,
    options: NativeCompilerOptions,
  ): Array<NativeCompilerIR.Base> {
    let retval = new Array<NativeCompilerIR.Base>();

    ir.forEach((ir_) => {
      switch (ir_.kind) {
        case NativeCompilerIR.Kind.BuilderStub: {
          const ir = ir_ as NativeCompilerIR.BuilderStub;
          const subblock = subbuilder[ir.index];
          retval = retval.concat(subblock.finalize(options));
          break;
        }
        default:
          retval.push(ir_);
          break;
      }
    });
    return retval;
  }
}
