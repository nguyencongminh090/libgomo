import fs from 'node:fs';
import path from 'node:path';

const LEGACY = ['audits', 'instruction', 'instructions', 'todos'];
const ROOT_OK = new Set(['README.md', 'fix-log.md', 'handoff.md', 'features.json']);

export function run(ctx) {
  const out = [];
  const docs = path.join(ctx.root, 'docs');
  if (fs.existsSync(docs)) {
    for (const e of fs.readdirSync(docs, { withFileTypes: true })) {
      if (e.isDirectory() && LEGACY.includes(e.name)) {
        out.push({ level: 'warn', code: 'LEGACY-DIR', file: `docs/${e.name}`, message: 'legacy folder; the standard has no such folder' });
      } else if (e.isFile() && !ROOT_OK.has(e.name)) {
        out.push({ level: 'warn', code: 'DOCS-ROOT-CLUTTER', file: `docs/${e.name}`, message: 'belongs in docs/reports/ or docs/archive/, not the docs root' });
      }
    }
  }
  const claude = path.join(ctx.root, 'CLAUDE.md');
  if (fs.existsSync(claude)) {
    const n = fs.readFileSync(claude, 'utf8').replace(/\n$/, '').split('\n').length;
    const cap = Number(ctx.config.overrides?.claudeMaxLines) || 60;
    if (n > cap) out.push({ level: 'error', code: 'CLAUDE-LONG', file: 'CLAUDE.md', message: `${n} lines, cap is ${cap}` });
  }
  return out;
}
