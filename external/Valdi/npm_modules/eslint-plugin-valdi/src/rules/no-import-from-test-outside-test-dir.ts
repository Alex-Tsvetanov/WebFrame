import { ESLintUtils, TSESTree } from '@typescript-eslint/utils';

const createRule = ESLintUtils.RuleCreator(name => `https://github.com/Snapchat/Valdi/blob/main/docs/docs/workflow-style-guide.md`);

const rule = createRule({
  name: 'no-import-from-test-outside-test-dir',
  meta: {
    type: 'problem',
    docs: {
      description: 'Ensures that non-test files do not import from tests directory.',
      recommended: 'strict',
    },
    messages: {
      importTestFromOutside:
        "Import of '{{importPath}}' from non-test directory could result in unexpected behavior between development and production environments. Consider refactoring to avoid.",
    },
    schema: [], // no options
  },
  defaultOptions: [],
  create(context) {
    const isTestFile = context.filename.includes('/test/');

    return {
      ImportDeclaration(node: TSESTree.ImportDeclaration) {
        if (isTestFile) return;

        if (node.source.value.includes('/test/')) {
          context.report({
            node: node,
            messageId: 'importTestFromOutside',
            data: {
              importPath: node.source.value,
            },
          });
        }
      },
    };
  },
});

export default rule;
