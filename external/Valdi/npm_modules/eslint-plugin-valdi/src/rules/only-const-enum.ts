import { TSESTree, ESLintUtils, AST_NODE_TYPES } from '@typescript-eslint/utils';
import { TypeFormatFlags } from 'typescript';

const createRule = ESLintUtils.RuleCreator(name => `https://github.com/Snapchat/Valdi/blob/main/docs/docs/workflow-style-guide.md`);

const rule = createRule({
  name: 'only-const-enum',
  meta: {
    type: 'problem',
    docs: {
      description: 'Ensures that enums are only declared as const enums. Only applies to d.ts or d.tsx files.',
      recommended: 'strict',
    },
    messages: {
      incorrectQualifier:
        "Enum '{{enumName}}' must be declared as a const enum in a d.ts or d.tsx file. If you are seeing this error outside of these files, please contact the Valdi team.",
    },
    schema: [], // no options
  },
  defaultOptions: [],
  create(context) {
    return {
      TSEnumDeclaration(node: TSESTree.TSEnumDeclaration) {
        if (!node.const) {
          context.report({
            node: node,
            messageId: 'incorrectQualifier',
            data: {
              enumName: node.id.name,
            },
          });
        }
      },
    };
  },
});

export default rule;
