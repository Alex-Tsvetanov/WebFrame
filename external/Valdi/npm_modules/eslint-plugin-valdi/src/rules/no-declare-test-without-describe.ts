import { ESLintUtils, TSESTree } from '@typescript-eslint/utils';

const createRule = ESLintUtils.RuleCreator(
  name => `https://github.com/Snapchat/Valdi/blob/main/docs/docs/workflow-style-guide.md`,
);

const rule = createRule({
  name: 'no-declare-test-without-describe',
  meta: {
    type: 'problem',
    docs: {
      description: 'Ensures that tests are not declared without using describe.',
      recommended: 'strict',
    },
    messages: {
      declaredTestWithoutDescribe:
        'Declaration of test cases without using describe will break CI. Please wrap test cases in a describe block.',
    },
    schema: [], // no options
  },
  defaultOptions: [],
  create(context) {
    const filePath = context.filename;
    const isInTestFolder = filePath.includes('/test/');
    const isTestExtension = filePath.endsWith('.spec.ts') || filePath.endsWith('.spec.tsx');

    if (!(isInTestFolder && isTestExtension)) return {};

    return {
      CallExpression(node: TSESTree.CallExpression) {
        let calleeName: string | undefined;
        if (node.callee.type === 'Identifier') {
          calleeName = node.callee.name;
        } else if (node.callee.type === 'MemberExpression' && node.callee.property.type === 'Identifier') {
          calleeName = node.callee.property.name;
        }

        // Ignore non test case declarations
        if (calleeName !== 'it' && calleeName !== 'valdiIt') {
          return;
        }

        const ancestors = context.sourceCode.getAncestors?.(node);
        const hasDescribe = ancestors
          ? ancestors.some(ancestor => {
              if (ancestor.type === 'CallExpression') {
                // Check if any ancestor of `it` or `valdiIt` is a `describe` call
                const callee = ancestor.callee;
                return callee.type === 'Identifier' && callee.name === 'describe';
              } else if (ancestor.type === 'FunctionDeclaration') {
                return true;
              }

              return false;
            })
          : false;

        if (!hasDescribe) {
          context.report({
            node,
            messageId: 'declaredTestWithoutDescribe',
          });
        }
      },
    };
  },
});

export default rule;
