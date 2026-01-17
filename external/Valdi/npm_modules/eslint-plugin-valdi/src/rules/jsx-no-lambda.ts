import { TSESTree, ESLintUtils, AST_NODE_TYPES } from '@typescript-eslint/utils';
import { TypeFormatFlags } from 'typescript';

const createRule = ESLintUtils.RuleCreator(
  name =>
    `https://github.com/Snapchat/Valdi/blob/main/docs/docs/performance-optimization.md#using-callbacks-in-elements`,
);

const rule = createRule({
  name: 'jsx-no-lambda',
  meta: {
    type: 'problem',
    docs: {
      description: "Ensures that lambda functions and arrays aren't rendered directly in JSX",
      recommended: 'strict',
    },
    messages: {
      incorrectLambda: "Avoid assigning a lambda function directly to the '{{attributeName}}' JSX attribute.",
      incorrectArray:
        "Avoid assigning an array directly to the '{{attributeName}}' JSX attribute . Consider storing the array as a member of this component.",
    },
    schema: [], // no options
  },
  defaultOptions: [],
  create(context) {
    return {
      JSXAttribute(node: TSESTree.JSXAttribute) {
        if (node.value?.type !== AST_NODE_TYPES.JSXExpressionContainer) {
          return;
        }
        switch (node.value.expression.type) {
          case AST_NODE_TYPES.ArrowFunctionExpression:
          case AST_NODE_TYPES.FunctionExpression:
            context.report({
              node: node.value,
              messageId: 'incorrectLambda',
              data: {
                attributeName: node.name.name,
              },
            });
            break;
          case AST_NODE_TYPES.ArrayExpression:
            context.report({
              node: node.value,
              messageId: 'incorrectArray',
              data: {
                attributeName: node.name.name,
              },
            });
            break;
        }
      },
    };
  },
});

export default rule;
