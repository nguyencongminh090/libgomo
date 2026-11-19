import fs from 'node:fs';
import path from 'node:path';
import { parseIndexLine, indexableTodos } from '../index.mjs';
import { codeOf } from '../workspace.mjs';

export function run(ctx) {
  const out = [];
  const drift = (file, message) => out.push({ level: 'error', code: 'INDEX-DRIFT', file, message });
  const lines = ctx.indexText.split('\n').map(parseIndexLine).filter(Boolean);
  const byCode = new Map(lines.map((l) => [l.code, l]));
  const todos = indexableTodos(ctx.items);
  const todoCodes = new Set(todos.map(codeOf));
  for (const t of todos) {
    const l = byCode.get(codeOf(t));
    if (!l) drift(t.path, `${codeOf(t)} is not in TODO.md`);
    else if (l.status !== t.data.status) drift(t.path, `${codeOf(t)} is ${t.data.status} in the file but ${l.status} in TODO.md`);
  }
  for (const l of lines) {
    if (!fs.existsSync(path.join(ctx.root, l.link))) drift('TODO.md', `${l.code} points to missing file ${l.link}`);
    else if (!todoCodes.has(l.code)) drift('TODO.md', `${l.code} has no matching todo file`);
  }
  return out;
}
