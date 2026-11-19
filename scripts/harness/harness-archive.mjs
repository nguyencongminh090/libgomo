#!/usr/bin/env node
// harness-archive [--root] [--days 30] [--max-todo 40] [--today YYYY-MM-DD] [--apply]
// Dry run by default. Age uses frontmatter `done` date if present, else `created`.
import fs from 'node:fs';
import path from 'node:path';
import { scanItems, codeOf } from './lib/workspace.mjs';
import { renderIndex } from './lib/index.mjs';
import { filesToScan, safeDecode, INLINE_LINK, REF_LINK } from './lib/checks/links.mjs';

const a = { root: process.cwd(), days: 30, maxTodo: 40, apply: false, today: new Date().toISOString().slice(0, 10) };
const argv = process.argv.slice(2);
for (let i = 0; i < argv.length; i++) {
  const k = argv[i];
  if (k === '--root') a.root = path.resolve(argv[++i]);
  else if (k === '--days') a.days = Number(argv[++i]);
  else if (k === '--max-todo') a.maxTodo = Number(argv[++i]);
  else if (k === '--today') a.today = argv[++i];
  else if (k === '--apply') a.apply = true;
}
const DAY = 86400000;
const dateOf = (it) => {
  const s = String(it.data.done || it.data.created || '');
  return /^\d{4}-\d{2}-\d{2}$/.test(s) ? s : null;
};
const age = (s) => (Date.parse(a.today) - Date.parse(s)) / DAY;

const items = scanItems(a.root);
const live = items.filter((i) => i.type === 'todo' && !i.archived && !i.error);
const done = live.filter((i) => i.data.status === 'done' && dateOf(i)).sort((x, y) => dateOf(x).localeCompare(dateOf(y)));
const picked = new Set(done.filter((i) => age(dateOf(i)) > a.days));
let remaining = live.length - picked.size;
for (const i of done) {
  if (remaining <= a.maxTodo) break;
  if (!picked.has(i)) { picked.add(i); remaining--; }
}
const moves = new Map(); // abs old -> abs new
const taken = new Set();
for (const i of [...picked]) {
  const year = dateOf(i).slice(0, 4);
  const dest = path.join(a.root, 'docs', 'archive', year, path.basename(i.path));
  if (taken.has(dest) || fs.existsSync(dest)) {
    console.log(`skipped: ${i.path} (name collision with ${path.relative(a.root, dest).split(path.sep).join('/')})`);
    picked.delete(i);
    continue;
  }
  taken.add(dest);
  moves.set(path.join(a.root, i.path), dest);
}
const rel = (p) => path.relative(a.root, p).split(path.sep).join('/');
for (const [from, to] of moves) console.log(`${a.apply ? 'archive' : 'would archive'}: ${rel(from)} -> ${rel(to)}`);
if (!moves.size) console.log('nothing to archive');
if (!a.apply || !moves.size) process.exit(0);

const EXTERNAL = /^([a-z][a-z0-9+.-]*:|#|\/)/i;
function rewrite(text, oldDir, newDir) {
  let fenced = false;
  const fix = (m, pre, target, post) => {
    if (EXTERNAL.test(target)) return m;
    const m2 = /^([^#?]*)(.*)$/.exec(target);
    const p = m2[1];
    if (!p) return m;
    const abs = path.resolve(oldDir, safeDecode(p));
    const moved = moves.get(abs);
    if (!moved && !fs.existsSync(abs)) return m;
    let next = path.relative(newDir, moved ?? abs).split(path.sep).join('/');
    if (p.includes('%')) next = encodeURI(next);
    return pre + next + m2[2] + post;
  };
  return text.split('\n').map((line) => {
    if (/^\s*(```|~~~)/.test(line)) { fenced = !fenced; return line; }
    if (fenced) return line;
    return line.split(/(`[^`]*`)/).map((seg, idx) => {
      if (idx % 2) return seg;
      const ref = REF_LINK.exec(seg);
      if (ref) return fix(seg, ref[1], ref[2], ref[3]);
      return seg.replace(INLINE_LINK, fix);
    }).join('');
  }).join('\n');
}

const files = [...new Set([...filesToScan({ root: a.root, items }), 'TODO.md'].map((f) => path.join(a.root, f)))].filter((f) => fs.existsSync(f));
for (const f of files) {
  const newF = moves.get(f) ?? f;
  const text = fs.readFileSync(f, 'utf8');
  let out = rewrite(text, path.dirname(f), path.dirname(newF));
  if (moves.has(f)) out = out.replace(/^status:.*$/m, 'status: archived');
  if (newF !== f) {
    fs.mkdirSync(path.dirname(newF), { recursive: true });
    fs.writeFileSync(newF, out);
    fs.rmSync(f);
  } else if (out !== text) fs.writeFileSync(f, out);
}
fs.writeFileSync(path.join(a.root, 'TODO.md'), renderIndex(scanItems(a.root)));
console.log(`archived ${moves.size} item(s); codes stay reserved: ${[...picked].map(codeOf).join(', ')}`);
