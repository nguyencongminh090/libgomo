import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { parseIndexLine, renderIndex } from '../scripts/lib/index.mjs';
import { run as fm } from '../scripts/lib/checks/frontmatter.mjs';
import { run as codes } from '../scripts/lib/checks/codes.mjs';
import { run as index } from '../scripts/lib/checks/index.mjs';
import { run as links } from '../scripts/lib/checks/links.mjs';
import { run as layout } from '../scripts/lib/checks/layout.mjs';
import { ctxFor, tmpCopy, write, read } from './helpers.mjs';

const has = (fs_, code, level) => fs_.some((f) => f.code === code && (!level || f.level === level));
const all = (ctx) => [fm, codes, index, links, layout].flatMap((c) => c(ctx));

test('clean fixture yields zero findings', () => {
  assert.deepEqual(all(ctxFor(tmpCopy('clean'))), []);
});
test('FM-MISSING on broken frontmatter, no crash', () => {
  const f = fm(ctxFor(tmpCopy('broken')));
  assert.equal(f.filter((x) => x.code === 'FM-MISSING' && x.level === 'error').length, 2);
});
test('FM-MISSING when a required key is absent', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-02-cart.md', read(r, 'docs/todo/WEB-02-cart.md').replace('size: M\n', ''));
  assert.ok(has(fm(ctxFor(r)), 'FM-MISSING', 'error'));
});
test('CODE-DUP across active and archived', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-00-again.md', read(r, 'docs/archive/2026/WEB-00-old.md').replace('archived', 'backlog').replace('WEB-00-old', 'WEB-00-again'));
  assert.ok(has(codes(ctxFor(r)), 'CODE-DUP', 'error'));
});
test('INDEX-DRIFT: file not in index', () => {
  const r = tmpCopy('clean');
  write(r, 'TODO.md', '# TODO\n');
  assert.ok(has(index(ctxFor(r)), 'INDEX-DRIFT', 'error'));
});
test('INDEX-DRIFT: index line with missing file', () => {
  const r = tmpCopy('clean');
  fs.rmSync(path.join(r, 'docs/todo/WEB-03-search.md'));
  assert.ok(has(index(ctxFor(r)), 'INDEX-DRIFT', 'error'));
});
test('INDEX-DRIFT: status mismatch', () => {
  const r = tmpCopy('clean');
  write(r, 'TODO.md', read(r, 'TODO.md').replace('- [~] **WEB-02.**', '- [x] **WEB-02.**'));
  assert.ok(has(index(ctxFor(r)), 'INDEX-DRIFT', 'error'));
});
test('LINK-BROKEN', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-02-cart.md', read(r, 'docs/todo/WEB-02-cart.md') + '\nsee [x](../design/nope.md)\n');
  assert.ok(has(links(ctxFor(r)), 'LINK-BROKEN', 'error'));
});
test('links ignore urls, anchors and fenced code', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-02-cart.md', read(r, 'docs/todo/WEB-02-cart.md') + '\n[a](https://x.y) [b](#top)\n```\n[c](missing.md)\n```\n');
  assert.deepEqual(links(ctxFor(r)), []);
});
test('LEGACY-DIR is warn, not error', () => {
  const f = layout(ctxFor(tmpCopy('legacy')));
  assert.equal(f.filter((x) => x.code === 'LEGACY-DIR').length, 2);
  assert.ok(f.every((x) => x.level === 'warn'));
});
test('DOCS-ROOT-CLUTTER warns on non-md and report files in docs root', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/shot.png', 'x'); write(r, 'docs/audit-report.md', '# r');
  assert.equal(layout(ctxFor(r)).filter((x) => x.code === 'DOCS-ROOT-CLUTTER' && x.level === 'warn').length, 2);
});
test('CLAUDE-LONG when over 60 lines, honours override', () => {
  const r = tmpCopy('clean');
  write(r, 'CLAUDE.md', Array.from({ length: 61 }, (_, i) => `line ${i}`).join('\n') + '\n');
  assert.ok(has(layout(ctxFor(r)), 'CLAUDE-LONG', 'error'));
  write(r, 'harness.json', JSON.stringify({ version: '0.1.0', domains: ['WEB'], overrides: { claudeMaxLines: 80 } }));
  assert.ok(!has(layout(ctxFor(r)), 'CLAUDE-LONG'));
});
test('parseIndexLine round-trips renderIndex output', () => {
  const ctx = ctxFor(tmpCopy('clean'));
  const text = renderIndex(ctx.items);
  const parsed = text.split('\n').map(parseIndexLine).filter(Boolean);
  assert.deepEqual(parsed.map((p) => p.code), ['WEB-03', 'WEB-02', 'WEB-01']);
  assert.equal(parsed[0].status, 'backlog');
  assert.equal(parsed[0].size, 'M');
  assert.equal(parsed[0].link, 'docs/todo/WEB-03-search.md');
  assert.equal(parseIndexLine('not an index line'), null);
});

import { run as gate } from '../scripts/lib/checks/gate.mjs';
import { run as done } from '../scripts/lib/checks/done.mjs';

// 0.3.0: the planning is the L unit. A split todo (planning: feat-x) may not start before approval.
const start = (r) => write(r, 'docs/todo/WEB-03-search.md', read(r, 'docs/todo/WEB-03-search.md').replace('status: backlog', 'status: active'));
test('L todo fails TODO-SIZE-L', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-03-search.md', read(r, 'docs/todo/WEB-03-search.md').replace('size: M', 'size: L').replace('planning: feat-x\n', ''));
  assert.ok(has(gate(ctxFor(r)), 'TODO-SIZE-L', 'error'));
});
test('started todo whose planning file is missing fails', () => {
  const r = tmpCopy('clean'); start(r);
  fs.rmSync(path.join(r, 'features'), { recursive: true });
  assert.ok(has(gate(ctxFor(r)), 'GATE-L-NO-PLANNING', 'error'));
});
test('started todo with draft planning fails', () => {
  const r = tmpCopy('clean'); start(r);
  write(r, 'features/feat-x/planning.md', read(r, 'features/feat-x/planning.md').replace('status: approved', 'status: draft'));
  assert.ok(has(gate(ctxFor(r)), 'GATE-L-NO-PLANNING', 'error'));
});
test('started todo with approved planning passes', () => {
  const r = tmpCopy('clean'); start(r);
  assert.ok(!has(gate(ctxFor(r)), 'GATE-L-NO-PLANNING'));
});
test('done todo with unchecked box fails', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-01-login.md', read(r, 'docs/todo/WEB-01-login.md').replace('- [x] form works', '- [ ] form works'));
  assert.ok(has(done(ctxFor(r)), 'DONE-UNCHECKED', 'error'));
});
test('unchecked box outside Done-when does not fail a done todo', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-01-login.md', read(r, 'docs/todo/WEB-01-login.md') + '- [ ] later idea\n');
  assert.ok(!has(done(ctxFor(r)), 'DONE-UNCHECKED'));
});
test('todo split from planning sized L is TODO-SIZE-L (replaces SPLIT-TOO-BIG)', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-03-search.md', read(r, 'docs/todo/WEB-03-search.md').replace('size: M', 'size: L'));
  const f = gate(ctxFor(r));
  assert.ok(has(f, 'TODO-SIZE-L', 'error')); assert.ok(!has(f, 'SPLIT-TOO-BIG'));
});
