import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { skillRoot, tmpCopy, tmpDir, write, read } from './helpers.mjs';

const sk = (s, args) => spawnSync(process.execPath, [path.join(skillRoot, 'scripts', s), ...args], { encoding: 'utf8' });
const oldDone = () => {
  const r = tmpCopy('clean');
  return r;
};
const arch = (r, extra = []) => sk('harness-archive.mjs', ['--root', r, '--today', '2026-12-15', '--apply', ...extra]);

test('F1 installed check runs when project path contains a space', () => {
  const base = fs.mkdtempSync(path.join(os.tmpdir(), 'h '));
  const root = path.join(base, 'my proj'); fs.mkdirSync(root);
  sk('harness-init.mjs', ['--root', root]);
  write(root, 'docs/todo/APP-01-x.md', '---\nid: APP-01-x\ntype: todo\nsize: Q\nstatus: zzz\n---\n# x\n');
  const r = spawnSync(process.execPath, [path.join(root, 'scripts/harness/harness-check.mjs'), '--root', root], { encoding: 'utf8' });
  assert.equal(r.status, 1, r.stdout);
  assert.match(r.stdout, /FM-MISSING/);
});
test('F1 check runs through a symlinked script path', () => {
  const root = tmpCopy('broken');
  const link = path.join(tmpDir(), 'hc.mjs');
  fs.symlinkSync(path.join(skillRoot, 'scripts/harness-check.mjs'), link);
  const r = spawnSync(process.execPath, [link, '--root', root], { encoding: 'utf8' });
  assert.equal(r.status, 1);
});
test('F2 archive never overwrites a same-named file; skips and reports', () => {
  const r = tmpCopy('clean');
  for (const d of ['sub', 'other']) {
    write(r, `docs/todo/${d}/APP-03-dup.md`,
      `---\nid: APP-0${d === 'sub' ? 3 : 4}-dup\ntype: todo\nsize: S\nstatus: done\ncreated: 2026-01-01\n---\n# ${d}\n## Done-when\n- [x] ok\n`);
  }
  const out = arch(r);
  assert.match(out.stdout, /name collision/);
  const left = ['docs/todo/sub/APP-03-dup.md', 'docs/todo/other/APP-03-dup.md', 'docs/archive/2026/APP-03-dup.md']
    .filter((p) => fs.existsSync(path.join(r, p)));
  assert.equal(left.length, 2, left.join());
  assert.ok(left.includes('docs/archive/2026/APP-03-dup.md'));
  assert.ok(left.some((p) => p.includes('archive')));
  const all = left.map((p) => read(r, p)).join('');
  assert.match(all, /# sub/); assert.match(all, /# other/);
  const again = arch(r);
  assert.equal(again.status, 0);
});
test('F3 malformed percent escape does not crash check or archive', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-02-cart.md', read(r, 'docs/todo/WEB-02-cart.md') + '\n[bad](%zz.md)\n');
  const c = sk('harness-check.mjs', ['--root', r]);
  assert.doesNotMatch(c.stderr, /URIError/);
  assert.match(c.stdout, /LINK-BROKEN/);
  const a = arch(r);
  assert.doesNotMatch(a.stderr, /URIError/);
  assert.equal(a.status, 0);
});
test('F4 links from reports, other feature files and root README are scanned and rewritten', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/reports/r.md', '[t](../todo/WEB-01-login.md)\n');
  write(r, 'features/feat-x/extra.md', '[t](../../docs/todo/WEB-01-login.md)\n');
  write(r, 'README.md', '[t](docs/todo/WEB-01-login.md)\n');
  write(r, 'docs/reports/broken.md', '[t](nope.md)\n');
  assert.match(sk('harness-check.mjs', ['--root', r]).stdout, /LINK-BROKEN docs\/reports\/broken\.md/);
  fs.rmSync(path.join(r, 'docs/reports/broken.md'));
  assert.equal(arch(r).status, 0);
  const c = sk('harness-check.mjs', ['--root', r]);
  assert.equal(c.status, 0, c.stdout);
  assert.match(read(r, 'README.md'), /docs\/archive\/2026\/WEB-01-login\.md/);
});
test('F5 archive keeps %20 encoding in rewritten links', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/notes/my file.md', '---\nid: n\ntype: note\nsize: S\nstatus: active\n---\n# n\n');
  write(r, 'docs/todo/WEB-01-login.md', read(r, 'docs/todo/WEB-01-login.md') + '\n[sp](../notes/my%20file.md)\n');
  arch(r);
  assert.match(read(r, 'docs/archive/2026/WEB-01-login.md'), /\]\(\.\.\/\.\.\/notes\/my%20file\.md\)/);
});
test('F6 overrides doneNeedsChecks/lNeedsPlanning false are honoured; init honours --override', () => {
  const r = tmpCopy('clean');
  write(r, 'docs/todo/WEB-01-login.md', read(r, 'docs/todo/WEB-01-login.md').replace('- [x] form works', '- [ ] form works'));
  assert.equal(sk('harness-check.mjs', ['--root', r]).status, 1);
  write(r, 'harness.json', JSON.stringify({ version: '0.1.0', domains: ['WEB'], overrides: { doneNeedsChecks: false } }));
  assert.equal(sk('harness-check.mjs', ['--root', r]).status, 0);
  const g = tmpCopy('clean');
  write(g, 'docs/todo/WEB-03-search.md', read(g, 'docs/todo/WEB-03-search.md').replace('size: M', 'size: L').replace('planning: feat-x\n', ''));
  assert.equal(sk('harness-check.mjs', ['--root', g]).status, 1);
  write(g, 'harness.json', JSON.stringify({ version: '0.1.0', domains: ['WEB'], overrides: { lNeedsPlanning: false } }));
  assert.equal(sk('harness-check.mjs', ['--root', g]).status, 0);
});
test('F6 profile weakening works with --override and zero-ish values', () => {
  const s = tmpDir();
  fs.cpSync(skillRoot, s, { recursive: true, filter: (p) => !p.includes(`${path.sep}tests`) });
  write(s, 'profiles/w/profile.json', JSON.stringify({ name: 'w', description: 'd', weakens: ['claudeMaxLines'] }));
  const root = tmpDir();
  const run = (a) => spawnSync(process.execPath, [path.join(s, 'scripts/harness-init.mjs'), '--root', root, '--profile', 'w', ...a], { encoding: 'utf8' });
  assert.equal(run([]).status, 1);
  const ok = run(['--override', 'claudeMaxLines=90']);
  assert.equal(ok.status, 0, ok.stderr);
  assert.equal(JSON.parse(read(root, 'harness.json')).overrides.claudeMaxLines, 90);
});
test('F7 CRLF TODO.md still matches index lines', () => {
  const r = tmpCopy('clean');
  write(r, 'TODO.md', read(r, 'TODO.md').replace(/\n/g, '\r\n'));
  const c = sk('harness-check.mjs', ['--root', r]);
  assert.equal(c.status, 0, c.stdout);
});
