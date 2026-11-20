import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { skillRoot, tmpCopy, tmpDir, write, read } from './helpers.mjs';

const sk = (s, args, o = {}) => spawnSync(process.execPath, [path.join(skillRoot, 'scripts', s), ...args], { encoding: 'utf8', ...o });
const gate = (root, file) => sk('harness-check.mjs', ['--gate', '--root', root], { input: JSON.stringify({ tool_input: { file_path: path.join(root, file) } }) });

test('M8 malformed frontmatter line does not switch the gate off', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-03-search.md', read(r, 'docs/todo/WEB-03-search.md')
    .replace('size: M', 'size: L').replace('status: backlog', 'status: active\ngarbage line').replace('planning: feat-x\n', ''));
  assert.equal(gate(r, 'src/a.js').status, 2);
  assert.match(sk('harness-check.mjs', ['--root', r]).stdout, /TODO-SIZE-L/);
});
test('M9 profile names with path parts are rejected', () => {
  const root = tmpDir();
  const r = sk('harness-init.mjs', ['--root', root, '--profile', '../x']);
  assert.equal(r.status, 1);
  assert.match(r.stderr, /invalid profile name/);
  const ans = path.join(tmpDir(), 'a.json'); fs.writeFileSync(ans, '{}');
  const n = sk('harness-init.mjs', ['--new-profile', '../../evil', '--answers', ans]);
  assert.equal(n.status, 1);
  assert.match(n.stderr, /invalid profile name/);
});
test('M10 --fix skips a todo without size instead of writing "size undefined"', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-02-cart.md', read(r, 'docs/todo/WEB-02-cart.md').replace('size: M\n', ''));
  sk('harness-check.mjs', ['--root', r, '--fix']);
  assert.doesNotMatch(read(r, 'TODO.md'), /undefined/);
  assert.match(sk('harness-check.mjs', ['--root', r]).stdout, /FM-MISSING docs\/todo\/WEB-02-cart\.md/);
});
test('M11 titled, query and reference-style links are checked', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/notes/n.md', '---\nid: n\ntype: note\nsize: S\nstatus: active\n---\n# n\n[a](missing1.md "title")\n[b](missing2.md?x=1)\n[c]: missing3.md\n[ok](../todo/WEB-01-login.md "fine")\n');
  const out = sk('harness-check.mjs', ['--root', r]).stdout;
  for (const m of ['missing1', 'missing2', 'missing3']) assert.match(out, new RegExp(`LINK-BROKEN docs/notes/n\\.md: .*${m}`));
  assert.doesNotMatch(out, /WEB-01-login/);
});
test('M11 archive rewrites titled and reference-style links', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/notes/n.md', '---\nid: n\ntype: note\nsize: S\nstatus: active\n---\n# n\n[ok](../todo/WEB-01-login.md "t")\n[ref]: ../todo/WEB-01-login.md\n');
  sk('harness-archive.mjs', ['--root', r, '--today', '2026-12-15', '--apply']);
  const t = read(r, 'docs/notes/n.md');
  assert.match(t, /\(\.\.\/archive\/2026\/WEB-01-login\.md "t"\)/);
  assert.match(t, /\[ref\]: \.\.\/archive\/2026\/WEB-01-login\.md/);
  assert.equal(sk('harness-check.mjs', ['--root', r]).status, 0);
});
test('M12 diff says INVALID for malformed harness.json, MISSING only when absent', () => {
  const r = tmpDir();
  write(r, 'harness.json', '{ nope');
  assert.match(sk('harness-diff.mjs', ['--root', r]).stdout, /^INVALID harness\.json/m);
  fs.rmSync(path.join(r, 'harness.json'));
  assert.match(sk('harness-diff.mjs', ['--root', r]).stdout, /^MISSING harness\.json/m);
});

import { loadProfileChecks, invalidProfileNames } from '../scripts/harness-check.mjs';
const withProfile = (name, body = 'export function run(){return []}') => {
  const base = tmpDir();
  write(base, `profile-checks/${name}/a.mjs`, body);
  return base;
};
test('HARNESS-01-name invalid profile names are never imported and are reported', async () => {
  assert.deepEqual(invalidProfileNames(['ok-1', '../x', 'A', 'a/b', '-a', 5]), ['../x', 'A', 'a/b', '-a', 5]);
  const base = tmpDir();
  write(base, 'profile-checks/ok/a.mjs', 'export function run(){return []}');
  write(base, 'evil.mjs', 'globalThis.__pwned = true;');
  const mods = await loadProfileChecks(['../evil', 'ok'], path.join(base, 'scripts'));
  assert.equal(globalThis.__pwned, undefined);
  assert.equal(mods.length, 0); // base differs, "ok" not under scripts/
  const r = tmpCopy('clean');
  write(r, 'harness.json', JSON.stringify({ version: '0', profiles: ['../x'], domains: [], overrides: {} }));
  const out = sk('harness-check.mjs', ['--root', r, '--json']);
  assert.ok(JSON.parse(out.stdout).some((f) => f.code === 'PROFILE-NAME' && f.level === 'error'));
});
test('HARNESS-01-root a profile directory symlinked outside its root is skipped', async () => {
  const base = tmpDir();
  const outside = tmpDir();
  write(outside, 'a.mjs', 'globalThis.__escaped = true; export function run(){return []}');
  fs.mkdirSync(path.join(base, 'profile-checks'));
  fs.symlinkSync(outside, path.join(base, 'profile-checks', 'link'));
  assert.equal((await loadProfileChecks(['link'], base)).length, 0);
  assert.equal(globalThis.__escaped, undefined);
  const good = withProfile('fine');
  assert.equal((await loadProfileChecks(['fine'], good)).length, 1);
});
