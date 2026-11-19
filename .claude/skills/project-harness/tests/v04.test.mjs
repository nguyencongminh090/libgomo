// 0.3.1: Stop hook ignores errors that existed at session start; a crashing hook warns instead of failing silently.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { skillRoot, tmpCopy, write, read } from './helpers.mjs';

const script = path.join(skillRoot, 'scripts/harness-check.mjs');
const check = (root, flag, input) => spawnSync(process.execPath, [script, flag, '--root', root], { encoding: 'utf8', input: JSON.stringify(input) });
const breakTodo = (root) => write(root, 'docs/todo/WEB-02-cart.md',
  read(root, 'docs/todo/WEB-02-cart.md').replace(/status: \w+/, 'status: done').replace('- [x]', '- [ ]'));
const sid = () => `t${process.pid}-${Math.random().toString(36).slice(2)}`;
const cleanup = (s) => fs.rmSync(path.join(os.tmpdir(), `harness-baseline-${s}.json`), { force: true });

test('--stop does not block on errors that already existed at --session-start', () => {
  const root = tmpCopy('clean'); const s = sid();
  breakTodo(root);
  assert.equal(check(root, '--session-start', { session_id: s }).status, 0);
  assert.equal(check(root, '--stop', { session_id: s }).status, 0);
  cleanup(s);
});
test('--stop still blocks on an error introduced after --session-start', () => {
  const root = tmpCopy('clean'); const s = sid();
  check(root, '--session-start', { session_id: s });
  breakTodo(root);
  const r = check(root, '--stop', { session_id: s });
  assert.equal(r.status, 2, r.stdout + r.stderr);
  assert.match(r.stderr, /new harness error/);
  cleanup(s);
});
test('--stop without a baseline blocks on all errors (hook not wired)', () => {
  const root = tmpCopy('clean');
  breakTodo(root);
  assert.equal(check(root, '--stop', { session_id: sid() }).status, 2);
});
test('a crashing hook (broken profile check) warns via systemMessage and exits 0', () => {
  const root = tmpCopy('clean');
  write(root, 'harness.json', JSON.stringify({ version: '0.3.1', profiles: ['broken'], domains: ['WEB'], overrides: {} }));
  write(root, 'scripts/harness/profile-checks/broken/bad.mjs', 'export const run = (;\n');
  fs.cpSync(path.join(skillRoot, 'scripts'), path.join(root, 'scripts/harness'), { recursive: true, force: false });
  const copy = path.join(root, 'scripts/harness/harness-check.mjs');
  for (const flag of ['--stop', '--session-start']) {
    const r = spawnSync(process.execPath, [copy, flag, '--root', root], { encoding: 'utf8', input: JSON.stringify({ session_id: sid() }) });
    assert.equal(r.status, 0, `${flag}: ${r.stdout}${r.stderr}`);
    assert.match(JSON.parse(r.stdout.trim().split('\n').pop()).systemMessage, /harness hook crashed and was skipped/);
  }
});

test('webapp profile installs a path-scoped frontend rule and an unscoped general rule', async () => {
  const { tmpDir } = await import('./helpers.mjs');
  const root = tmpDir();
  const r = spawnSync(process.execPath, [path.join(skillRoot, 'scripts/harness-init.mjs'), '--root', root, '--profile', 'webapp', '--domains', 'WEB'], { encoding: 'utf8' });
  assert.equal(r.status, 0, r.stderr);
  assert.match(read(root, '.claude/rules/webapp-client.md'), /^---\npaths:\n  - "client\/\*\*"/);
  assert.doesNotMatch(read(root, '.claude/rules/webapp.md'), /^---/);
  assert.equal(spawnSync(process.execPath, [path.join(root, 'scripts/harness/harness-check.mjs'), '--root', root], { encoding: 'utf8' }).status, 0);
});
