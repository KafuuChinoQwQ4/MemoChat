// Build script: compiles QML runtime TypeScript → QML-compatible .pragma library JS.
//
// QML .pragma library files require ALL functions to be declared at the TOP LEVEL —
// any module wrapper (IIFE, CJS, ESM exports) hides them from QML.
//
// Strategy: use esbuild.transform() per file (type-stripping only, no bundling/wrapping),
// then remove the `export` keyword from top-level declarations so QML sees plain functions.
import * as esbuild from 'esbuild'
import { readFileSync, writeFileSync, readdirSync } from 'fs'
import { join, dirname, relative } from 'path'
import { fileURLToPath } from 'url'

const __dirname = dirname(fileURLToPath(import.meta.url))

// ── Discover .ts runtime files ───────────────────────────────────────────────
function findRuntimeTs(dir) {
  const results = []
  for (const entry of readdirSync(dir, { withFileTypes: true })) {
    const full = join(dir, entry.name)
    if (entry.isDirectory()) {
      results.push(...findRuntimeTs(full))
    } else if (entry.isFile() && entry.name.endsWith('.ts') && full.includes('/runtime/')) {
      results.push(full)
    }
  }
  return results
}

const tsFiles = [
  ...findRuntimeTs(join(__dirname, 'features')),
  ...findRuntimeTs(join(__dirname, 'qml')),
]

// ── Strip `export` from top-level declarations ───────────────────────────────
// QML .pragma library files are not modules — functions/consts must be at top level
// without any export keyword.  We keep the names; only the `export` token is removed.
function makeQmlCompatible(code) {
  return code
    // export function foo(   →  function foo(
    .replace(/^export\s+(function\s)/gm, '$1')
    // export const/let/var   →  const/let/var
    .replace(/^export\s+(const|let|var)\s/gm, '$1 ')
    // export class Foo       →  class Foo
    .replace(/^export\s+(class\s)/gm, '$1')
    // export interface / export type  — esbuild already removes these, but just in case
    .replace(/^export\s+(interface|type)\s[^{]*\{[^}]*\}\s*;?\s*$/gm, '')
    // named re-export stubs: `export { foo, bar };`
    .replace(/^export\s*\{[^}]*\}\s*;?\s*$/gm, '')
    // default export (shouldn't appear, but clean up)
    .replace(/^export\s+default\s+/gm, '')
}

// ── Transform one file ───────────────────────────────────────────────────────
async function buildFile(tsPath) {
  const src = readFileSync(tsPath, 'utf8')

  const result = await esbuild.transform(src, {
    loader: 'ts',
    target: 'es2017',    // downlevel async/await etc., keep readable
    format: 'esm',       // tells esbuild to emit ES syntax (no IIFE wrapper)
    minify: false,
  })

  const qmlCode = makeQmlCompatible(result.code)
  const output = '.pragma library\n' + qmlCode

  const outPath = tsPath.replace(/\.ts$/, '.js')
  writeFileSync(outPath, output, 'utf8')
  return { in: relative(__dirname, tsPath), out: relative(__dirname, outPath) }
}

// ── Main ─────────────────────────────────────────────────────────────────────
const isWatch = process.argv.includes('--watch')

if (isWatch) {
  // Watch mode: rebuild on change via chokidar if available, else poll
  let chokidar
  try { chokidar = await import('chokidar') } catch { chokidar = null }

  const rebuild = async (changedPath) => {
    if (!changedPath.endsWith('.ts')) return
    try {
      const r = await buildFile(changedPath)
      console.log(`[watch] rebuilt ${r.in} → ${r.out}`)
    } catch (e) {
      console.error(`[watch] error building ${changedPath}:`, e.message)
    }
  }

  if (chokidar) {
    chokidar.default.watch(tsFiles).on('change', rebuild)
    console.log(`Watching ${tsFiles.length} TypeScript runtime files...`)
  } else {
    // Fallback: poll every 1s
    const mtimes = new Map(tsFiles.map(f => [f, 0]))
    const { statSync } = await import('fs')
    setInterval(() => {
      for (const f of tsFiles) {
        try {
          const mtime = statSync(f).mtimeMs
          if (mtime !== mtimes.get(f)) { mtimes.set(f, mtime); rebuild(f) }
        } catch {}
      }
    }, 1000)
    console.log(`Polling ${tsFiles.length} TypeScript runtime files (install chokidar for inotify-based watch)...`)
  }
} else {
  // Single build
  const results = await Promise.all(tsFiles.map(buildFile))
  const maxIn  = Math.max(...results.map(r => r.in.length))
  for (const r of results) {
    console.log(`  ${r.in.padEnd(maxIn)}  →  ${r.out}`)
  }
  console.log(`\n⚡ Built ${results.length} files.`)
}
