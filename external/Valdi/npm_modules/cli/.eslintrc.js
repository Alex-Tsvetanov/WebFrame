module.exports = {
  root: true,
  parser: '@typescript-eslint/parser',
  parserOptions: {
    tsconfigRootDir: __dirname,
    project: ['./tsconfig.json'],
  },
  plugins: ['@typescript-eslint', 'import'],
  extends: [
    'eslint:recommended',
    'plugin:@typescript-eslint/recommended',
    'plugin:@typescript-eslint/recommended-requiring-type-checking',
    'plugin:import/recommended',
    'plugin:import/typescript',
    'plugin:unicorn/recommended',
    'prettier',
  ],
  rules: {
    eqeqeq: 'error',
    '@typescript-eslint/consistent-type-assertions': [
      'warn',
      {
        assertionStyle: 'as',
        objectLiteralTypeAssertions: 'never',
      },
    ],
    '@typescript-eslint/consistent-type-imports': [
      'error',
      {
        prefer: 'type-imports',
      },
    ],
    '@typescript-eslint/member-ordering': 'warn',
    '@typescript-eslint/no-inferrable-types': ['error', { ignoreParameters: true }],
    '@typescript-eslint/prefer-ts-expect-error': 'error',
    '@typescript-eslint/no-floating-promises': ['error', { ignoreVoid: true }],
    '@typescript-eslint/no-misused-promises': [
      'error',
      {
        checksVoidReturn: false,
      },
    ],
    '@typescript-eslint/no-empty-function': 'off',
    'import/default': 'off',
    'import/namespace': 'off',
    'import/no-unresolved': 'off',
    'import/no-duplicates': 'error',
    'import/no-named-as-default': 'error',
    'import/order': ['warn', { alphabetize: { order: 'asc' } }],
    'unicorn/filename-case': [
      'error',
      {
        cases: {
          camelCase: true,
          pascalCase: true,
        },
        ignore: ['^.*CLI.*'],
      },
    ],
    'unicorn/no-array-for-each': 'off',
    'unicorn/no-array-reduce': 'off',
    'unicorn/no-for-loop': 'off',
    'unicorn/no-null': 'off',
    'unicorn/prefer-node-protocol': 'off',
    'unicorn/prefer-query-selector': 'off',
    'unicorn/prefer-ternary': 'off',
    'unicorn/prefer-top-level-await': 'off',
    'unicorn/prefer-spread': 'off',
    'unicorn/prevent-abbreviations': 'off',
    'sort-imports': ['error', { allowSeparatedGroups: true, ignoreDeclarationSort: true }],
  },
};
