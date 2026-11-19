import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { skillRoot, tmpCopy, write, read } from './helpers.mjs';

const script = (n) => path.join(skillRoot, 'scripts', n);
const node = (args) => spawnSync(process.execPath, args, { encoding: 'utf8' });

test('check exits 0 on clean', () => {
  const r = node([script('harness-check.mjs'), '--root', tmpCopy('clean')]);
  assert.equal(r.status, 0, r.stdout + r.stderr);
});
test('check exits 1 on broken and prints LEVEL CODE file: message', () => {
  const r = node([script('harness-check.mjs'), '--root', tmpCopy('broken')]);
  assert.equal(r.status, 1);
  assert.match(r.stdout, /^ERROR FM-MISSING docs\/todo\/WEB-01-bad\.md: /m);
});
test('check --fix regenerates drifted index then exits 0', () => {
  const root = tmpCopy('clean');
  write(root, 'TODO.md', '# TODO\n');
  assert.equal(node([script('harness-check.mjs'), '--root', root]).status, 1);
  assert.equal(node([script('harness-check.mjs'), '--root', root, '--fix']).status, 0);
  assert.match(read(root, 'TODO.md'), /\*\*WEB-02\.\*\* Cart page/);
  assert.equal(node([script('harness-check.mjs'), '--root', root]).status, 0);
});
test('check --json output parses', () => {
  const r = node([script('harness-check.mjs'), '--root', tmpCopy('broken'), '--json']);
  const f = JSON.parse(r.stdout);
  assert.ok(Array.isArray(f) && f[0].code && f[0].level);
});
test('check on a docs-only tree without harness.json does not crash', () => {
  const root = tmpCopy('legacy');
  fs.rmSync(path.join(root, 'harness.json'));
  const r = node([script('harness-check.mjs'), '--root', root]);
  assert.equal(r.status, 0, r.stdout);
  assert.match(r.stdout, /WARN CONFIG-MISSING/);
});
test('track prints todo body', () => {
  const r = node([script('harness-track.mjs'), 'WEB-02', '--root', tmpCopy('clean')]);
  assert.equal(r.status, 0);
  assert.match(r.stdout, /id: WEB-02-cart/);
  assert.match(r.stdout, /## Done-when/);
});
test('track unknown code exits 1 with did-you-mean', () => {
  const r = node([script('harness-track.mjs'), 'WEB-9', '--root', tmpCopy('clean')]);
  assert.equal(r.status, 1);
  assert.match(r.stderr + r.stdout, /did you mean[\s\S]*WEB-0/i);
});
