import { TSESTree, ESLintUtils, AST_NODE_TYPES } from '@typescript-eslint/utils';
import { TypeFormatFlags } from 'typescript';

const createRule = ESLintUtils.RuleCreator(name => `assign-timer-id`);

const rule = createRule({
  name: 'assign-timer-id',
  meta: {
    type: 'problem',
    docs: {
      description: 'Ensures that timer IDs are assigned to an identifier or member so they can be cleaned up.',
      recommended: 'strict',
    },
    messages: {
      assignTimerId: 'Timer id should assign to an identifier or member for cleaning up.',
    },
    schema: [], // no options
  },
  defaultOptions: [],
  create(context) {
    function isZeroDelay(delayValue: TSESTree.CallExpressionArgument) {
      return (
        delayValue === undefined || (delayValue && delayValue.type === 'Literal' && Number(delayValue.value) === 0)
      );
    }

    return {
      CallExpression(node: TSESTree.CallExpression) {
        let calleeName: string | undefined;
        if (node.callee.type === 'Identifier') {
          calleeName = node.callee.name;
        } else if (node.callee.type === 'MemberExpression' && node.callee.property.type === 'Identifier') {
          calleeName = node.callee.property.name;
        }
        if (!calleeName) {
          return;
        }

        const timeoutExpressions = [
          'setTimeout',
          'setTimeoutInterruptible',
          'setTimeoutConfigurable',
          'setTimeoutUninterruptible',
        ];
        const isTimeout = timeoutExpressions.includes(calleeName);
        const isInterval = calleeName === 'setInterval';
        const delayValue = node.arguments[1];

        if (isTimeout && isZeroDelay(delayValue)) {
          return;
        }

        if (isTimeout || isInterval) {
          const parent = node.parent;
          if (parent.type === 'AssignmentExpression' && parent.right === node) {
            return;
          }
          if (parent.type === 'VariableDeclarator' && parent.init === node) {
            return;
          }
          if (parent.type === 'ReturnStatement' && parent.argument === node) {
            return;
          }
          context.report({
            node,
            messageId: 'assignTimerId',
          });
        }
      },
    };
  },
});

export default rule;
