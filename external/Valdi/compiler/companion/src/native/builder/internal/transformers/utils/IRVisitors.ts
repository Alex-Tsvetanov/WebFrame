import { NativeCompilerBuilderJumpTargetID, NativeCompilerBuilderVariableID } from '../../../INativeCompilerBuilder';
import { NativeCompilerIR } from '../../../NativeCompilerBuilderIR';

export function isBaseWithReturn(ir: NativeCompilerIR.Base): ir is NativeCompilerIR.BaseWithReturn {
  switch (ir.kind) {
    case NativeCompilerIR.Kind.GetFunctionArg:
    case NativeCompilerIR.Kind.GetFunctionArgumentsObject:
    case NativeCompilerIR.Kind.GetClosureArg:
    case NativeCompilerIR.Kind.GetModuleConst:
    case NativeCompilerIR.Kind.GetSuper:
    case NativeCompilerIR.Kind.GetSuperConstructor:
    case NativeCompilerIR.Kind.LiteralInteger:
    case NativeCompilerIR.Kind.LiteralLong:
    case NativeCompilerIR.Kind.LiteralDouble:
    case NativeCompilerIR.Kind.LiteralString:
    case NativeCompilerIR.Kind.LiteralBool:
    case NativeCompilerIR.Kind.NewObject:
    case NativeCompilerIR.Kind.NewArray:
    case NativeCompilerIR.Kind.Global:
    case NativeCompilerIR.Kind.Keyword:
    case NativeCompilerIR.Kind.GetException:
    case NativeCompilerIR.Kind.Exception:
    case NativeCompilerIR.Kind.GetProperty:
    case NativeCompilerIR.Kind.GetPropertyValue:
    case NativeCompilerIR.Kind.DeleteProperty:
    case NativeCompilerIR.Kind.DeletePropertyValue:
    case NativeCompilerIR.Kind.UnaryOp:
    case NativeCompilerIR.Kind.BinaryOp:
    case NativeCompilerIR.Kind.NewArrowFunctionValue:
    case NativeCompilerIR.Kind.NewFunctionValue:
    case NativeCompilerIR.Kind.NewClassValue:
    case NativeCompilerIR.Kind.IntrinsicCall:
    case NativeCompilerIR.Kind.FunctionInvocation:
    case NativeCompilerIR.Kind.ConstructorInvocation:
    case NativeCompilerIR.Kind.NewVariableRef:
    case NativeCompilerIR.Kind.LoadVariableRef:
    case NativeCompilerIR.Kind.Iterator:
    case NativeCompilerIR.Kind.KeysIterator:
    case NativeCompilerIR.Kind.IteratorNext:
    case NativeCompilerIR.Kind.KeysIteratorNext:
    case NativeCompilerIR.Kind.Generator:
    case NativeCompilerIR.Kind.Resume:
      return true;
    case NativeCompilerIR.Kind.SetProperty:
    case NativeCompilerIR.Kind.SetPropertyValue:
    case NativeCompilerIR.Kind.SetPropertyIndex:
    case NativeCompilerIR.Kind.CopyPropertiesFrom:
    case NativeCompilerIR.Kind.Retain:
    case NativeCompilerIR.Kind.Branch:
    case NativeCompilerIR.Kind.Throw:
    case NativeCompilerIR.Kind.Free:
    case NativeCompilerIR.Kind.Assignment:
    case NativeCompilerIR.Kind.Jump:
    case NativeCompilerIR.Kind.Comments:
    case NativeCompilerIR.Kind.ProgramCounterInfo:
    case NativeCompilerIR.Kind.BindJumpTarget:
    case NativeCompilerIR.Kind.SetVariableRef:
    case NativeCompilerIR.Kind.SetClassElement:
    case NativeCompilerIR.Kind.SetClassElementValue:
    case NativeCompilerIR.Kind.CheckException:
    case NativeCompilerIR.Kind.Reentry:
    case NativeCompilerIR.Kind.SetResumePoint:
      return false;
    default:
      throw new Error(`Not all IRs are supported: ${ir.kind}`);
  }
}

export function isBaseWithExceptionTarget(ir: NativeCompilerIR.Base): ir is NativeCompilerIR.BaseWithExceptionTarget {
  if ((ir as unknown as NativeCompilerIR.BaseWithExceptionTarget).exceptionTarget) {
    return true;
  }
  return false;
}

function visitVariableArray(
  variables: NativeCompilerBuilderVariableID[],
  transform: boolean,
  visitor: (variable: NativeCompilerBuilderVariableID) => NativeCompilerBuilderVariableID,
): NativeCompilerBuilderVariableID[] {
  if (transform) {
    const out: NativeCompilerBuilderVariableID[] = [];

    for (const variable of variables) {
      out.push(visitor(variable));
    }

    return out;
  } else {
    for (const variable of variables) {
      visitor(variable);
    }
    return variables;
  }
}

export function visitVariables(
  ir: NativeCompilerIR.Base,
  transform: boolean,
  visitor: (variable: NativeCompilerBuilderVariableID) => NativeCompilerBuilderVariableID,
): NativeCompilerIR.Base {
  switch (ir.kind) {
    case NativeCompilerIR.Kind.GetFunctionArg:
    case NativeCompilerIR.Kind.GetFunctionArgumentsObject:
    case NativeCompilerIR.Kind.GetClosureArg:
    case NativeCompilerIR.Kind.GetModuleConst:
    case NativeCompilerIR.Kind.LiteralInteger:
    case NativeCompilerIR.Kind.LiteralLong:
    case NativeCompilerIR.Kind.LiteralDouble:
    case NativeCompilerIR.Kind.LiteralString:
    case NativeCompilerIR.Kind.LiteralBool:
    case NativeCompilerIR.Kind.NewObject:
    case NativeCompilerIR.Kind.NewArray:
    case NativeCompilerIR.Kind.Global:
    case NativeCompilerIR.Kind.Keyword:
    case NativeCompilerIR.Kind.GetException:
    case NativeCompilerIR.Kind.NewVariableRef:
    case NativeCompilerIR.Kind.Exception: {
      let typedIR = ir as NativeCompilerIR.BaseWithReturn;
      const variable = visitor(typedIR.variable);

      if (transform) {
        typedIR = { ...typedIR, variable };
      }

      return typedIR;
    }
    case NativeCompilerIR.Kind.GetSuper: {
      let typedIR = ir as NativeCompilerIR.GetSuper;
      const variable = visitor(typedIR.variable);
      if (transform) {
        typedIR = { ...typedIR, variable };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.GetSuperConstructor: {
      let typedIR = ir as NativeCompilerIR.GetSuperConstructor;
      const variable = visitor(typedIR.variable);
      if (transform) {
        typedIR = { ...typedIR, variable };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.SetProperty: {
      let typedIR = ir as NativeCompilerIR.SetProperty;
      const object = visitor(typedIR.object);
      const values = typedIR.values.map((v) => {
        return visitor(v);
      });
      if (transform) {
        typedIR = { ...typedIR, object, values };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.SetPropertyValue: {
      let typedIR = ir as NativeCompilerIR.SetPropertyValue;
      const object = visitor(typedIR.object);
      const property = visitor(typedIR.property);
      const value = visitor(typedIR.value);
      if (transform) {
        typedIR = { ...typedIR, object, property, value };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.SetPropertyIndex: {
      let typedIR = ir as NativeCompilerIR.SetPropertyIndex;
      const object = visitor(typedIR.object);
      const value = visitor(typedIR.value);
      if (transform) {
        typedIR = { ...typedIR, object, value };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.CopyPropertiesFrom: {
      let typedIR = ir as NativeCompilerIR.CopyPropertiesFrom;
      const object = visitor(typedIR.object);
      const value = visitor(typedIR.value);
      const copiedPropertiesCount = visitor(typedIR.copiedPropertiesCount);
      if (transform) {
        typedIR = { ...typedIR, object, value, copiedPropertiesCount };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.Retain: {
      let typedIR = ir as NativeCompilerIR.Retain;
      const value = visitor(typedIR.value);
      if (transform) {
        typedIR = { ...typedIR, value };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.Branch: {
      let typedIR = ir as NativeCompilerIR.Branch;
      const conditionVariable = visitor(typedIR.conditionVariable);
      if (transform) {
        typedIR = { ...typedIR, conditionVariable };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.Throw: {
      let typedIR = ir as NativeCompilerIR.Throw;
      const value = visitor(typedIR.value);
      if (transform) {
        typedIR = { ...typedIR, value };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.Free: {
      let typedIR = ir as NativeCompilerIR.Free;
      const value = visitor(typedIR.value);
      if (transform) {
        typedIR = { ...typedIR, value };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.FreeV: {
      let typedIR = ir as NativeCompilerIR.FreeV;
      const values = typedIR.values.map((v) => {
        return visitor(v);
      });
      if (transform) {
        typedIR = { ...typedIR, values };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.GetProperty: {
      let typedIR = ir as NativeCompilerIR.GetProperty;
      const variable = visitor(typedIR.variable);
      const object = visitor(typedIR.object);
      const thisObject = visitor(typedIR.thisObject);
      if (transform) {
        typedIR = { ...typedIR, variable, object, thisObject };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.GetPropertyFree: {
      let typedIR = ir as NativeCompilerIR.GetPropertyFree;
      const variable = visitor(typedIR.variable);
      const object = visitor(typedIR.object);
      const thisObject = visitor(typedIR.thisObject);
      if (transform) {
        typedIR = { ...typedIR, variable, object, thisObject };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.GetPropertyValue: {
      let typedIR = ir as NativeCompilerIR.GetPropertyValue;
      const variable = visitor(typedIR.variable);
      const property = visitor(typedIR.property);
      const object = visitor(typedIR.object);
      const thisObject = visitor(typedIR.thisObject);
      if (transform) {
        typedIR = { ...typedIR, variable, object, property, thisObject };
      }

      return typedIR;
    }
    case NativeCompilerIR.Kind.DeleteProperty: {
      let typedIR = ir as NativeCompilerIR.DeleteProperty;
      const variable = visitor(typedIR.variable);
      const object = visitor(typedIR.object);
      if (transform) {
        typedIR = { ...typedIR, variable, object };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.DeletePropertyValue: {
      let typedIR = ir as NativeCompilerIR.DeletePropertyValue;
      const variable = visitor(typedIR.variable);
      const property = visitor(typedIR.property);
      const object = visitor(typedIR.object);
      if (transform) {
        typedIR = { ...typedIR, variable, object, property };
      }

      return typedIR;
    }
    case NativeCompilerIR.Kind.UnaryOp: {
      let typedIR = ir as NativeCompilerIR.UnaryOp;
      const variable = visitor(typedIR.variable);
      const operand = visitor(typedIR.operand);

      if (transform) {
        typedIR = { ...typedIR, variable, operand };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.BinaryOp: {
      let typedIR = ir as NativeCompilerIR.BinaryOp;
      const variable = visitor(typedIR.variable);
      const left = visitor(typedIR.left);
      const right = visitor(typedIR.right);

      if (transform) {
        typedIR = { ...typedIR, variable, left, right };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.NewArrowFunctionValue:
    case NativeCompilerIR.Kind.NewFunctionValue: {
      let typedIR = ir as NativeCompilerIR.NewArrowFunctionValue | NativeCompilerIR.NewFunctionValue;
      const variable = visitor(typedIR.variable);
      const closureArgs = visitVariableArray(typedIR.closureArgs, transform, visitor);

      if (transform) {
        typedIR = { ...typedIR, variable, closureArgs };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.NewClassValue: {
      let typedIR = ir as NativeCompilerIR.NewClassValue;
      const variable = visitor(typedIR.variable);
      const parentClass = visitor(typedIR.parentClass);
      const closureArgs = visitVariableArray(typedIR.closureArgs, transform, visitor);

      if (transform) {
        typedIR = { ...typedIR, variable, parentClass, closureArgs };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.SetClassElement: {
      let typedIR = ir as NativeCompilerIR.SetClassElement;
      const cls = visitor(typedIR.cls);
      const value = visitor(typedIR.value);

      if (transform) {
        typedIR = { ...typedIR, cls, value };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.SetClassElementValue: {
      let typedIR = ir as NativeCompilerIR.SetClassElementValue;
      const cls = visitor(typedIR.cls);
      const name = visitor(typedIR.name);
      const value = visitor(typedIR.value);

      if (transform) {
        typedIR = { ...typedIR, cls, name, value };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.IntrinsicCall: {
      let typedIR = ir as NativeCompilerIR.FunctionInvocation;
      const variable = visitor(typedIR.variable);
      const args = visitVariableArray(typedIR.args, transform, visitor);
      if (transform) {
        typedIR = { ...typedIR, variable, args };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.FunctionInvocation: {
      let typedIR = ir as NativeCompilerIR.FunctionInvocation;
      const variable = visitor(typedIR.variable);
      const func = visitor(typedIR.func);
      const obj = visitor(typedIR.obj);
      const args = visitVariableArray(typedIR.args, transform, visitor);

      if (transform) {
        typedIR = { ...typedIR, variable, func, obj, args };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.ConstructorInvocation: {
      let typedIR = ir as NativeCompilerIR.ConstructorInvocation;
      const variable = visitor(typedIR.variable);
      const func = visitor(typedIR.func);
      const new_target = visitor(typedIR.new_target);
      const args = visitVariableArray(typedIR.args, transform, visitor);

      if (transform) {
        typedIR = { ...typedIR, variable, func, new_target, args };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.Assignment: {
      let typedIR = ir as NativeCompilerIR.Assignment;
      const left = visitor(typedIR.left);
      const right = visitor(typedIR.right);
      if (transform) {
        typedIR = { ...typedIR, left, right };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.LoadVariableRef: {
      let typedIR = ir as NativeCompilerIR.LoadVariableRef;
      const target = visitor(typedIR.target);
      const variable = visitor(typedIR.variable);
      if (transform) {
        typedIR = { ...typedIR, target, variable };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.SetVariableRef: {
      let typedIR = ir as NativeCompilerIR.SetVariableRef;
      const target = visitor(typedIR.target);
      const value = visitor(typedIR.value);
      if (transform) {
        typedIR = { ...typedIR, target, value };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.Iterator:
    case NativeCompilerIR.Kind.KeysIterator: {
      let typedIR = ir as NativeCompilerIR.Iterator | NativeCompilerIR.KeysIterator;
      const arg = visitor(typedIR.arg);
      const variable = visitor(typedIR.variable);
      if (transform) {
        typedIR = { ...typedIR, arg, variable };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.IteratorNext:
    case NativeCompilerIR.Kind.KeysIteratorNext: {
      let typedIR = ir as NativeCompilerIR.IteratorNext | NativeCompilerIR.KeysIteratorNext;
      const iterator = visitor(typedIR.iterator);
      const variable = visitor(typedIR.variable);
      if (transform) {
        typedIR = { ...typedIR, iterator, variable };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.Generator: {
      let typedIR = ir as NativeCompilerIR.Generator;
      const arg = visitor(typedIR.arg);
      const variable = visitor(typedIR.variable);
      if (transform) {
        typedIR = { ...typedIR, arg, variable };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.Resume: {
      let typedIR = ir as NativeCompilerIR.Resume;
      const variable = visitor(typedIR.variable);
      if (transform) {
        typedIR = { ...typedIR, variable };
      }
      return typedIR;
    }
    case NativeCompilerIR.Kind.CheckException:
    case NativeCompilerIR.Kind.Jump:
    case NativeCompilerIR.Kind.Comments:
    case NativeCompilerIR.Kind.ProgramCounterInfo:
    case NativeCompilerIR.Kind.BindJumpTarget:
    case NativeCompilerIR.Kind.Reentry:
    case NativeCompilerIR.Kind.SetResumePoint:
      return ir;
    default:
      throw new Error(`Not all IRs are supported: ${ir.kind}`);
  }
}

export function visitJump(ir: NativeCompilerIR.Base, visitor: (jump: NativeCompilerBuilderJumpTargetID) => void) {
  switch (ir.kind) {
    case NativeCompilerIR.Kind.Throw:
      {
        const typedIR = ir as NativeCompilerIR.Throw;
        visitor(typedIR.target);
      }
      break;
    case NativeCompilerIR.Kind.Jump:
      {
        const typedIR = ir as NativeCompilerIR.Jump;
        visitor(typedIR.target);
      }
      break;
    case NativeCompilerIR.Kind.Branch:
      {
        const typedIR = ir as NativeCompilerIR.Branch;
        visitor(typedIR.trueTarget);
        if (typedIR.falseTarget) {
          visitor(typedIR.falseTarget);
        }
      }
      break;
    case NativeCompilerIR.Kind.Reentry:
      {
        const typedIR = ir as NativeCompilerIR.Reentry;
        for (const rp of typedIR.resumePoints) {
          visitor(rp);
        }
      }
      break;
    default:
      {
        if (isBaseWithExceptionTarget(ir)) {
          const typedIr = ir as NativeCompilerIR.BaseWithExceptionTarget;
          visitor(typedIr.exceptionTarget);
        }
      }
      break;
  }
}
