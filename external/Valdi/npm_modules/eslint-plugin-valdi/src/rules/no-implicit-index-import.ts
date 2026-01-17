import { TSESTree, ESLintUtils } from '@typescript-eslint/utils';

const createRule = ESLintUtils.RuleCreator(name => `https://github.com/Snapchat/Valdi/blob/main/docs/docs/workflow-style-guide.md`);

const rule = createRule({
  name: 'no-implicit-index-import',
  meta: {
    type: 'problem',
    docs: {
      description:
        "Ensures that imports of index files are explicit. Valdi module loader doesn't resolve implicit index so we disallow them to prevent errors at runtime.",
      recommended: 'strict',
    },
    messages: {
      implicitIndexImport: "Import of '{{importPath}}' should be explicit, not implicit, append /index",
      changeToExplicitImport: "Change to explicit import '{{importPath}}/index'",
    },
    schema: [], // no options
    hasSuggestions: true,
  },
  defaultOptions: [],
  create(context) {
    return {
      ImportDeclaration(node: TSESTree.ImportDeclaration) {
        if (node.source.value.endsWith('/src')) {
          context.report({
            node: node,
            messageId: 'implicitIndexImport',
            data: {
              importPath: node.source.value,
            },
            suggest: [
              {
                messageId: 'changeToExplicitImport',
                data: {
                  importPath: node.source.value,
                },
                fix: fixer => {
                  return fixer.replaceText(node.source, `'${node.source.value}/index'`);
                },
              },
            ],
          });
        }
      },
    };
  },
});

export default rule;
