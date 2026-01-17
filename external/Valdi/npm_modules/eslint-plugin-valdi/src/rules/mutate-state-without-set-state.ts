import { ESLintUtils } from '@typescript-eslint/utils';

const createRule = ESLintUtils.RuleCreator(name => `https://github.com/Snapchat/Valdi/blob/main/docs/docs/workflow-style-guide.md`);

const rule = createRule({
  name: 'mutate-state-without-set-state',
  meta: {
    type: 'problem',
    docs: {
      description: 'Ensures that clients do not mutate state without calling setState.',
      recommended: 'strict',
    },
    messages: {
      stateAssignment: 'Assignment to this.state will not update the UI. Use setState() instead.',
    },
    schema: [], // no options
    hasSuggestions: true,
  },
  defaultOptions: [],
  create(context) {
    return {
      AssignmentExpression(node) {
        if (node.left != null && node.operator == '=') {
          let range = node.left.range;
          let leftText = context.sourceCode.getText().slice(range[0], range[1]);
          if (/^this\.state\./.test(leftText.toString())) {
            context.report({
              messageId: 'stateAssignment',
              node: node,
              data: {
                message: leftText,
              },
            });
          }
        }
      },
    };
  },
});

export default rule;
