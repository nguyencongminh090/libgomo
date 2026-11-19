import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { skillRoot, tmpCopy, tmpDir, write, read } from './helpers.mjs';
import { scanItems, nextCode } from '../scripts/lib/workspace.mjs';

const run = (script, args) => spawnSync(process.execPath, [path.join(skillRoot, 'scripts', script), ...args], { encoding: 'utf8' });
const snapshot = (root) => {
  const out = [];
  (function w(d) {
    for (const e of fs.readdirSync(d, { withFileTypes: true })) {
      const p = path.join(d, e.name);
      if (e.isDirectory()) w(p); else out.push(p + ':' + fs.readFileSync(p, 'utf8'));
    }
  })(root);
  return out.sort().join('\n');
};

test('diff reports changed template and writes nothing', () => {
  const root = tmpDir();
  spawnSync(process.execPath, [path.join(skillRoot, 'scripts/harness-init.mjs'), '--root', root]);
  write(root, 'docs/templates/todo.md', 'changed\n');
  fs.rmSync(path.join(root, 'docs/templates/adr.md'));
  const before = snapshot(root);
  const r = run('harness-diff.mjs', ['--root', root]);
  assert.equal(r.status, 0, r.stderr);
  assert.match(r.stdout, /^CHANGED docs\/templates\/todo\.md/m);
  assert.match(r.stdout, /^MISSING docs\/templates\/adr\.md/m);
  assert.equal(snapshot(root), before);
});
test('diff reports version mismatch', () => {
  const root = tmpDir();
  spawnSync(process.execPath, [path.join(skillRoot, 'scripts/harness-init.mjs'), '--root', root]);
  write(root, 'harness.json', JSON.stringify({ version: '0.0.1', profiles: [], domains: ['APP'], overrides: {} }));
  assert.match(run('harness-diff.mjs', ['--root', root]).stdout, new RegExp(`^VERSION installed=0\\.0\\.1 skill=${read(skillRoot, 'VERSION').trim().replace(/\./g, '\\.')}`, 'm'));
});

function oldDone() {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-02-cart.md', read(r, 'docs/todo/WEB-02-cart.md') + '\nSee [login](WEB-01-login.md) and [plan](../../features/feat-x/planning.md).\n');
  write(r, 'docs/todo/WEB-01-login.md', read(r, 'docs/todo/WEB-01-login.md').replace('## Links\nnone', '## Links\n[plan](../../features/feat-x/planning.md) [cart](WEB-02-cart.md)'));
  return r;
}
test('archive dry-run lists candidates and moves nothing', () => {
  const r = oldDone();
  const before = snapshot(r);
  const out = run('harness-archive.mjs', ['--root', r, '--today', '2026-12-15']);
  assert.equal(out.status, 0, out.stderr);
  assert.match(out.stdout, /would archive: docs\/todo\/WEB-01-login\.md -> docs\/archive\/2026\/WEB-01-login\.md/);
  assert.equal(snapshot(r), before);
});
test('archive does not touch items younger than --days', () => {
  const out = run('harness-archive.mjs', ['--root', oldDone(), '--today', '2026-10-05']);
  assert.doesNotMatch(out.stdout, /would archive/);
});
test('archive --apply leaves zero LINK-BROKEN and a passing check', () => {
  const r = oldDone();
  const out = run('harness-archive.mjs', ['--root', r, '--today', '2026-12-15', '--apply']);
  assert.equal(out.status, 0, out.stderr);
  assert.ok(fs.existsSync(path.join(r, 'docs/archive/2026/WEB-01-login.md')));
  assert.ok(!fs.existsSync(path.join(r, 'docs/todo/WEB-01-login.md')));
  assert.match(read(r, 'docs/archive/2026/WEB-01-login.md'), /status: archived/);
  assert.match(read(r, 'docs/archive/2026/WEB-01-login.md'), /\(\.\.\/\.\.\/\.\.\/features\/feat-x\/planning\.md\)/);
  assert.match(read(r, 'docs/todo/WEB-02-cart.md'), /\(\.\.\/archive\/2026\/WEB-01-login\.md\)/);
  const c = run('harness-check.mjs', ['--root', r]);
  assert.equal(c.status, 0, c.stdout);
  assert.doesNotMatch(c.stdout, /LINK-BROKEN/);
});
test('archive keeps code reserved (no CODE-DUP, nextCode skips it)', () => {
  const r = oldDone();
  run('harness-archive.mjs', ['--root', r, '--today', '2026-12-15', '--apply']);
  assert.doesNotMatch(run('harness-check.mjs', ['--root', r]).stdout, /CODE-DUP/);
  assert.equal(nextCode(scanItems(r), 'WEB'), 'WEB-04');
});
test('archive --max-todo archives oldest done items beyond the cap even if recent', () => {
  const r = oldDone();
  const out = run('harness-archive.mjs', ['--root', r, '--today', '2026-10-05', '--max-todo', '2']);
  assert.match(out.stdout, /would archive: docs\/todo\/WEB-01-login\.md/);
});
