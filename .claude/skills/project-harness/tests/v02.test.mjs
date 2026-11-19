// v0.2 improvements: Stop hook that really blocks, open-questions check, SessionStart summary, templates.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { run as gate } from '../scripts/lib/checks/gate.mjs';
import { skillRoot, tmpCopy, write, read, ctxFor } from './helpers.mjs';

const node = (args, opts = {}) => spawnSync(process.execPath, args, { encoding: 'utf8', ...opts });
const check = (root, flag, input = '{}') => node([path.join(skillRoot, 'scripts/harness-check.mjs'), flag, '--root', root], { input });
const has = (fs_, code, level) => fs_.some((f) => f.code === code && (!level || f.level === level));
const PLAN = 'features/feat-x/planning.md';
const breakTodo = (root) => write(root, 'docs/todo/WEB-02-cart.md',
  read(root, 'docs/todo/WEB-02-cart.md').replace(/status: \w+/, 'status: done').replace('- [x]', '- [ ]'));

test('--stop exits 2 with findings on stderr when there are errors', () => {
  const root = tmpCopy('clean');
  breakTodo(root);
  const r = check(root, '--stop', JSON.stringify({ stop_hook_active: false }));
  assert.equal(r.status, 2, r.stdout + r.stderr);
  assert.match(r.stderr, /ERROR/);
});
test('--stop exits 0 on a clean workspace', () => {
  assert.equal(check(tmpCopy('clean'), '--stop').status, 0);
});
test('--stop does not block again when stop_hook_active is true (loop guard)', () => {
  const root = tmpCopy('clean');
  breakTodo(root);
  assert.equal(check(root, '--stop', JSON.stringify({ stop_hook_active: true })).status, 0);
});
test('hook sample uses --stop on Stop and --session-start on SessionStart', () => {
  const j = JSON.parse(read(skillRoot, 'core/hooks/settings.sample.json'));
  assert.match(JSON.stringify(j.hooks.Stop), /harness-check\.mjs.*"--stop"/);
  assert.match(JSON.stringify(j.hooks.SessionStart), /harness-check\.mjs.*"--session-start"/);
});

test('approved planning with open questions listed is PLANNING-OPEN and does not pass the L gate', () => {
  const root = tmpCopy('clean');
  write(root, PLAN, read(root, PLAN) + '- which cache do we use?\n');
  write(root, 'docs/todo/WEB-03-search.md', read(root, 'docs/todo/WEB-03-search.md').replace('status: backlog', 'status: active'));
  const f = gate(ctxFor(root));
  assert.ok(has(f, 'PLANNING-OPEN', 'error'));
  assert.ok(has(f, 'GATE-L-NO-PLANNING', 'error'));
});
test('approved planning with a NEEDS CLARIFICATION marker is PLANNING-OPEN', () => {
  const root = tmpCopy('clean');
  write(root, PLAN, read(root, PLAN).replace('## Constraints\nc', '## Constraints\n[NEEDS CLARIFICATION: max latency?]'));
  assert.ok(has(gate(ctxFor(root)), 'PLANNING-OPEN', 'error'));
});
test('draft planning may hold open questions; template placeholder is not a question', () => {
  const root = tmpCopy('clean');
  write(root, PLAN, read(root, PLAN).replace('approved', 'draft') + '- open one\n[NEEDS CLARIFICATION: x]\n');
  assert.ok(!has(gate(ctxFor(root)), 'PLANNING-OPEN'));
  const r2 = tmpCopy('clean');
  write(r2, PLAN, read(r2, PLAN) + '(optional: list of questions; empty when approved)\n');
  assert.ok(!has(gate(ctxFor(r2)), 'PLANNING-OPEN'));
});

test('a planning copied from the shipped template and approved passes (no PLANNING-OPEN, gate open)', () => {
  const root = tmpCopy('clean');
  const t = fs.readFileSync(path.join(skillRoot, 'templates', 'planning.md'), 'utf8')
    .replace('id: <slug>', 'id: feat-x').replace('status: draft', 'status: approved');
  write(root, PLAN, t);
  write(root, 'docs/todo/WEB-03-search.md', read(root, 'docs/todo/WEB-03-search.md').replace('status: backlog', 'status: active'));
  const f = gate(ctxFor(root));
  assert.ok(!has(f, 'PLANNING-OPEN'), JSON.stringify(f));
  assert.ok(!has(f, 'GATE-L-NO-PLANNING'), JSON.stringify(f));
});
test('a marker inside backticks is documentation, not an open question', () => {
  const root = tmpCopy('clean');
  write(root, PLAN, read(root, PLAN).replace('## Constraints\nc', '## Constraints\nUse `[NEEDS CLARIFICATION: x]` for gaps.'));
  assert.ok(!has(gate(ctxFor(root)), 'PLANNING-OPEN'));
});

test('--session-start prints handoff, active todos and finding counts, exit 0', () => {
  const root = tmpCopy('clean');
  write(root, 'docs/handoff.md', '---\nid: handoff\ntype: handoff\nsize: S\nstatus: active\nowner: x\ncreated: 2026-11-05\nsupersedes: []\nlinks: []\n---\n# Handoff\n## Doing now\nwiring cart\n## Next step\nadd tests\n## Uncertain\n## Blockers\n');
  write(root, 'docs/todo/WEB-03-search.md', read(root, 'docs/todo/WEB-03-search.md').replace(/status: \w+/, 'status: active'));
  breakTodo(root);
  const r = check(root, '--session-start');
  assert.equal(r.status, 0);
  assert.match(r.stdout, /wiring cart/);
  assert.match(r.stdout, /add tests/);
  assert.match(r.stdout, /WEB-03/);
  assert.match(r.stdout, /[1-9]\d* error\(s\)/);
  assert.match(r.stdout, /DONE-UNCHECKED/);
});
test('--session-start works without a handoff file', () => {
  const r = check(tmpCopy('clean'), '--session-start');
  assert.equal(r.status, 0);
  assert.match(r.stdout, /no handoff/i);
});

const tpl = (n) => fs.readFileSync(path.join(skillRoot, 'templates', n + '.md'), 'utf8');
test('CLAUDE.md template puts a Commands slot first and points to the size rule instead of repeating it', () => {
  const t = tpl('CLAUDE');
  assert.ok(t.indexOf('## Commands') > -1 && t.indexOf('## Commands') < t.indexOf('## Hard rules'));
  assert.match(t, /single test/i);
  assert.match(t, /\.claude\/rules\/task-size\.md/);
  assert.match(t, /@AGENTS\.md/);
  assert.ok(t.split('\n').length <= 50);
});
test('todo template: WHEN/THEN acceptance with checked_by (in features.json since 0.3.0), and an Evidence section', () => {
  const t = tpl('todo');
  assert.match(t, /WHEN .+ THEN .+checked_by/);
  assert.match(t, /^## Evidence/m);
});
test('planning template documents the NEEDS CLARIFICATION marker', () => {
  assert.match(tpl('planning'), /\[NEEDS CLARIFICATION: /);
});
