import { NativeCompilerIR } from '../../NativeCompilerBuilderIR';

/**
 * Replace the sequence
 *   free @X
 *   @x = get_prop(...)
 * with
 *   get_prop_free(@X, ...)
 */
export namespace NativeCompilerTransformerAutoRelease {
  export function transform(irs: NativeCompilerIR.Base[]): NativeCompilerIR.Base[] {
    const output: NativeCompilerIR.Base[] = [];

    for (const ir of irs) {
      switch (ir.kind) {
        case NativeCompilerIR.Kind.GetProperty: {
          const typedIR = ir as NativeCompilerIR.GetProperty;
          const lastIR = output.pop();
          if (lastIR?.kind === NativeCompilerIR.Kind.Free) {
            const lastFree = lastIR as NativeCompilerIR.Free;
            if (lastFree.value.variable == typedIR.variable.variable) {
              // last IR is freeing the target of this IR
              // replace this IR with GetPropertyFree
              const newIR: NativeCompilerIR.GetPropertyFree = {
                ...typedIR,
                kind: NativeCompilerIR.Kind.GetPropertyFree,
              };
              output.push(newIR);
              continue; // skip the old IR
            }
          }
          if (lastIR) {
            output.push(lastIR);
          }
          output.push(ir);
          break;
        }
        default:
          output.push(ir);
          break;
      }
    }

    return output;
  }
}
