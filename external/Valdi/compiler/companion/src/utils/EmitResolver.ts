import * as ts from 'typescript';

export interface SymbolTracker {}

export interface EmitResolver {
  hasGlobalName(name: string): boolean;
  getReferencedExportContainer(
    node: ts.Identifier,
    prefixLocals?: boolean,
  ): ts.SourceFile | ts.ModuleDeclaration | ts.EnumDeclaration | undefined;
  getReferencedImportDeclaration(node: ts.Identifier): ts.Declaration | undefined;
  getReferencedDeclarationWithCollidingName(node: ts.Identifier): ts.Declaration | undefined;
  isDeclarationWithCollidingName(node: ts.Declaration): boolean;
  isValueAliasDeclaration(node: ts.Node): boolean;
  isReferencedAliasDeclaration(node: ts.Node, checkChildren?: boolean): boolean;
  isTopLevelValueImportEqualsWithEntityName(node: ts.ImportEqualsDeclaration): boolean;
  // getNodeCheckFlags(node: Node): NodeCheckFlags;
  // isDeclarationVisible(node: ts.Declaration | AnyImportSyntax): boolean;
  // isLateBound(node: ts.Declaration): node is ts.LateBoundDeclaration;
  collectLinkedAliases(node: ts.Identifier, setVisibility?: boolean): Node[] | undefined;
  isImplementationOfOverload(node: ts.SignatureDeclaration): boolean | undefined;
  isRequiredInitializedParameter(node: ts.ParameterDeclaration): boolean;
  isOptionalUninitializedParameterProperty(node: ts.ParameterDeclaration): boolean;
  isExpandoFunctionDeclaration(node: ts.FunctionDeclaration): boolean;
  getPropertiesOfContainerFunction(node: ts.Declaration): Symbol[];
  createTypeOfDeclaration(
    declaration: ts.AccessorDeclaration | ts.VariableLikeDeclaration | ts.PropertyAccessExpression,
    enclosingDeclaration: Node,
    flags: ts.NodeBuilderFlags,
    tracker: SymbolTracker,
    addUndefined?: boolean,
  ): ts.TypeNode | undefined;
  createReturnTypeOfSignatureDeclaration(
    signatureDeclaration: ts.SignatureDeclaration,
    enclosingDeclaration: Node,
    flags: ts.NodeBuilderFlags,
    tracker: SymbolTracker,
  ): ts.TypeNode | undefined;
  createTypeOfExpression(
    expr: ts.Expression,
    enclosingDeclaration: Node,
    flags: ts.NodeBuilderFlags,
    tracker: SymbolTracker,
  ): ts.TypeNode | undefined;
  createLiteralConstValue(
    node: ts.VariableDeclaration | ts.PropertyDeclaration | ts.PropertySignature | ts.ParameterDeclaration,
    tracker: SymbolTracker,
  ): ts.Expression;
  // isSymbolAccessible(
  //   symbol: Symbol,
  //   enclosingDeclaration: Node | undefined,
  //   meaning: ts.SymbolFlags | undefined,
  //   shouldComputeAliasToMarkVisible: boolean,
  // ): ts.SymbolAccessibilityResult;
  // isEntityNameVisible(
  //   entityName: ts.EntityNameOrEntityNameExpression,
  //   enclosingDeclaration: Node,
  // ): ts.SymbolVisibilityResult;
  // Returns the constant value this property access resolves to, or 'undefined' for a non-constant
  getConstantValue(
    node: ts.EnumMember | ts.PropertyAccessExpression | ts.ElementAccessExpression,
  ): string | number | undefined;
  getReferencedValueDeclaration(reference: ts.Identifier): ts.Declaration | undefined;
  // getTypeReferenceSerializationKind(typeName: ts.EntityName, location?: Node): ts.TypeReferenceSerializationKind;
  isOptionalParameter(node: ts.ParameterDeclaration): boolean;
  moduleExportsSomeValue(moduleReferenceExpression: ts.Expression): boolean;
  isArgumentsLocalBinding(node: ts.Identifier): boolean;
  getExternalModuleFileFromDeclaration(
    declaration:
      | ts.ImportEqualsDeclaration
      | ts.ImportDeclaration
      | ts.ExportDeclaration
      | ts.ModuleDeclaration
      | ts.ImportTypeNode
      | ts.ImportCall,
  ): ts.SourceFile | undefined;
  getTypeReferenceDirectivesForEntityName(name: ts.EntityNameOrEntityNameExpression): string[] | undefined;
  getTypeReferenceDirectivesForSymbol(symbol: Symbol, meaning?: ts.SymbolFlags): string[] | undefined;
  isLiteralConstDeclaration(
    node: ts.VariableDeclaration | ts.PropertyDeclaration | ts.PropertySignature | ts.ParameterDeclaration,
  ): boolean;
  getJsxFactoryEntity(location?: Node): ts.EntityName | undefined;
  getJsxFragmentFactoryEntity(location?: Node): ts.EntityName | undefined;
  // getAllAccessorDeclarations(declaration: ts.AccessorDeclaration): ts.AllAccessorDeclarations;
  getSymbolOfExternalModuleSpecifier(node: ts.StringLiteralLike): Symbol | undefined;
  // isBindingCapturedByNode(node: Node, decl: ts.VariableDeclaration | BindingElement): boolean;
  getDeclarationStatementsForSourceFile(
    node: ts.SourceFile,
    flags: ts.NodeBuilderFlags,
    tracker: SymbolTracker,
    bundled?: boolean,
  ): ts.Statement[] | undefined;
  isImportRequiredByAugmentation(decl: ts.ImportDeclaration): boolean;
}

interface EmitResolverProvider {
  getEmitResolver(sourceFile: ts.SourceFile, cancellationToken: ts.CancellationToken): EmitResolver;
}

export function getEmitResolver(
  typeChecker: ts.TypeChecker,
  sourceFile: ts.SourceFile,
  cancellationToken: ts.CancellationToken | undefined,
): EmitResolver {
  const provider = typeChecker as unknown as EmitResolverProvider;
  if (!provider.getEmitResolver) {
    throw new Error('Could not resolve getEmitResolver() fn');
  }

  return provider.getEmitResolver(sourceFile, cancellationToken as any);
}
