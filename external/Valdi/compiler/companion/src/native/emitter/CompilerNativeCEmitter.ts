import {
  NativeCompilerBuilderVariableID,
  NativeCompilerBuilderJumpTargetID,
  NativeCompilerBuilderAtomID,
  NativeCompilerBuilderBranchType,
  NativeCompilerBuilderVariableType,
} from '../builder/INativeCompilerBuilder';
import { NativeCompilerIR } from '../builder/NativeCompilerBuilderIR';
import { NativeCodeWriter } from '../NativeCodeWriter';
import { escapeCComment, toCString } from '../utils/StringEscape';
import { NativeCompilerOptions } from '../NativeCompilerOptions';

import { INativeCompilerIREmitter, NativeCompilerEmitterOutput } from './INativeCompilerEmitter';

const getNativeCompilerFunctionArgToStringLUT = {
  [NativeCompilerIR.FunctionTypeArgs.Context]: 'ctx',
  [NativeCompilerIR.FunctionTypeArgs.This]: 'this_val',
  [NativeCompilerIR.FunctionTypeArgs.StackFrame]: 'stackframe',
  [NativeCompilerIR.FunctionTypeArgs.Argc]: 'argc',
  [NativeCompilerIR.FunctionTypeArgs.Argv]: 'argv',
  [NativeCompilerIR.FunctionTypeArgs.Closure]: 'closure',
  [NativeCompilerIR.FunctionTypeArgs.NewTarget]: 'new_target',
};

const getNativeFunctionArguementDeclarationStringLUT = {
  [NativeCompilerIR.FunctionTypeArgs.Context]:
    'tsn_vm *' + getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
  [NativeCompilerIR.FunctionTypeArgs.This]:
    'tsn_value_const ' + getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.This),
  [NativeCompilerIR.FunctionTypeArgs.Argc]:
    'int ' + getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Argc),
  [NativeCompilerIR.FunctionTypeArgs.StackFrame]:
    'tsn_stackframe *' + getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.StackFrame),
  [NativeCompilerIR.FunctionTypeArgs.Argv]:
    'tsn_value_const *' + getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Argv),
  [NativeCompilerIR.FunctionTypeArgs.Closure]:
    'tsn_closure *' + getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Closure),
  [NativeCompilerIR.FunctionTypeArgs.NewTarget]:
    'tsn_value_const ' + getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.NewTarget),
};

function getNativeCompilerFunctionArgToString(arg: NativeCompilerIR.FunctionTypeArgs): string {
  return getNativeCompilerFunctionArgToStringLUT[arg];
}

function getJumpTargetName(name: string, jumpTarget: NativeCompilerBuilderJumpTargetID) {
  return `target_${name}_${jumpTarget.label}_${jumpTarget.tag}`;
}

function convertFunctionArgsToString(type: NativeCompilerIR.FunctionType) {
  const args = NativeCompilerIR.getFunctionArgsFromType(type);
  return args.map((arg) => getNativeFunctionArguementDeclarationString(arg));
}

function getNativeFunctionArguementDeclarationString(arg: NativeCompilerIR.FunctionTypeArgs): string {
  return getNativeFunctionArguementDeclarationStringLUT[arg];
}

interface CompilerNativeCEmitterFunctionContext {
  readonly name: string;
  readonly type: NativeCompilerIR.FunctionType;
  readonly exceptionJumpTarget: NativeCompilerBuilderJumpTargetID;
  readonly returnJumpTarget: NativeCompilerBuilderJumpTargetID;
  readonly stackOnHeap: boolean;
}

const enum VariadicCallArgumentType {
  Atom,
  VarRef,
  Value,
}

function getVariadicCallArgumentTypeString(argumentType: VariadicCallArgumentType): string {
  switch (argumentType) {
    case VariadicCallArgumentType.Atom:
      return 'tsn_atom';
    case VariadicCallArgumentType.VarRef:
      return 'tsn_var_ref';
    case VariadicCallArgumentType.Value:
      return 'tsn_value';
  }
}

function generateVariadicCallArguments(argumentType: VariadicCallArgumentType, input: string[]): string[] {
  if (input.length === 0) {
    return ['0', `NULL`];
  } else {
    return [input.length.toString(), `(${getVariadicCallArgumentTypeString(argumentType)}[]){${input.join(', ')}}`];
  }
}

export class CompilerNativeCEmitter implements INativeCompilerIREmitter {
  private _functionContext: CompilerNativeCEmitterFunctionContext | undefined;
  private _functionPrototypeBlock: NativeCodeWriter | undefined;
  private writer = new NativeCodeWriter();
  private _symbol: string | undefined;
  private atomDefineNameById: string[] = [];
  private allocatedAtomDefineNames = new Set<string>();
  private pendingRegisterCalls: [string, string][] = [];

  private get functionContext(): CompilerNativeCEmitterFunctionContext {
    if (this._functionContext === undefined) {
      throw new Error('_functionContext needs to be initialized');
    }
    return this._functionContext;
  }

  private get functionPrototypeBlock(): NativeCodeWriter {
    if (this._functionPrototypeBlock === undefined) {
      throw new Error('_functionPrototypeBlock needs to be initialized');
    }
    return this._functionPrototypeBlock;
  }

  private set functionContext(context: CompilerNativeCEmitterFunctionContext | undefined) {
    this._functionContext = context;
  }

  private getVariableName(variableID: NativeCompilerBuilderVariableID) {
    const prefix =
      variableID.type !== NativeCompilerBuilderVariableType.ReturnValue && this.functionContext.stackOnHeap
        ? 'stack->'
        : '';
    return prefix + variableID.toString();
  }

  constructor(readonly options: NativeCompilerOptions) {
    this.emitPrologue();
  }

  private allocateAtom(atom: NativeCompilerBuilderAtomID): string {
    const baseName = `TSN_ATOM_${atom.identifier.toUpperCase()}`;
    let name = baseName;

    let i = 0;
    while (this.allocatedAtomDefineNames.has(name)) {
      name = `${baseName}_${i + 1}`;
      i++;
    }
    this.allocatedAtomDefineNames.add(name);
    this.atomDefineNameById[atom.atom] = name;

    return name;
  }

  private resolveAtomDefineName(atom: NativeCompilerBuilderAtomID): string {
    const atomDefineName = this.atomDefineNameById[atom.atom];
    if (!atomDefineName) {
      throw new Error(`Atom ${atom.identifier} with id ${atom.atom} was not allocated`);
    }

    return atomDefineName;
  }

  private resolveAtomString(atom: NativeCompilerBuilderAtomID): string {
    const atomDefineName = this.resolveAtomDefineName(atom);
    return `module->atoms[${atomDefineName}]`;
  }

  private emitPrologue() {
    this.writer.appendWithNewLine(`#include "tsn/tsn.h"`);
    this.writer.appendWithNewLine(`#include <math.h>`);
  }

  emitBoilerplatePrologue(atoms: NativeCompilerBuilderAtomID[]) {
    this.writer.appendWithNewLine('\n // BEGIN ATOMS');
    for (const atom of atoms) {
      const defineName = this.allocateAtom(atom);
      this.writer.appendWithNewLine(`#define ${defineName} ${atom.atom}`);
    }
    this.writer.appendWithNewLine('// END ATOMS\n');

    this._functionPrototypeBlock = new NativeCodeWriter();
    this.writer.appendWithNewLine(this.functionPrototypeBlock);
  }

  emitBoilerplateEpilogue(
    moduleName: string,
    modulePath: string,
    moduleInitFunctionName: string,
    atoms: readonly NativeCompilerBuilderAtomID[],
    stringConstants: readonly string[],
    propCacheSlots: number,
  ) {
    this.writer.appendWithNewLine(`\n // BEGIN MODULE DEFINITION '${moduleName}'`);
    this.writer.append(`static const tsn_string_def ${moduleName}_module_atoms[] = `);
    this.writer.beginInitialization();
    for (const atom of atoms) {
      const cString = toCString(atom.identifier);
      this.writer.appendWithNewLine(`{${cString.literalContent}, ${cString.length}},`);
    }
    this.writer.endInitialization();
    this.writer.append(`static const tsn_string_def ${moduleName}_module_constants[] = `);
    this.writer.beginInitialization();
    for (const stringConstant of stringConstants) {
      const cString = toCString(stringConstant);
      this.writer.appendWithNewLine(`{${cString.literalContent}, ${cString.length}},`);
    }
    this.writer.endInitialization();

    const functionName = `${moduleName}_create_module_func`;
    this.writer.append(
      `static tsn_value ${functionName}(tsn_vm *${getNativeCompilerFunctionArgToString(
        NativeCompilerIR.FunctionTypeArgs.Context,
      )})`,
    );

    this.writer.beginScope();

    const moduleVarName = 'module';

    this.writer.appendWithNewLine(`tsn_module *${moduleVarName};`);
    this.writer.appendAssignmentWithFunctionCall(moduleVarName, 'tsn_new_module', [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      atoms.length.toString(),
      stringConstants.length.toString(),
      propCacheSlots.toString(),
    ]);

    this.writer.appendFunctionCall('tsn_assign_module_atoms', [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      'module',
      `${moduleName}_module_atoms`,
      atoms.length.toString(),
    ]);
    this.writer.appendFunctionCall('tsn_assign_module_strings', [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      'module',
      `${moduleName}_module_constants`,
      stringConstants.length.toString(),
    ]);
    this.writer.appendWithNewLine(`tsn_value output;`);
    this.writer.appendAssignmentWithFunctionCall('output', 'tsn_new_module_init_fn', [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      moduleVarName,
      `&${moduleInitFunctionName}`,
    ]);

    this.writer.appendFunctionCall('tsn_free_module', [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      moduleVarName,
    ]);

    this.writer.appendWithNewLine(`return output;`);

    this.writer.endScope();

    this.pendingRegisterCalls.push([modulePath, functionName]);
  }

  emitStartFunction(
    name: string,
    type: NativeCompilerIR.FunctionType,
    exceptionJumpTarget: NativeCompilerBuilderJumpTargetID,
    returnJumpTarget: NativeCompilerBuilderJumpTargetID,
    returnVariable: NativeCompilerBuilderVariableID,
    localVars: NativeCompilerBuilderVariableID[],
  ) {
    this.functionContext = {
      name: name,
      type: type,
      exceptionJumpTarget: exceptionJumpTarget,
      returnJumpTarget: returnJumpTarget,
      stackOnHeap: localVars.length > 0,
    };

    if (this.functionContext.stackOnHeap) {
      this.functionPrototypeBlock.appendWithNewLine(`typedef struct {`);
      let numberOfValues = 0;
      let numberOfVRefs = 0;
      let numberOfIterators = 0;
      this.functionPrototypeBlock.withIndentation('    ', () => {
        // all var_refs first
        for (let v of localVars) {
          if (v.type === NativeCompilerBuilderVariableType.VariableRef) {
            this.functionPrototypeBlock.appendWithNewLine(`tsn_var_ref ${v.toString()};`);
            ++numberOfVRefs;
          }
        }
        // followed by values
        for (let v of localVars) {
          if (
            v.type !== NativeCompilerBuilderVariableType.VariableRef &&
            v.type !== NativeCompilerBuilderVariableType.ReturnValue &&
            v.type !== NativeCompilerBuilderVariableType.Iterator
          ) {
            this.functionPrototypeBlock.appendWithNewLine(`tsn_value ${v.toString()};`);
            ++numberOfValues;
          }
        }
        // followed by iterators
        for (let v of localVars) {
          if (v.type === NativeCompilerBuilderVariableType.Iterator) {
            this.functionPrototypeBlock.appendWithNewLine(`tsn_iterator ${v.toString()};`);
            ++numberOfIterators;
          }
        }
      });
      this.functionPrototypeBlock.appendWithNewLine(`} ${name}_stack_vars;`);
      this.functionPrototypeBlock.appendWithNewLine(
        `enum { ${name}_stack_vars_vrefs = ${numberOfVRefs}, ${name}_stack_vars_values = ${numberOfValues}, ${name}_stack_vars_iterators = ${numberOfIterators} };`,
      );
    }

    [this.writer, this.functionPrototypeBlock].forEach((writer) => {
      writer.append('static tsn_value');
      writer.append(` ${name}(`);
      writer.appendParameterList(convertFunctionArgsToString(type));
      writer.append(`)`);
    });
    this.functionPrototypeBlock.appendWithNewLine(';');

    this.writer.appendWithNewLine();
    this.writer.beginScope();

    this.writer.appendWithNewLine(
      `tsn_module *module = ${getNativeCompilerFunctionArgToString(
        NativeCompilerIR.FunctionTypeArgs.Closure,
      )}->module;`,
    );

    this.writer.appendWithNewLine(
      `tsn_value ${this.getVariableName(returnVariable)} = tsn_undefined(${getNativeCompilerFunctionArgToString(
        NativeCompilerIR.FunctionTypeArgs.Context,
      )});`,
    );

    if (this.functionContext.stackOnHeap) {
      this.writer.appendWithNewLine(`${name}_stack_vars* stack = (${name}_stack_vars*)get_closure_stack(closure);`);
    }
  }

  emitEndFunction(returnVariable: NativeCompilerBuilderVariableID) {
    this.writer.appendWithNewLine(`return ${this.getVariableName(returnVariable)};`);
    this.writer.endScope();
    this.writer.appendWithNewLine();
    this.functionContext = undefined;
  }

  emitSlot(value: NativeCompilerBuilderVariableID) {
    if (value.type === NativeCompilerBuilderVariableType.VariableRef) {
      const prefix = this.functionContext.stackOnHeap ? '' : 'tsn_var_ref ';
      this.writer.appendWithNewLine(
        `${prefix}${this.getVariableName(value)} = tsn_empty_var_ref(${getNativeCompilerFunctionArgToString(
          NativeCompilerIR.FunctionTypeArgs.Context,
        )});`,
      );
    } else if (value.type === NativeCompilerBuilderVariableType.Iterator) {
      const prefix = this.functionContext.stackOnHeap ? '' : 'tsn_iterator ';
      this.writer.appendWithNewLine(
        `${prefix}${this.getVariableName(value)} = tsn_empty_iterator(${getNativeCompilerFunctionArgToString(
          NativeCompilerIR.FunctionTypeArgs.Context,
        )});`,
      );
    } else if (value.type === NativeCompilerBuilderVariableType.Null) {
      const prefix = this.functionContext.stackOnHeap ? '' : 'tsn_value ';
      this.writer.appendWithNewLine(
        `${prefix}${this.getVariableName(value)} = tsn_null(${getNativeCompilerFunctionArgToString(
          NativeCompilerIR.FunctionTypeArgs.Context,
        )});`,
      );
    } else if (value.type !== NativeCompilerBuilderVariableType.ReturnValue) {
      const prefix = this.functionContext.stackOnHeap ? '' : 'tsn_value ';
      this.writer.appendWithNewLine(
        `${prefix}${this.getVariableName(value)} = tsn_undefined(${getNativeCompilerFunctionArgToString(
          NativeCompilerIR.FunctionTypeArgs.Context,
        )});`,
      );
    }
  }

  emitGlobal(variable: NativeCompilerBuilderVariableID) {
    this.writer.appendAssignmentWithFunctionCall(this.getVariableName(variable), `tsn_get_global`, [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
    ]);
  }

  emitUndefined(variable: NativeCompilerBuilderVariableID) {
    this.writeAssign(
      variable,
      `tsn_undefined(${getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context)})`,
    );
  }

  emitNull(variable: NativeCompilerBuilderVariableID): void {
    this.writeAssign(
      variable,
      `tsn_null(${getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context)})`,
    );
  }

  emitThis(variable: NativeCompilerBuilderVariableID): void {
    this.writer.appendAssignmentWithFunctionCall(this.getVariableName(variable), 'tsn_retain', [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.This),
    ]);
  }

  emitNewTarget(variable: NativeCompilerBuilderVariableID): void {
    this.writer.appendAssignmentWithFunctionCall(this.getVariableName(variable), 'tsn_retain', [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.NewTarget),
    ]);
  }

  emitException(variable: NativeCompilerBuilderVariableID) {
    this.writeAssign(
      variable,
      `tsn_exception(${getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context)})`,
    );
  }

  emitGetException(variable: NativeCompilerBuilderVariableID): void {
    this.writer.appendAssignmentWithFunctionCall(this.getVariableName(variable), 'tsn_get_exception', [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
    ]);
  }

  emitCheckException(exceptionTarget: NativeCompilerBuilderJumpTargetID): void {
    this.appendCheckedFunctionCall(
      'tsn_has_exception',
      [getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context)],
      exceptionTarget,
    );
  }

  private generateMonoOperandOp(
    retval: NativeCompilerBuilderVariableID,
    operand: NativeCompilerBuilderVariableID,
    funcName: string,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ) {
    // if operand is number or bool, we can omit exception check
    const safeOp =
      operand.type == NativeCompilerBuilderVariableType.Number ||
      operand.type == NativeCompilerBuilderVariableType.Bool;
    const args = [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      this.getVariableName(operand),
    ];
    if (safeOp) {
      this.writer.appendAssignmentWithFunctionCall(this.getVariableName(retval), funcName, args);
    } else {
      this.appendCheckedInOutFunctionCall(retval, funcName, args, exceptionTarget);
    }
  }

  private generateUndefOrNullCheck(
    retval: NativeCompilerBuilderVariableID,
    operand: NativeCompilerBuilderVariableID,
    funcName: string,
  ) {
    const ctx = getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context);
    this.writeAssign(retval, `tsn_new_bool(${ctx}, ${funcName}(${ctx}, ${this.getVariableName(operand)}))`);
  }

  private generateTwoOperandOp(
    retval: NativeCompilerBuilderVariableID,
    left: NativeCompilerBuilderVariableID,
    right: NativeCompilerBuilderVariableID,
    funcName: string,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ) {
    // if both sides are same type of either number or bool, we can omit exception check
    const safeOp =
      left.type == right.type &&
      (left.type == NativeCompilerBuilderVariableType.Number || left.type == NativeCompilerBuilderVariableType.Bool);

    const args = [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      this.getVariableName(left),
      this.getVariableName(right),
    ];

    if (safeOp) {
      this.writer.appendAssignmentWithFunctionCall(this.getVariableName(retval), funcName, args);
    } else {
      this.appendCheckedInOutFunctionCall(retval, funcName, args, exceptionTarget);
    }
  }

  private appendCheckedInOutFunctionCall(
    variable: NativeCompilerBuilderVariableID,
    func: string,
    args: Array<string>,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    const functionContext = this.functionContext;
    this.writer.appendAssignmentWithFunctionCall(this.getVariableName(variable), func, args);

    this.writer.beginIfStatement();

    let functionName: string;
    let byPointer = false;
    switch (variable.type) {
      case NativeCompilerBuilderVariableType.VariableRef:
        functionName = 'tsn_var_ref_is_exception';
        break;
      case NativeCompilerBuilderVariableType.Iterator:
        functionName = 'tsn_iterator_is_exception';
        byPointer = true;
        break;
      default:
        functionName = 'tsn_is_exception';
        break;
    }

    this.writer.appendFunctionCall(
      functionName,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        byPointer ? '&' + this.getVariableName(variable) : this.getVariableName(variable),
      ],
      false,
    );
    this.writer.endEndifStatement();
    this.writer.beginScope();
    this.writer.appendGoto(getJumpTargetName(functionContext.name, exceptionTarget));
    this.writer.endScope();
  }

  private appendCheckedFunctionCall(
    func: string,
    args: Array<string>,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    const functionContext = this.functionContext;
    this.writer.beginIfStatement();
    this.writer.appendFunctionCall(func, args, false);
    this.writer.endEndifStatement();
    this.writer.beginScope();
    this.writer.appendGoto(getJumpTargetName(functionContext.name, exceptionTarget));
    this.writer.endScope();
  }

  emitSetProperty(
    object: NativeCompilerBuilderVariableID,
    properties: NativeCompilerBuilderAtomID[],
    values: NativeCompilerBuilderVariableID[],
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ) {
    if (properties.length == 1) {
      this.appendCheckedFunctionCall(
        'tsn_set_property',
        [
          getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
          '&' + this.getVariableName(object),
          this.resolveAtomString(properties[0]),
          '&' + this.getVariableName(values[0]),
        ],
        exceptionTarget,
      );
    } else {
      const atoms = properties.map((a) => this.resolveAtomDefineName(a)).join(', ');
      const vals = values.map((v) => this.getVariableName(v)).join(', ');
      this.appendCheckedFunctionCall(
        'tsn_set_properties',
        [
          getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
          '&' + this.getVariableName(object),
          properties.length.toString(),
          `(int[]){${atoms}}`,
          `(tsn_value[]){${vals}}`,
          'module',
        ],
        exceptionTarget,
      );
    }
  }

  emitSetPropertyValue(
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
    value: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedFunctionCall(
      `tsn_set_property_value`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(object),
        this.getVariableName(property),
        this.getVariableName(value),
      ],
      exceptionTarget,
    );
  }

  emitSetPropertyIndex(
    object: NativeCompilerBuilderVariableID,
    index: number,
    value: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedFunctionCall(
      `tsn_set_property_index`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(object),
        index.toString(),
        this.getVariableName(value),
      ],
      exceptionTarget,
    );
  }

  emitCopyPropertiesFrom(
    object: NativeCompilerBuilderVariableID,
    value: NativeCompilerBuilderVariableID,
    propertiesToIgnore: NativeCompilerBuilderAtomID[] | undefined,
    copiedPropertiesCount: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    const args: string[] = [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      this.getVariableName(object),
      this.getVariableName(value),
    ];
    if (propertiesToIgnore && propertiesToIgnore.length > 0) {
      args.push(
        ...generateVariadicCallArguments(
          VariadicCallArgumentType.Atom,
          propertiesToIgnore.map((p) => this.resolveAtomString(p)),
        ),
      );

      this.appendCheckedInOutFunctionCall(copiedPropertiesCount, `tsn_copy_filtered_properties`, args, exceptionTarget);
    } else {
      this.appendCheckedInOutFunctionCall(copiedPropertiesCount, `tsn_copy_properties`, args, exceptionTarget);
    }
  }

  emitGetProperty(
    variable: NativeCompilerBuilderVariableID,
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
    thisObject: NativeCompilerBuilderVariableID,
    propCacheSlot: number,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ) {
    this.appendCheckedInOutFunctionCall(
      variable,
      `tsn_get_property`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        '&' + this.getVariableName(object),
        this.resolveAtomString(property),
        '&' + this.getVariableName(thisObject),
        this.options.inlinePropertyCache ? `module->prop_cache + ${propCacheSlot}` : '0',
      ],
      exceptionTarget,
    );
  }
  emitGetPropertyFree(
    variable: NativeCompilerBuilderVariableID,
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
    thisObject: NativeCompilerBuilderVariableID,
    propCacheSlot: number,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ) {
    this.appendCheckedFunctionCall(
      `tsn_get_property_free`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        '&' + this.getVariableName(variable),
        '&' + this.getVariableName(object),
        this.resolveAtomString(property),
        '&' + this.getVariableName(thisObject),
        this.options.inlinePropertyCache ? `module->prop_cache + ${propCacheSlot}` : '0',
      ],
      exceptionTarget,
    );
  }

  emitGetPropertyValue(
    variable: NativeCompilerBuilderVariableID,
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
    thisObject: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ) {
    this.appendCheckedInOutFunctionCall(
      variable,
      `tsn_get_property_value`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(object),
        this.getVariableName(property),
        this.getVariableName(thisObject),
      ],
      exceptionTarget,
    );
  }

  emitDeleteProperty(
    variable: NativeCompilerBuilderVariableID,
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderAtomID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      variable,
      `tsn_delete_property`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(object),
        this.resolveAtomString(property),
      ],
      exceptionTarget,
    );
  }

  emitDeletePropertyValue(
    variable: NativeCompilerBuilderVariableID,
    object: NativeCompilerBuilderVariableID,
    property: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      variable,
      `tsn_delete_property_value`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(object),
        this.getVariableName(property),
      ],
      exceptionTarget,
    );
  }

  emitGetFunctionArg(variable: NativeCompilerBuilderVariableID, index: number) {
    this.writer.appendAssignmentWithFunctionCall(this.getVariableName(variable), 'tsn_get_func_arg', [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Argc),
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Argv),
      index.toString(),
    ]);
  }

  emitGetFunctionArgumentsObject(
    variable: NativeCompilerBuilderVariableID,
    startIndex: number,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      variable,
      'tsn_get_func_args_object',
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Argc),
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Argv),
        startIndex.toString(),
      ],
      exceptionTarget,
    );
  }

  emitGetSuper(
    targetVariable: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      targetVariable,
      'tsn_get_super',
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.StackFrame),
      ],
      exceptionTarget,
    );
  }

  emitGetSuperConstructor(
    targetVariable: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      targetVariable,
      'tsn_get_super_constructor',
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.StackFrame),
      ],
      exceptionTarget,
    );
  }

  emitGetClosureArg(variable: NativeCompilerBuilderVariableID, index: number): void {
    this.writer.appendAssignmentWithFunctionCall(this.getVariableName(variable), 'tsn_get_closure_var', [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Closure),
      index.toString(),
    ]);
  }

  emitGetModuleConst(variable: NativeCompilerBuilderVariableID, index: number): void {
    this.writer.appendAssignmentWithFunctionCall(this.getVariableName(variable), 'tsn_get_module_const', [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      'module',
      index.toString(),
    ]);
  }

  emitLiteralInteger(variable: NativeCompilerBuilderVariableID, value: string) {
    this.writeAssign(
      variable,
      `tsn_int32(${getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context)}, ${value})`,
    );
  }

  emitLiteralLong(variable: NativeCompilerBuilderVariableID, value: string): void {
    this.writeAssign(
      variable,
      `tsn_int64(${getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context)}, ${value})`,
    );
  }

  emitLiteralDouble(variable: NativeCompilerBuilderVariableID, value: string) {
    value = value.toUpperCase();

    this.writeAssign(
      variable,
      `tsn_double(${getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context)}, ${value})`,
    );
  }

  emitLiteralString(
    variable: NativeCompilerBuilderVariableID,
    value: string,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ) {
    const cString = toCString(value);
    this.appendCheckedInOutFunctionCall(
      variable,
      `tsn_new_string_with_len`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        cString.literalContent,
        cString.length.toString(),
      ],
      exceptionTarget,
    );
  }

  emitLiteralBool(variable: NativeCompilerBuilderVariableID, value: boolean): void {
    this.writeAssign(
      variable,
      `tsn_new_bool(${getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context)}, ${
        value ? 'true' : 'false'
      })`,
    );
  }

  emitUnaryOp(
    operator: NativeCompilerIR.UnaryOperator,
    variable: NativeCompilerBuilderVariableID,
    operand: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    switch (operator) {
      case NativeCompilerIR.UnaryOperator.Neg:
        this.generateMonoOperandOp(variable, operand, `tsn_op_neg`, exceptionTarget);
        break;
      case NativeCompilerIR.UnaryOperator.Plus:
        this.generateMonoOperandOp(variable, operand, `tsn_op_plus`, exceptionTarget);
        break;
      case NativeCompilerIR.UnaryOperator.Inc:
        this.generateMonoOperandOp(variable, operand, `tsn_op_inc`, exceptionTarget);
        break;
      case NativeCompilerIR.UnaryOperator.Dec:
        this.generateMonoOperandOp(variable, operand, `tsn_op_dec`, exceptionTarget);
        break;
      case NativeCompilerIR.UnaryOperator.BitwiseNot:
        this.generateMonoOperandOp(variable, operand, `tsn_op_bnot`, exceptionTarget);
        break;
      case NativeCompilerIR.UnaryOperator.LogicalNot:
        this.generateMonoOperandOp(variable, operand, `tsn_op_lnot`, exceptionTarget);
        break;
      case NativeCompilerIR.UnaryOperator.TypeOf:
        this.generateMonoOperandOp(variable, operand, `tsn_op_typeof`, exceptionTarget);
        break;
      default:
        throw new Error(`Unsupported operator ${operator}`);
    }
  }

  emitBinaryOp(
    variable: NativeCompilerBuilderVariableID,
    operator: NativeCompilerIR.BinaryOperator,
    left: NativeCompilerBuilderVariableID,
    right: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    switch (operator) {
      case NativeCompilerIR.BinaryOperator.Mult:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_mult`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.Add:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_add`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.Sub:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_sub`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.Div:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_div`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.LeftShift:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_ls`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.RightShift:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_rs`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.UnsignedRightShift:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_urs`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.BitwiseOR:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_bor`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.BitwiseXOR:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_bxor`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.BitwiseAND:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_band`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.LessThan:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_lt`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.LessThanOrEqual:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_lte`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.LessThanOrEqualEqual:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_lte_strict`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.GreaterThan:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_gt`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.GreaterThanOrEqual:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_gte`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.GreaterThanOrEqualEqual:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_gte_strict`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.EqualEqual:
        if (this.options.optimizeNullChecks) {
          if (
            right.type === NativeCompilerBuilderVariableType.Undefined ||
            right.type === NativeCompilerBuilderVariableType.Null
          ) {
            return this.generateUndefOrNullCheck(variable, left, `tsn_is_undefined_or_null`);
          } else if (
            left.type === NativeCompilerBuilderVariableType.Undefined ||
            left.type === NativeCompilerBuilderVariableType.Null
          ) {
            return this.generateUndefOrNullCheck(variable, right, `tsn_is_undefined_or_null`);
          }
        }
        this.generateTwoOperandOp(variable, left, right, `tsn_op_eq`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.EqualEqualEqual:
        if (this.options.optimizeNullChecks) {
          if (right.type === NativeCompilerBuilderVariableType.Undefined) {
            return this.generateUndefOrNullCheck(variable, left, `tsn_is_undefined`);
          } else if (left.type === NativeCompilerBuilderVariableType.Undefined) {
            return this.generateUndefOrNullCheck(variable, right, `tsn_is_undefined`);
          } else if (right.type === NativeCompilerBuilderVariableType.Null) {
            return this.generateUndefOrNullCheck(variable, left, `tsn_is_null`);
          } else if (left.type === NativeCompilerBuilderVariableType.Null) {
            return this.generateUndefOrNullCheck(variable, right, `tsn_is_null`);
          }
        }
        this.generateTwoOperandOp(variable, left, right, `tsn_op_eq_strict`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.DifferentThan:
        if (this.options.optimizeNullChecks) {
          if (
            right.type === NativeCompilerBuilderVariableType.Undefined ||
            right.type === NativeCompilerBuilderVariableType.Null
          ) {
            return this.generateUndefOrNullCheck(variable, left, `tsn_not_undefined_or_null`);
          } else if (
            left.type === NativeCompilerBuilderVariableType.Undefined ||
            left.type === NativeCompilerBuilderVariableType.Null
          ) {
            return this.generateUndefOrNullCheck(variable, right, `tsn_not_undefined_or_null`);
          }
        }
        this.generateTwoOperandOp(variable, left, right, `tsn_op_ne`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.DifferentThanStrict:
        if (this.options.optimizeNullChecks) {
          if (right.type === NativeCompilerBuilderVariableType.Undefined) {
            return this.generateUndefOrNullCheck(variable, left, `tsn_not_undefined`);
          } else if (left.type === NativeCompilerBuilderVariableType.Undefined) {
            return this.generateUndefOrNullCheck(variable, right, `tsn_not_undefined`);
          } else if (right.type === NativeCompilerBuilderVariableType.Null) {
            return this.generateUndefOrNullCheck(variable, left, `tsn_not_null`);
          } else if (left.type === NativeCompilerBuilderVariableType.Null) {
            return this.generateUndefOrNullCheck(variable, right, `tsn_not_null`);
          }
        }
        this.generateTwoOperandOp(variable, left, right, `tsn_op_ne_strict`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.Modulo:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_mod`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.Exponentiation:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_exp`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.InstanceOf:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_instanceof`, exceptionTarget);
        return;
      case NativeCompilerIR.BinaryOperator.In:
        this.generateTwoOperandOp(variable, left, right, `tsn_op_in`, exceptionTarget);
        return;
    }
    throw new Error(`Unhandled binary operator type ${operator}`);
  }

  emitNewObject(variable: NativeCompilerBuilderVariableID, exceptionTarget: NativeCompilerBuilderJumpTargetID) {
    this.appendCheckedInOutFunctionCall(
      variable,
      `tsn_new_object`,
      [getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context)],
      exceptionTarget,
    );
  }

  emitNewArray(variable: NativeCompilerBuilderVariableID, exceptionTarget: NativeCompilerBuilderJumpTargetID): void {
    this.appendCheckedInOutFunctionCall(
      variable,
      `tsn_new_array`,
      [getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context)],
      exceptionTarget,
    );
  }

  emitNewArrowFunctionValue(
    variable: NativeCompilerBuilderVariableID,
    functionName: string,
    argc: number,
    closureVariables: NativeCompilerBuilderVariableID[],
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
    stackOnHeap: boolean,
  ): void {
    const stackOnHeapSize = stackOnHeap
      ? [
          `${functionName}_stack_vars_vrefs`,
          `${functionName}_stack_vars_values`,
          `${functionName}_stack_vars_iterators`,
        ]
      : ['0', '0', '0'];
    if (!closureVariables.length) {
      this.appendCheckedInOutFunctionCall(
        variable,
        `tsn_new_arrow_function`,
        [
          getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
          'module',
          `&${functionName}`,
          `${argc}`,
          ...stackOnHeapSize,
        ],
        exceptionTarget,
      );
    } else {
      this.appendCheckedInOutFunctionCall(
        variable,
        `tsn_new_arrow_function_closure`,
        [
          getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
          'module',
          `&${functionName}`,
          `${argc}`,
          ...generateVariadicCallArguments(
            VariadicCallArgumentType.VarRef,
            closureVariables.map(this.getVariableName, this),
          ),
          ...stackOnHeapSize,
        ],
        exceptionTarget,
      );
    }
  }

  emitNewFunctionValue(
    variable: NativeCompilerBuilderVariableID,
    functionName: string,
    constructorName: NativeCompilerBuilderAtomID,
    argc: number,
    closureVariables: NativeCompilerBuilderVariableID[],
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    let tsnFunctionName = 'tsn_new_function';
    const trailingArguments: string[] = [];

    if (closureVariables.length) {
      tsnFunctionName += '_closure';
      trailingArguments.push(
        ...generateVariadicCallArguments(
          VariadicCallArgumentType.VarRef,
          closureVariables.map(this.getVariableName, this),
        ),
      );
    }

    this.appendCheckedInOutFunctionCall(
      variable,
      tsnFunctionName,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        'module',
        this.resolveAtomString(constructorName),
        `&${functionName}`,
        `${argc}`,
      ].concat(...trailingArguments),
      exceptionTarget,
    );
  }

  emitNewClassValue(
    variable: NativeCompilerBuilderVariableID,
    functionName: string,
    constructorName: NativeCompilerBuilderAtomID,
    argc: number,
    parentClass: NativeCompilerBuilderVariableID,
    closureVariables: NativeCompilerBuilderVariableID[],
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    let tsnFunctionName: string;
    const trailingArguments: string[] = [];

    tsnFunctionName = 'tsn_new_class';
    trailingArguments.push(this.getVariableName(parentClass));

    if (closureVariables.length) {
      tsnFunctionName += '_closure';
      trailingArguments.push(
        ...generateVariadicCallArguments(
          VariadicCallArgumentType.VarRef,
          closureVariables.map(this.getVariableName, this),
        ),
      );
    }

    this.appendCheckedInOutFunctionCall(
      variable,
      tsnFunctionName,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        'module',
        this.resolveAtomString(constructorName),
        `&${functionName}`,
        `${argc}`,
      ].concat(...trailingArguments),
      exceptionTarget,
    );
  }

  emitSetClassMethod(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderAtomID,
    method: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedFunctionCall(
      isStatic ? 'tsn_set_class_static_method' : 'tsn_set_class_method',
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(variable),
        this.resolveAtomString(name),
        this.getVariableName(method),
      ],
      exceptionTarget,
    );
  }

  emitSetClassMethodValue(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderVariableID,
    method: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedFunctionCall(
      isStatic ? 'tsn_set_class_static_method_value' : 'tsn_set_class_method_value',
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(variable),
        this.getVariableName(name),
        this.getVariableName(method),
      ],
      exceptionTarget,
    );
  }

  emitSetClassPropertyGetter(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderAtomID,
    getter: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedFunctionCall(
      isStatic ? 'tsn_set_class_static_property_getter' : 'tsn_set_class_property_getter',
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(variable),
        this.resolveAtomString(name),
        this.getVariableName(getter),
      ],
      exceptionTarget,
    );
  }

  emitSetClassPropertyGetterValue(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderVariableID,
    getter: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedFunctionCall(
      isStatic ? 'tsn_set_class_static_property_getter_value' : 'tsn_set_class_property_getter_value',
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(variable),
        this.getVariableName(name),
        this.getVariableName(getter),
      ],
      exceptionTarget,
    );
  }

  emitSetClassPropertySetter(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderAtomID,
    setter: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedFunctionCall(
      isStatic ? 'tsn_set_class_static_property_setter' : 'tsn_set_class_property_setter',
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(variable),
        this.resolveAtomString(name),
        this.getVariableName(setter),
      ],
      exceptionTarget,
    );
  }

  emitSetClassPropertySetterValue(
    variable: NativeCompilerBuilderVariableID,
    name: NativeCompilerBuilderVariableID,
    setter: NativeCompilerBuilderVariableID,
    isStatic: boolean,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedFunctionCall(
      isStatic ? 'tsn_set_class_static_property_getter_value' : 'tsn_set_class_property_getter_value',
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(variable),
        this.getVariableName(name),
        this.getVariableName(setter),
      ],
      exceptionTarget,
    );
  }

  emitNewVariableRef(value: NativeCompilerBuilderVariableID, exceptionTarget: NativeCompilerBuilderJumpTargetID): void {
    this.appendCheckedInOutFunctionCall(
      value,
      `tsn_new_var_ref`,
      [getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context)],
      exceptionTarget,
    );
  }

  emitLoadVariableRef(
    variable: NativeCompilerBuilderVariableID,
    variableRefId: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      variable,
      `tsn_load_var_ref`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(variableRefId),
      ],
      exceptionTarget,
    );
  }

  emitSetVariableRef(value: NativeCompilerBuilderVariableID, variableRefId: NativeCompilerBuilderVariableID): void {
    this.writer.appendFunctionCall(`tsn_set_var_ref`, [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      this.getVariableName(variableRefId),
      this.getVariableName(value),
    ]);
  }

  emitIterator(
    value: NativeCompilerBuilderVariableID,
    output: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      output,
      `tsn_new_iterator`,
      [getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context), this.getVariableName(value)],
      exceptionTarget,
    );
  }

  emitKeysIterator(
    value: NativeCompilerBuilderVariableID,
    output: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      output,
      `tsn_new_keys_iterator`,
      [getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context), this.getVariableName(value)],
      exceptionTarget,
    );
  }

  emitIteratorNext(
    iterator: NativeCompilerBuilderVariableID,
    output: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      output,
      `tsn_iterator_next`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        '&' + this.getVariableName(iterator),
      ],
      exceptionTarget,
    );
  }

  emitKeysIteratorNext(
    iterator: NativeCompilerBuilderVariableID,
    output: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      output,
      `tsn_keys_iterator_next`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        '&' + this.getVariableName(iterator),
      ],
      exceptionTarget,
    );
  }

  emitGenerator(
    arg: NativeCompilerBuilderVariableID,
    output: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      output,
      `tsn_new_generator`,
      [getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context), this.getVariableName(arg)],
      exceptionTarget,
    );
  }

  private retainFunctionName() {
    return this.options.noinlineRetainRelease ? 'tsn_retain' : 'tsn_retain_inline';
  }
  private releaseFunctionName() {
    return this.options.noinlineRetainRelease ? 'tsn_release' : 'tsn_release_inline';
  }

  emitResume(variable: NativeCompilerBuilderVariableID, exceptionTarget: NativeCompilerBuilderJumpTargetID): void {
    const ctx = getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context);
    this.appendCheckedInOutFunctionCall(variable, 'tsn_resume', [ctx, 'closure', 'argc', 'argv'], exceptionTarget);
  }

  emitReentry(resumePoints: NativeCompilerBuilderJumpTargetID[]): void {
    if (resumePoints.length > 0) {
      this.writer.appendWithNewLine('switch(closure->resume_point) {');
      const functionContext = this.functionContext;
      for (const rp of resumePoints) {
        this.writer.appendWithNewLine(`case ${rp.label}: goto ${getJumpTargetName(functionContext.name, rp)};`);
      }
      this.writer.appendWithNewLine('}');
    }
  }

  emitSetResumePoint(resumePoint: NativeCompilerBuilderJumpTargetID): void {
    this.writer.appendWithNewLine(`closure->resume_point = ${resumePoint.label};`);
  }

  emitRetain(value: NativeCompilerBuilderVariableID) {
    // retain always follows assignment
    // we emit a combined retain and assignment call in emitAssignment
  }

  private doEmitRetain(variable: NativeCompilerBuilderVariableID, value: NativeCompilerBuilderVariableID) {
    let functionName: string;
    let byPointer = false;
    switch (value.type) {
      case NativeCompilerBuilderVariableType.VariableRef:
        functionName = 'tsn_retain_var_ref';
        break;
      case NativeCompilerBuilderVariableType.Iterator:
        functionName = 'tsn_retain_iterator';
        byPointer = true;
        break;
      default:
        functionName = this.retainFunctionName();
        break;
    }
    const args = [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      byPointer ? '&' + this.getVariableName(value) : this.getVariableName(value),
    ];
    this.writer.appendAssignmentWithFunctionCall(this.getVariableName(variable), functionName, args);
  }

  emitFree(value: NativeCompilerBuilderVariableID) {
    let functionName: string;
    let byPointer = false;
    switch (value.type) {
      case NativeCompilerBuilderVariableType.VariableRef:
        functionName = 'tsn_release_var_ref';
        break;
      case NativeCompilerBuilderVariableType.Iterator:
        functionName = 'tsn_release_iterator';
        byPointer = true;
        break;
      default:
        functionName = this.releaseFunctionName();
        break;
    }
    const variableName = this.getVariableName(value);
    this.writer.appendFunctionCall(functionName, [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      byPointer ? '&' + variableName : variableName,
    ]);
  }

  emitFreeV(values: NativeCompilerBuilderVariableID[]) {
    const vars = values.filter(
      (v) =>
        v.type != NativeCompilerBuilderVariableType.VariableRef && v.type != NativeCompilerBuilderVariableType.Iterator,
    );
    const varRefs = values.filter((v) => v.type == NativeCompilerBuilderVariableType.VariableRef);
    const iterators = values.filter((v) => v.type == NativeCompilerBuilderVariableType.Iterator);
    const ctx = getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context);
    if (vars.length > 0) {
      this.writer.appendFunctionCall('tsn_release_vars', [
        ctx,
        vars.length.toString(),
        `(tsn_value*[]){${vars.map((v) => '&' + this.getVariableName(v)).join(', ')}}`,
      ]);
    }
    if (varRefs.length > 0) {
      this.writer.appendFunctionCall('tsn_release_var_refs', [
        ctx,
        varRefs.length.toString(),
        `(tsn_var_ref[]){${varRefs.map((v) => this.getVariableName(v)).join(', ')}}`,
      ]);
    }
    if (iterators.length > 0) {
      this.writer.appendFunctionCall('tsn_release_iterators', [
        ctx,
        iterators.length.toString(),
        `(tsn_iterator*[]){${iterators.map((v) => '&' + this.getVariableName(v)).join(', ')}}`,
      ]);
    }
  }

  emitIntrinsicCall(
    variable: NativeCompilerBuilderVariableID,
    func: string,
    args: Array<NativeCompilerBuilderVariableID>,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    const functionContext = this.functionContext;
    let argList;
    if (func.endsWith('_v')) {
      argList = [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        args.length.toString(),
        `(tsn_value[]){${args.map((v) => this.getVariableName(v)).join(', ')}}`,
      ];
    } else {
      argList = [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        ...args.map(this.getVariableName, this),
      ];
    }
    this.writer.appendAssignmentWithFunctionCall(this.getVariableName(variable), func, argList);
    this.writer.beginIfStatement();
    this.writer.appendFunctionCall(
      'tsn_is_exception',
      [getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context), this.getVariableName(variable)],
      false,
    );
    this.writer.endEndifStatement();
    this.writer.beginScope();
    this.writer.appendGoto(getJumpTargetName(functionContext.name, exceptionTarget));
    this.writer.endScope();
  }

  emitFunctionInvocation(
    variable: NativeCompilerBuilderVariableID,
    func: NativeCompilerBuilderVariableID,
    args: Array<NativeCompilerBuilderVariableID>,
    obj: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ) {
    let argList = [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      this.getVariableName(func),
      this.getVariableName(obj),
      ...generateVariadicCallArguments(VariadicCallArgumentType.Value, args.map(this.getVariableName, this)),
    ];

    this.appendCheckedInOutFunctionCall(variable, `tsn_call`, argList, exceptionTarget);
  }

  emitFunctionInvocationWithArgsArray(
    variable: NativeCompilerBuilderVariableID,
    func: NativeCompilerBuilderVariableID,
    args: NativeCompilerBuilderVariableID,
    obj: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      variable,
      `tsn_call_vargs`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(func),
        this.getVariableName(obj),
        this.getVariableName(args),
      ],
      exceptionTarget,
    );
  }

  emitFunctionInvocationWithForwardedArgs(
    variable: NativeCompilerBuilderVariableID,
    func: NativeCompilerBuilderVariableID,
    obj: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      variable,
      `tsn_call_fargs`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(func),
        this.getVariableName(obj),
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Argc),
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Argv),
      ],
      exceptionTarget,
    );
  }

  emitConstructorInvocation(
    variable: NativeCompilerBuilderVariableID,
    func: NativeCompilerBuilderVariableID,
    new_target: NativeCompilerBuilderVariableID,
    args: NativeCompilerBuilderVariableID[],
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    let argList = [
      getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
      this.getVariableName(func),
      this.getVariableName(new_target),
      ...generateVariadicCallArguments(VariadicCallArgumentType.Value, args.map(this.getVariableName, this)),
    ];

    this.appendCheckedInOutFunctionCall(variable, `tsn_call_constructor`, argList, exceptionTarget);
  }

  emitConstructorInvocationWithArgsArray(
    variable: NativeCompilerBuilderVariableID,
    func: NativeCompilerBuilderVariableID,
    new_target: NativeCompilerBuilderVariableID,
    args: NativeCompilerBuilderVariableID,
    exceptionTarget: NativeCompilerBuilderJumpTargetID,
  ): void {
    this.appendCheckedInOutFunctionCall(
      variable,
      `tsn_call_constructor_vargs`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        this.getVariableName(func),
        this.getVariableName(new_target),
        this.getVariableName(args),
      ],
      exceptionTarget,
    );
  }

  emitBindJumpTarget(target: NativeCompilerBuilderJumpTargetID) {
    const functionContext = this.functionContext;
    this.writer.appendLabel(getJumpTargetName(functionContext.name, target));
  }

  emitJump(target: NativeCompilerBuilderJumpTargetID) {
    const functionContext = this.functionContext;
    this.writer.appendGoto(getJumpTargetName(functionContext.name, target));
  }

  emitBranch(
    conditionVariable: NativeCompilerBuilderVariableID,
    type: NativeCompilerBuilderBranchType,
    trueTarget: NativeCompilerBuilderJumpTargetID,
    falseTarget: NativeCompilerBuilderJumpTargetID | undefined,
  ) {
    const functionContext = this.functionContext;

    this.writer.beginIfStatement();
    let functionCallName: string;
    let byPointer = false;
    if (conditionVariable.type === NativeCompilerBuilderVariableType.Iterator) {
      functionCallName = 'tsn_iterator_has_value';
      byPointer = true;
    } else {
      switch (type) {
        case NativeCompilerBuilderBranchType.Truthy:
          if (conditionVariable.type == NativeCompilerBuilderVariableType.Bool) {
            functionCallName = 'tsn_truthy_bool';
          } else {
            functionCallName = 'tsn_to_bool';
          }
          break;
        case NativeCompilerBuilderBranchType.NotUndefinedOrNull:
          functionCallName = 'tsn_not_undefined_or_null';
          break;
      }
    }

    this.writer.appendFunctionCall(
      functionCallName,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context),
        byPointer ? '&' + this.getVariableName(conditionVariable) : this.getVariableName(conditionVariable),
      ],
      false,
    );
    this.writer.endEndifStatement();
    this.writer.beginScope();
    this.writer.appendGoto(getJumpTargetName(functionContext.name, trueTarget));
    this.writer.endScope();

    if (falseTarget !== undefined) {
      this.writer.append(`else`);
      this.writer.beginScope();
      this.writer.appendGoto(getJumpTargetName(functionContext.name, falseTarget));
      this.writer.endScope();
    }
  }

  private writeAssign(target: NativeCompilerBuilderVariableID, inputArgument: string): void {
    this.writer.appendAssignment(this.getVariableName(target), inputArgument);
  }

  emitAssignment(left: NativeCompilerBuilderVariableID, right: NativeCompilerBuilderVariableID) {
    if (right.isRetainable()) {
      this.doEmitRetain(left, right);
    } else {
      this.writeAssign(left, this.getVariableName(right));
    }
  }

  emitThrow(value: NativeCompilerBuilderVariableID, target: NativeCompilerBuilderJumpTargetID): void {
    const functionContext = this.functionContext;
    this.writer.appendFunctionCall(
      `tsn_throw`,
      [getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.Context), this.getVariableName(value)],
      false,
    );
    this.writer.appendWithNewLine(';');
    this.writer.appendGoto(getJumpTargetName(functionContext.name, target));
  }

  emitComments(text: string): void {
    // Escape comment string
    const resolvedText = escapeCComment(text.trim());
    this.writer.beginComment();
    this.writer.appendWithNewLine(resolvedText);
    this.writer.endComment();
  }

  emitProgramCounterInfo(lineNumber: number, columnNumber: number): void {
    this.writer.appendFunctionCall(
      `tsn_set_pc`,
      [
        getNativeCompilerFunctionArgToString(NativeCompilerIR.FunctionTypeArgs.StackFrame),
        lineNumber.toString(),
        columnNumber.toString(),
      ],
      true,
    );
  }

  finalize(): NativeCompilerEmitterOutput {
    this.writer.append('\n__attribute__((constructor)) static void __do_register_module()');
    this.writer.beginScope();
    for (const [modulePath, functionName] of this.pendingRegisterCalls) {
      this.writer.appendFunctionCall(`tsn_register_module`, [toCString(modulePath).literalContent, functionName]);
    }
    this.writer.endScope();
    this.writer.append('\n');

    const source = this.writer.content();
    return { source: source };
  }
}
