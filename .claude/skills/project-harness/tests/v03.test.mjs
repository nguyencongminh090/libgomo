// v0.3: acceptance in docs/features.json (passes flag), planning is the L unit (no L todos).
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { run as gate } from '../scripts/lib/checks/gate.mjs';
import { run as features } from '../scripts/lib/checks/features.mjs';
import { skillRoot, tmpCopy, tmpDir, write, read, ctxFor } from './helpers.mjs';

const node = (args, opts = {}) => spawnSync(process.execPath, args, { encoding: 'utf8', ...opts });
const sk = (s, args, o = {}) => node([path.join(skillRoot, 'scripts', s), ...args], o);
const has = (fs_, code, level) => fs_.some((f) => f.code === code && (!level || f.level === level));
const FJ = 'docs/features.json';
const PLAN = 'features/feat-x/planning.md';
const T3 = 'docs/todo/WEB-03-search.md';
const getF = (r) => JSON.parse(read(r, FJ));
const putF = (r, j) => write(r, FJ, JSON.stringify(j, null, 2));
const hook = (root, input) => sk('harness-check.mjs', ['--gate', '--root', root], { input: JSON.stringify(input) });

// ---- features.json checks
test('clean fixture with features.json has no findings at all', () => {
  const r = sk('harness-check.mjs', ['--root', tmpCopy('clean')]);
  assert.equal(r.status, 0);
  assert.match(r.stdout, /^0 finding\(s\)/m);
});
test('invalid JSON or wrong shape is FEATURES-INVALID', () => {
  const r = tmpCopy('clean'); write(r, FJ, '{ not json');
  assert.ok(has(features(ctxFor(r)), 'FEATURES-INVALID', 'error'));
  write(r, FJ, '[]');
  assert.ok(has(features(ctxFor(r)), 'FEATURES-INVALID', 'error'));
});
test('missing fields, duplicate ids and unknown todo codes are errors', () => {
  const r = tmpCopy('clean'); const j = getF(r);
  j.features.push({ id: 'WEB-02.1', todo: 'WEB-02', description: 'd', steps: [], passes: false });
  j.features.push({ id: 'WEB-09.1', todo: 'WEB-09', description: 'd', steps: [], passes: false });
  j.features.push({ id: 'WEB-02.2', todo: 'WEB-02', description: 'd', passes: 'yes' });
  putF(r, j);
  const f = features(ctxFor(r));
  assert.ok(has(f, 'FEATURE-DUP', 'error'));
  assert.ok(has(f, 'FEATURE-ORPHAN', 'error'));
  assert.ok(has(f, 'FEATURE-FIELDS', 'error'));
});
test('a feature may point at an archived todo', () => {
  const r = tmpCopy('clean'); const j = getF(r);
  j.features.push({ id: 'WEB-00.1', todo: 'WEB-00', description: 'd', steps: [], passes: true });
  putF(r, j);
  assert.ok(!has(features(ctxFor(r)), 'FEATURE-ORPHAN'));
});
test('done todo with a failing feature is DONE-FEATURE-FAILING; doneNeedsChecks:false switches it off', () => {
  const r = tmpCopy('clean'); const j = getF(r);
  j.features[0].passes = false; putF(r, j);
  assert.ok(has(features(ctxFor(r)), 'DONE-FEATURE-FAILING', 'error'));
  write(r, 'harness.json', JSON.stringify({ version: '0', domains: ['WEB'], overrides: { doneNeedsChecks: false } }));
  assert.ok(!has(features(ctxFor(r)), 'DONE-FEATURE-FAILING'));
});
test('active M todo with no feature warns FEATURE-NONE; missing file means no features', () => {
  const r = tmpCopy('clean'); fs.rmSync(path.join(r, FJ));
  const f = features(ctxFor(r));
  assert.ok(has(f, 'FEATURE-NONE', 'warn'));
  assert.ok(!f.some((x) => x.level === 'error'));
});

test('done M todo with zero features is an error when features.json exists (no vacuous done)', () => {
  const r = tmpCopy('clean'); const j = getF(r);
  j.features = j.features.filter((f) => f.todo !== 'WEB-02'); putF(r, j);
  write(r, 'docs/todo/WEB-02-cart.md', read(r, 'docs/todo/WEB-02-cart.md').replace('status: active', 'status: done').replace('- [ ]', '- [x]'));
  const f = features(ctxFor(r));
  assert.ok(has(f, 'FEATURE-NONE', 'error'), JSON.stringify(f));
  fs.rmSync(path.join(r, FJ));
  assert.ok(has(features(ctxFor(r)), 'FEATURE-NONE', 'warn'), 'legacy project without the file stays at warn');
});

// ---- planning is the L unit
test('a todo of size L is TODO-SIZE-L; SPLIT-TOO-BIG is gone', () => {
  const r = tmpCopy('clean'); write(r, T3, read(r, T3).replace('size: M', 'size: L'));
  const f = gate(ctxFor(r));
  assert.ok(has(f, 'TODO-SIZE-L', 'error'));
  assert.ok(!has(f, 'SPLIT-TOO-BIG'));
});
test('lNeedsPlanning:false allows L todos', () => {
  const r = tmpCopy('clean'); write(r, T3, read(r, T3).replace('size: M', 'size: L').replace('planning: feat-x\n', ''));
  write(r, 'harness.json', JSON.stringify({ version: '0', domains: ['WEB'], overrides: { lNeedsPlanning: false } }));
  assert.ok(!has(gate(ctxFor(r)), 'TODO-SIZE-L'));
});
test('active M todo naming a draft planning fails; backlog one is fine; approved is fine', () => {
  const r = tmpCopy('clean');
  write(r, PLAN, read(r, PLAN).replace('status: approved', 'status: draft'));
  assert.ok(!has(gate(ctxFor(r)), 'GATE-L-NO-PLANNING'), 'backlog may name a draft planning');
  write(r, T3, read(r, T3).replace('status: backlog', 'status: active'));
  assert.ok(has(gate(ctxFor(r)), 'GATE-L-NO-PLANNING', 'error'));
  write(r, PLAN, read(r, PLAN).replace('status: draft', 'status: approved'));
  assert.ok(!has(gate(ctxFor(r)), 'GATE-L-NO-PLANNING'));
});
test('--gate blocks code edits for an active M todo with a draft planning and for an active L todo', () => {
  const r = tmpCopy('clean');
  write(r, PLAN, read(r, PLAN).replace('status: approved', 'status: draft'));
  write(r, T3, read(r, T3).replace('status: backlog', 'status: active'));
  const g = hook(r, { tool_input: { file_path: path.join(r, 'src/a.js') } });
  assert.equal(g.status, 2); assert.match(g.stderr, /GATE-L-NO-PLANNING/);
  const r2 = tmpCopy('clean');
  write(r2, T3, read(r2, T3).replace('size: M', 'size: L').replace('status: backlog', 'status: active').replace('planning: feat-x\n', ''));
  const g2 = hook(r2, { tool_input: { file_path: path.join(r2, 'src/a.js') } });
  assert.equal(g2.status, 2); assert.match(g2.stderr, /TODO-SIZE-L/);
});

// ---- features.json edit guard
const decision = (res) => { try { return JSON.parse(res.stdout).hookSpecificOutput.permissionDecision; } catch { return 'none'; } };
test('editing only passes in features.json is allowed silently', () => {
  const r = tmpCopy('clean'); const file = path.join(r, FJ);
  const res = hook(r, { tool_name: 'Edit', tool_input: { file_path: file, old_string: '"WEB-02.1", "todo": "WEB-02", "description": "x", "passes": false', new_string: '"WEB-02.1", "todo": "WEB-02", "description": "x", "passes": true' } });
  assert.equal(res.status, 0); assert.equal(decision(res), 'none');
});
test('editing anything else in features.json asks the owner', () => {
  const r = tmpCopy('clean'); const file = path.join(r, FJ);
  const e = hook(r, { tool_name: 'Edit', tool_input: { file_path: file, old_string: '"description": "a"', new_string: '"description": "b"' } });
  assert.equal(e.status, 0); assert.equal(decision(e), 'ask');
  const j = getF(r); j.features[1].passes = true;
  const w1 = hook(r, { tool_name: 'Write', tool_input: { file_path: file, content: JSON.stringify(j, null, 2) } });
  assert.equal(decision(w1), 'none', 'Write that only flips passes is allowed');
  j.features.pop();
  const w2 = hook(r, { tool_name: 'Write', tool_input: { file_path: file, content: JSON.stringify(j, null, 2) } });
  assert.equal(decision(w2), 'ask');
  const m = hook(r, { tool_name: 'MultiEdit', tool_input: { file_path: file, edits: [{ old_string: '"passes": false', new_string: '"passes": true' }, { old_string: '"steps": []', new_string: '"steps": ["x"]' }] } });
  assert.equal(decision(m), 'ask');
});

// ---- tooling
test('init creates docs/features.json and a fresh init checks clean (0 findings)', () => {
  const root = tmpDir();
  assert.equal(sk('harness-init.mjs', ['--root', root, '--profile', 'webapp', '--domains', 'WEB']).status, 0);
  assert.deepEqual(JSON.parse(read(root, FJ)), { features: [] });
  const c = sk('harness-check.mjs', ['--root', root]);
  assert.equal(c.status, 0); assert.match(c.stdout, /^0 finding\(s\)/m);
});
test('harness-track prints the todo features; session-start prints passing/total', () => {
  const r = tmpCopy('clean');
  const t = sk('harness-track.mjs', ['WEB-02', '--root', r]);
  assert.match(t.stdout, /WEB-02\.1/); assert.match(t.stdout, /FAIL|passes: false|\[ \]/);
  const s = sk('harness-check.mjs', ['--session-start', '--root', r], { input: '{}' });
  assert.match(s.stdout, /WEB-02[^\n]*0\/1/);
});
test('todo template points Done-when to features.json and keeps Evidence', () => {
  const t = read(skillRoot, 'templates/todo.md');
  assert.match(t, /## Done-when[\s\S]*docs\/features\.json/);
  assert.match(t, /^## Evidence/m);
});
