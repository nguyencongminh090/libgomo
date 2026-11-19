import { codeOf } from '../workspace.mjs';
const CODED = new Set(['todo', 'design', 'audit', 'adr', 'fix-log']);

export function run(ctx) {
  const seen = new Map();
  const out = [];
  for (const it of ctx.items) {
    if (!CODED.has(it.type)) continue;
    const c = codeOf(it);
    if (!c) continue;
    if (seen.has(c)) {
      out.push({ level: 'error', code: 'CODE-DUP', file: it.path, message: `code ${c} already used by ${seen.get(c)}` });
    } else seen.set(c, it.path);
  }
  return out;
}
