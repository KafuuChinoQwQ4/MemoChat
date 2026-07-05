import js from '@eslint/js'
import tseslint from 'typescript-eslint'
import reactPlugin from 'eslint-plugin-react'
import reactHooks from 'eslint-plugin-react-hooks'
import reactRefresh from 'eslint-plugin-react-refresh'
import importPlugin from 'eslint-plugin-import'
import boundaries from 'eslint-plugin-boundaries'

export default tseslint.config(
  { ignores: ['dist', 'node_modules', '*.config.*'] },
  js.configs.recommended,
  ...tseslint.configs.recommendedTypeChecked,
  {
    languageOptions: {
      parserOptions: {
        projectService: true,
        tsconfigRootDir: import.meta.dirname,
      },
    },
  },
  {
    plugins: {
      react: reactPlugin,
      'react-hooks': reactHooks,
      'react-refresh': reactRefresh,
      import: importPlugin,
      boundaries,
    },
    settings: {
      react: { version: 'detect' },
      'import/resolver': {
        typescript: { alwaysTryTypes: true },
      },
      'boundaries/elements': [
        { type: 'app',     pattern: 'src/app/**/*' },
        { type: 'core',    pattern: 'src/core/**/*' },
        { type: 'shared',  pattern: 'src/shared/**/*' },
        { type: 'feature', pattern: 'src/features/**/*' },
        { type: 'route',   pattern: 'src/routes/**/*' },
        { type: 'theme',   pattern: 'src/theme/**/*' },
      ],
      'boundaries/ignore': ['src/test/**/*', '**/*.test.*', '**/*.spec.*'],
    },
    rules: {
      // React
      ...reactPlugin.configs.recommended.rules,
      ...reactPlugin.configs['jsx-runtime'].rules,
      ...reactHooks.configs.recommended.rules,
      'react-refresh/only-export-components': ['warn', { allowConstantExport: true }],

      // TS
      '@typescript-eslint/no-unused-vars': ['error', { argsIgnorePattern: '^_' }],
      '@typescript-eslint/no-explicit-any': 'warn',
      '@typescript-eslint/consistent-type-imports': ['error', { prefer: 'type-imports' }],

      // Boundaries — feature modules must not cross-import each other
      'boundaries/element-types': [
        'error',
        {
          default: 'disallow',
          rules: [
            // app can import everything
            { from: 'app',     allow: ['app', 'core', 'shared', 'feature', 'theme'] },
            // route can import feature + shared + core + theme
            { from: 'route',   allow: ['route', 'feature', 'shared', 'core', 'theme', 'app'] },
            // feature can import core + shared + theme only (NOT other features)
            { from: 'feature', allow: ['core', 'shared', 'theme'] },
            // shared can import core + theme only
            { from: 'shared',  allow: ['core', 'theme'] },
            // core is self-contained
            { from: 'core',    allow: ['core'] },
            // theme is self-contained
            { from: 'theme',   allow: [] },
          ],
        },
      ],

      // No React import in api/ or core/ files
      'import/no-restricted-paths': [
        'error',
        {
          zones: [
            // core network cannot import features or shared
            {
              target: './src/core',
              from: './src/features',
              message: 'core/ must not import features/',
            },
            {
              target: './src/core',
              from: './src/shared',
              message: 'core/ must not import shared/',
            },
            // shared cannot import features
            {
              target: './src/shared',
              from: './src/features',
              message: 'shared/ must not import features/',
            },
            // feature components cannot import api/ directly or core/network
            {
              target: './src/features/*/components',
              from: './src/core/network',
              message: 'components/ must not import core/network directly',
            },
          ],
        },
      ],
    },
  },
)
