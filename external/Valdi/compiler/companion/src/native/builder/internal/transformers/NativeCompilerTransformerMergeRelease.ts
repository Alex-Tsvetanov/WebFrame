import { NativeCompilerIR } from '../..//NativeCompilerBuilderIR';
import { NativeCompilerBuilderVariableID } from '../../INativeCompilerBuilder';

/**
 * Transformer that replaces multiple successive release operations with one variadic release.
 */
export namespace NativeCompilerTransformerMergeRelease {
  function emitFree(output: NativeCompilerIR.Base[], vars: NativeCompilerBuilderVariableID[]) {
    if (vars.length > 1) {
      // emit a FreeV to replace multipe Free
      const freev: NativeCompilerIR.FreeV = {
        kind: NativeCompilerIR.Kind.FreeV,
        values: vars,
      };
      output.push(freev);
    } else if (vars.length == 1) {
      // emit the normal Free if there is a single item to free
      const free: NativeCompilerIR.Free = {
        kind: NativeCompilerIR.Kind.Free,
        value: vars[0],
      };
      output.push(free);
    }
  }

  export function transform(irs: NativeCompilerIR.Base[]): NativeCompilerIR.Base[] {
    const output: NativeCompilerIR.Base[] = [];
    var variablesToFree: NativeCompilerBuilderVariableID[] = [];
    for (let ir of irs) {
      if (ir.kind == NativeCompilerIR.Kind.Free) {
        // store variables to free
        // don't emit anything
        const free = ir as NativeCompilerIR.Free;
        variablesToFree.push(free.value);
      } else {
        // flush stored variables if present
        emitFree(output, variablesToFree);
        variablesToFree = [];
        // copy input to output
        output.push(ir);
      }
    }
    // check again at the end
    emitFree(output, variablesToFree);
    variablesToFree = [];
    return output;
  }
}
