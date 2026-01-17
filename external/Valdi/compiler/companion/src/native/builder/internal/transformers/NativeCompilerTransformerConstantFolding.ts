import { NativeCompilerIR } from '../../NativeCompilerBuilderIR';
import { ConstantType, ConstValue, NativeCompilerBuilderVariableID } from '../../INativeCompilerBuilder';
import { visitVariables } from './utils/IRVisitors';
import { NativeCompilerModuleBuilder } from '../../NativeCompilerBuilder';

export namespace NativeCompilerTransformerConstantFolding {
  interface ConstantDef {
    type: ConstantType;
    value: string;
    index: number; // where does this constant start to come into effect
  }

  const numericalTypes = [ConstantType.Integer, ConstantType.Long, ConstantType.Double];

  function buildLiteralInteger(
    variable: NativeCompilerBuilderVariableID,
    value: string,
  ): NativeCompilerIR.LiteralInteger {
    return {
      kind: NativeCompilerIR.Kind.LiteralInteger,
      variable: variable,
      value: value,
    };
  }

  function buildLiteralLong(variable: NativeCompilerBuilderVariableID, value: string): NativeCompilerIR.LiteralLong {
    return {
      kind: NativeCompilerIR.Kind.LiteralLong,
      variable: variable,
      value: value,
    };
  }
  function buildLiteralDouble(
    variable: NativeCompilerBuilderVariableID,
    value: string,
  ): NativeCompilerIR.LiteralDouble {
    return {
      kind: NativeCompilerIR.Kind.LiteralDouble,
      variable: variable,
      value: value,
    };
  }
  function buildLiteralBool(variable: NativeCompilerBuilderVariableID, value: boolean): NativeCompilerIR.LiteralBool {
    return {
      kind: NativeCompilerIR.Kind.LiteralBool,
      variable: variable,
      value: value,
    };
  }

  type UnaryOp = (variable: NativeCompilerBuilderVariableID, c: ConstantDef) => NativeCompilerIR.Base | undefined;
  type BinaryOp = (
    variable: NativeCompilerBuilderVariableID,
    moduleBuilder: NativeCompilerModuleBuilder,
    left: ConstantDef,
    right: ConstantDef,
  ) => NativeCompilerIR.Base | undefined;

  function doNeg(variable: NativeCompilerBuilderVariableID, c: ConstantDef): NativeCompilerIR.Base | undefined {
    switch (c.type) {
      case ConstantType.Double:
        return buildLiteralDouble(variable, (-Number(c.value)).toString());
      case ConstantType.Integer:
        return buildLiteralInteger(variable, (-Number(c.value)).toString());
      case ConstantType.Long:
        return buildLiteralLong(variable, (-Number(c.value)).toString());
      default:
        return undefined;
    }
  }
  function doPlus(variable: NativeCompilerBuilderVariableID, c: ConstantDef): NativeCompilerIR.Base | undefined {
    switch (c.type) {
      case ConstantType.Double:
        return buildLiteralDouble(variable, c.value);
      case ConstantType.Integer:
        return buildLiteralInteger(variable, c.value);
      case ConstantType.Long:
        return buildLiteralLong(variable, c.value);
      default:
        return undefined;
    }
  }
  function doBitwiseNot(variable: NativeCompilerBuilderVariableID, c: ConstantDef): NativeCompilerIR.Base | undefined {
    switch (c.type) {
      case ConstantType.Integer:
        return buildLiteralInteger(variable, String(~Number(c.value)));
      case ConstantType.Long:
        return buildLiteralLong(variable, String(~Number(c.value)));
      default:
        return undefined;
    }
  }
  function doLogicalNot(variable: NativeCompilerBuilderVariableID, c: ConstantDef): NativeCompilerIR.Base | undefined {
    switch (c.type) {
      case ConstantType.Bool:
        return buildLiteralBool(variable, !(c.value == 'true'));
      default:
        return undefined;
    }
  }

  const maxInt32 = 0x7fffffff;
  const minInt32 = -2147483648;

  function isSafeInt32(x: number): boolean {
    return x <= maxInt32 && x >= minInt32 && x % 1 === 0;
  }

  function doBinary(
    variable: NativeCompilerBuilderVariableID,
    operator: (left: number, right: number) => number,
    left: ConstantDef,
    right: ConstantDef,
  ) {
    if (numericalTypes.includes(left.type) && numericalTypes.includes(right.type)) {
      const [smallerType, largerType] = left.type <= right.type ? [left.type, right.type] : [right.type, left.type];
      const res = operator(Number(left.value), Number(right.value));
      // int + int
      if (smallerType === ConstantType.Integer && largerType === ConstantType.Integer) {
        if (isSafeInt32(res)) {
          return buildLiteralInteger(variable, String(res));
        } else {
          return buildLiteralDouble(variable, String(res));
        }
      }
      // long + long
      if (smallerType === ConstantType.Long && largerType === ConstantType.Long) {
        return buildLiteralLong(variable, String(res));
      }
      // double + double
      if (smallerType === ConstantType.Double && largerType === ConstantType.Double) {
        return buildLiteralDouble(variable, String(res));
      }
      // int + long
      if (smallerType === ConstantType.Integer && largerType === ConstantType.Long) {
        return buildLiteralLong(variable, String(res));
      }
      // int + double
      if (smallerType === ConstantType.Integer && largerType === ConstantType.Double) {
        return buildLiteralDouble(variable, String(res));
      }
      // long + double
      if (smallerType === ConstantType.Long && largerType === ConstantType.Double) {
        return buildLiteralDouble(variable, String(res));
      }
    }
    return undefined;
  }

  function doBand(
    variable: NativeCompilerBuilderVariableID,
    moduleBuilder: NativeCompilerModuleBuilder,
    left: ConstantDef,
    right: ConstantDef,
  ): NativeCompilerIR.Base | undefined {
    return doBinary(variable, (x, y) => x & y, left, right);
  }
  function doBor(
    variable: NativeCompilerBuilderVariableID,
    moduleBuilder: NativeCompilerModuleBuilder,
    left: ConstantDef,
    right: ConstantDef,
  ): NativeCompilerIR.Base | undefined {
    return doBinary(variable, (x, y) => x | y, left, right);
  }

  function doAdd(
    variable: NativeCompilerBuilderVariableID,
    moduleBuilder: NativeCompilerModuleBuilder,
    left: ConstantDef,
    right: ConstantDef,
  ): NativeCompilerIR.Base | undefined {
    // str + str
    if (left.type === ConstantType.String && right.type === ConstantType.String) {
      const constantIndex = moduleBuilder.registerStringConstant(left.value + right.value);
      const newIR: NativeCompilerIR.GetModuleConst = {
        kind: NativeCompilerIR.Kind.GetModuleConst,
        variable: variable,
        index: constantIndex,
      };
      return newIR;
    }
    // Number types
    return doBinary(variable, (x, y) => x + y, left, right);
  }

  function doMulti(
    variable: NativeCompilerBuilderVariableID,
    moduleBuilder: NativeCompilerModuleBuilder,
    left: ConstantDef,
    right: ConstantDef,
  ): NativeCompilerIR.Base | undefined {
    return doBinary(variable, (x, y) => x * y, left, right);
  }

  function doSub(
    variable: NativeCompilerBuilderVariableID,
    moduleBuilder: NativeCompilerModuleBuilder,
    left: ConstantDef,
    right: ConstantDef,
  ): NativeCompilerIR.Base | undefined {
    return doBinary(variable, (x, y) => x - y, left, right);
  }

  function doDiv(
    variable: NativeCompilerBuilderVariableID,
    moduleBuilder: NativeCompilerModuleBuilder,
    left: ConstantDef,
    right: ConstantDef,
  ): NativeCompilerIR.Base | undefined {
    return doBinary(variable, (x, y) => x / y, left, right);
  }

  interface TransformResult {
    irs: NativeCompilerIR.Base[];
    changes: number;
  }

  function addNewIR(output: TransformResult, newIR: NativeCompilerIR.Base) {
    output.irs.push(newIR);
    output.changes++;
  }

  export function transform(irs: NativeCompilerIR.Base[], moduleBuilder: NativeCompilerModuleBuilder): TransformResult {
    const output: TransformResult = { irs: [], changes: 0 };
    const constants = new Map<number, ConstantDef>();
    const unused = new Set<number>();
    const constVarRefs = new Map<number, ConstValue>();

    // pass 1:
    // * identify all constants. constants are created from literal and then never assigned.
    // * mark all unused constants
    // * find assignments of constants to vrefs and update the vref's const value
    irs.forEach((ir, idx) => {
      // if variable is touched, remove from unused candidate
      visitVariables(ir, false, (v) => {
        unused.delete(v.variable);
        return v;
      });
      switch (ir.kind) {
        // Create constant candidates when we see literals
        case NativeCompilerIR.Kind.LiteralBool: {
          const typedIR = ir as NativeCompilerIR.LiteralBool;
          constants.set(typedIR.variable.variable, {
            type: ConstantType.Bool,
            value: typedIR.value ? 'true' : 'false',
            index: idx,
          });
          unused.add(typedIR.variable.variable);
          break;
        }
        case NativeCompilerIR.Kind.LiteralDouble: {
          const typedIR = ir as NativeCompilerIR.LiteralDouble;
          constants.set(typedIR.variable.variable, {
            type: ConstantType.Double,
            value: typedIR.value,
            index: idx,
          });
          unused.add(typedIR.variable.variable);
          break;
        }
        case NativeCompilerIR.Kind.LiteralInteger: {
          const typedIR = ir as NativeCompilerIR.LiteralInteger;
          constants.set(typedIR.variable.variable, {
            type: ConstantType.Integer,
            value: typedIR.value,
            index: idx,
          });
          unused.add(typedIR.variable.variable);
          break;
        }
        case NativeCompilerIR.Kind.LiteralLong: {
          const typedIR = ir as NativeCompilerIR.LiteralLong;
          constants.set(typedIR.variable.variable, {
            type: ConstantType.Long,
            value: typedIR.value,
            index: idx,
          });
          unused.add(typedIR.variable.variable);
          break;
        }
        case NativeCompilerIR.Kind.LiteralString: {
          const typedIR = ir as NativeCompilerIR.LiteralString;
          constants.set(typedIR.variable.variable, {
            type: ConstantType.String,
            value: typedIR.value,
            index: idx,
          });
          unused.add(typedIR.variable.variable);
          break;
        }
        case NativeCompilerIR.Kind.GetModuleConst: {
          const typedIR = ir as NativeCompilerIR.GetModuleConst;
          constants.set(typedIR.variable.variable, {
            type: ConstantType.String,
            value: moduleBuilder.stringConstants[typedIR.index],
            index: idx,
          });
          unused.add(typedIR.variable.variable);
          break;
        }
        // If a candidate is assigned, it's no longer a constant
        case NativeCompilerIR.Kind.Assignment: {
          const typedIR = ir as NativeCompilerIR.Assignment;
          if (constants.has(typedIR.left.variable)) {
            constants.delete(typedIR.left.variable);
          }
          break;
        }
        // find const vrefs defined in this function
        case NativeCompilerIR.Kind.NewVariableRef: {
          const typedIR = ir as NativeCompilerIR.NewVariableRef;
          if (typedIR.constValue) {
            constVarRefs.set(typedIR.variable.variable, typedIR.constValue);
          }
          break;
        }
        // find const vrefs captured from parent
        case NativeCompilerIR.Kind.GetClosureArg: {
          const typedIR = ir as NativeCompilerIR.GetClosureArg;
          if (typedIR.constValue) {
            constVarRefs.set(typedIR.variable.variable, typedIR.constValue);
          }
          break;
        }
        // update const vref's value
        case NativeCompilerIR.Kind.SetVariableRef: {
          const typedIR = ir as NativeCompilerIR.SetVariableRef;
          const c = constants.get(typedIR.value.variable);
          const vref = constVarRefs.get(typedIR.target.variable);
          if (vref && vref.value === undefined && c) {
            //console.log(`found assignment to const vref ${typedIR.target.variable} with ${c.value}`);
            vref.value = c.value;
            vref.ctype = c.type;
          }
          break;
        }
      }
    });
    // pass 2:
    // * replace computations that entirely depends on constants with end result
    // * delete unused literals
    // * replace load vref with constant when available
    irs.forEach((ir, idx) => {
      switch (ir.kind) {
        case NativeCompilerIR.Kind.LiteralBool:
        case NativeCompilerIR.Kind.LiteralInteger:
        case NativeCompilerIR.Kind.LiteralLong:
        case NativeCompilerIR.Kind.LiteralDouble:
        case NativeCompilerIR.Kind.LiteralString:
        case NativeCompilerIR.Kind.GetModuleConst: {
          const typedIR = ir as NativeCompilerIR.BaseWithReturn;
          if (unused.has(typedIR.variable.variable)) {
            // skip unused literal
            return;
          }
          break;
        }
        case NativeCompilerIR.Kind.UnaryOp: {
          const typedIR = ir as NativeCompilerIR.UnaryOp;
          const c = constants.get(typedIR.operand.variable);
          if (c && idx > c.index) {
            const dispatcher = new Map<NativeCompilerIR.UnaryOperator, UnaryOp>([
              [NativeCompilerIR.UnaryOperator.Neg, doNeg],
              [NativeCompilerIR.UnaryOperator.Plus, doPlus],
              [NativeCompilerIR.UnaryOperator.BitwiseNot, doBitwiseNot],
              [NativeCompilerIR.UnaryOperator.LogicalNot, doLogicalNot],
            ]);
            const newIR = dispatcher.get(typedIR.operator)?.(typedIR.variable, c);
            if (newIR) {
              addNewIR(output, newIR);
              return; // skip original IR
            }
          }
          break;
        }
        case NativeCompilerIR.Kind.BinaryOp: {
          const typedIR = ir as NativeCompilerIR.BinaryOp;
          const c1 = constants.get(typedIR.left.variable);
          const c2 = constants.get(typedIR.right.variable);
          if (c1 && c2 && idx > c1.index && idx > c2.index) {
            const dispatcher = new Map<NativeCompilerIR.BinaryOperator, BinaryOp>([
              [NativeCompilerIR.BinaryOperator.Add, doAdd],
              [NativeCompilerIR.BinaryOperator.Sub, doSub],
              [NativeCompilerIR.BinaryOperator.Mult, doMulti],
              [NativeCompilerIR.BinaryOperator.Div, doDiv],
              [NativeCompilerIR.BinaryOperator.BitwiseAND, doBand],
              [NativeCompilerIR.BinaryOperator.BitwiseOR, doBor],
            ]);
            const newIR = dispatcher.get(typedIR.operator)?.(typedIR.variable, moduleBuilder, c1, c2);
            if (newIR) {
              addNewIR(output, newIR);
              return; // skip original IR
            }
          }
          break;
        }
        case NativeCompilerIR.Kind.LoadVariableRef: {
          const typedIR = ir as NativeCompilerIR.LoadVariableRef;
          const vref = constVarRefs.get(typedIR.target.variable);
          if (vref && vref.value && vref.ctype !== undefined) {
            //console.log(`replace const vref with value ${vref.value}`);
            switch (vref.ctype) {
              case ConstantType.Bool: {
                const newIR: NativeCompilerIR.LiteralBool = {
                  kind: NativeCompilerIR.Kind.LiteralBool,
                  value: vref.value == 'true',
                  variable: typedIR.variable,
                };
                output.irs.push(newIR);
                break;
              }
              case ConstantType.Integer: {
                const newIR: NativeCompilerIR.LiteralInteger = {
                  kind: NativeCompilerIR.Kind.LiteralInteger,
                  value: vref.value,
                  variable: typedIR.variable,
                };
                output.irs.push(newIR);
                break;
              }
              case ConstantType.Long: {
                const newIR: NativeCompilerIR.LiteralLong = {
                  kind: NativeCompilerIR.Kind.LiteralLong,
                  value: vref.value,
                  variable: typedIR.variable,
                };
                output.irs.push(newIR);
                break;
              }
              case ConstantType.Double: {
                const newIR: NativeCompilerIR.LiteralDouble = {
                  kind: NativeCompilerIR.Kind.LiteralDouble,
                  value: vref.value,
                  variable: typedIR.variable,
                };
                output.irs.push(newIR);
                break;
              }
              case ConstantType.String: {
                const constantIndex = moduleBuilder.registerStringConstant(vref.value);
                const newIR: NativeCompilerIR.GetModuleConst = {
                  kind: NativeCompilerIR.Kind.GetModuleConst,
                  variable: typedIR.variable,
                  index: constantIndex,
                };
                output.irs.push(newIR);
                break;
              }
            }
            output.changes++;
            return; // skip original IR
          }
          break;
        }
      }
      output.irs.push(ir);
    });

    // pass 3:
    // Remove unused closure captures (replaced by constants)
    const unusedVarRefs = new Map<number /*vref*/, number /*idx*/>();
    output.irs.forEach((ir, idx) => {
      switch (ir.kind) {
        case NativeCompilerIR.Kind.GetClosureArg:
          unusedVarRefs.set((ir as NativeCompilerIR.BaseWithReturn).variable.variable, idx);
          break;
        default:
          visitVariables(ir, false, (v) => {
            unusedVarRefs.delete(v.variable);
            return v;
          });
          break;
      }
    });
    unusedVarRefs.forEach((idx, _) => {
      const comment: NativeCompilerIR.Comments = {
        kind: NativeCompilerIR.Kind.Comments,
        text: 'unused vref removed',
      };
      output.irs[idx] = comment;
    });
    return output;
  }
}
