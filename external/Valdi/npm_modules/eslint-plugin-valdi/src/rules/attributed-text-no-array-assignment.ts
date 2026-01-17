import { TSESTree, ESLintUtils, AST_NODE_TYPES } from '@typescript-eslint/utils';
import { TypeFormatFlags } from 'typescript';

const createRule = ESLintUtils.RuleCreator(
  name =>
    `https://github.com/Snapchat/Valdi/blob/main/src/valdi_modules/src/valdi/valdi_core/src/utils/AttributedTextBuilder.ts`,
);

const rule = createRule({
  name: 'attributed-text-no-array-assignment',
  meta: {
    type: 'problem',
    docs: {
      description: "Ensures that the AttributedText type doesn't get directly assigned to an array.",
      recommended: 'strict',
    },
    messages: {
      incorrectInitializer:
        'Variable that resolves to AttributedText should not be initialized as an array. Use AttributedTextBuilder instead.',
      incorrectAssignment:
        'Variable that resolves to AttributedText should not be assigned to an array. Use AttributedTextBuilder instead.',
      incorrectJSXAttribute:
        'JSX attribute that resolves to AttributedText should not be assigned to an array. Use AttributedTextBuilder instead.',
    },
    schema: [], // no options
  },
  defaultOptions: [],
  create(context) {
    const parserServices = ESLintUtils.getParserServices(context);
    const checker = parserServices?.program?.getTypeChecker();

    function getTypesOfNode(node: TSESTree.Node): string[] {
      const tsNode = parserServices.esTreeNodeToTSNodeMap.get(node);
      const type = checker.getTypeAtLocation(tsNode);
      if (type.isUnion()) {
        return type.types.map(type => checker.typeToString(type));
      }
      return [checker.typeToString(type)];
    }

    function getTypesOfJSXAttribute(node: TSESTree.JSXOpeningElement, attributeName: string): string[] {
      const className = node.name;
      if (className.type !== AST_NODE_TYPES.JSXIdentifier) {
        return [];
      }

      const classSymbol = checker.getSymbolAtLocation(parserServices?.esTreeNodeToTSNodeMap.get(className));
      if (!classSymbol) {
        return [];
      }

      const attribute = checker.getDeclaredTypeOfSymbol(classSymbol)?.getProperty(attributeName);
      if (!attribute) {
        return [];
      }

      const attributeType = checker.getDeclaredTypeOfSymbol(attribute);
      if (attributeType.isUnion()) {
        return attributeType.types.map(type => checker.typeToString(type));
      }
      return [checker.typeToString(attributeType)];
    }

    function checkRule(lhsTypes: string[], rhsExpressionType: AST_NODE_TYPES): boolean {
      return lhsTypes.includes('AttributedText') && rhsExpressionType == AST_NODE_TYPES.ArrayExpression;
    }

    return {
      VariableDeclarator(node: TSESTree.VariableDeclarator) {
        if (!node.init) {
          return;
        }

        if (checkRule(getTypesOfNode(node.id), node.init.type)) {
          context.report({
            node,
            messageId: 'incorrectInitializer',
          });
        }
      },

      AssignmentExpression(node: TSESTree.AssignmentExpression) {
        if (checkRule(getTypesOfNode(node.left), node.right.type)) {
          context.report({
            node,
            messageId: 'incorrectAssignment',
          });
        }
      },

      JSXOpeningElement(node: TSESTree.JSXOpeningElement) {
        for (const attribute of node.attributes) {
          if (
            attribute.type !== AST_NODE_TYPES.JSXAttribute ||
            attribute.name.type !== AST_NODE_TYPES.JSXIdentifier ||
            attribute.value?.type !== AST_NODE_TYPES.JSXExpressionContainer
          ) {
            continue;
          }
          if (checkRule(getTypesOfJSXAttribute(node, attribute.name.name), attribute.value.expression.type)) {
            context.report({
              node: attribute,
              messageId: 'incorrectJSXAttribute',
            });
          }
        }
      },
    };
  },
});

export default rule;
