import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { skillRoot, tmpDir, write, read } from './helpers.mjs';

const init = (args) => spawnSync(process.execPath, [path.join(skillRoot, 'scripts/harness-init.mjs'), ...args], { encoding: 'utf8' });
const check = (root) => spawnSync(process.execPath, [path.join(root, 'scripts/harness/harness-check.mjs'), '--root', root], { encoding: 'utf8' });

test('init creates expected tree and check passes (installed copy)', () => {
  const root = tmpDir();
  const r = init(['--root', root, '--domains', 'WEB,API']);
  assert.equal(r.status, 0, r.stderr);
  for (const p of ['CLAUDE.md', 'TODO.md', 'harness.json', 'docs/README.md', 'docs/handoff.md', 'docs/fix-log.md',
    'docs/templates/todo.md', 'docs/templates/planning.md', 'docs/todo/.gitkeep', 'docs/archive/.gitkeep',
    'docs/reports/.gitkeep', 'features/.gitkeep', 'scripts/harness/harness-check.mjs', 'scripts/harness/lib/frontmatter.mjs']) {
    assert.ok(fs.existsSync(path.join(root, p)), p);
  }
  const cfg = JSON.parse(read(root, 'harness.json'));
  assert.equal(cfg.version, read(skillRoot, 'VERSION').trim());
  assert.deepEqual(cfg.domains, ['WEB', 'API']);
  const c = check(root);
  assert.equal(c.status, 0, c.stdout + c.stderr);
});
test('init twice skips and reports, never overwrites', () => {
  const root = tmpDir();
  init(['--root', root]);
  write(root, 'TODO.md', '# my edits\n');
  const r = init(['--root', root]);
  assert.equal(r.status, 0);
  assert.match(r.stdout, /skipped: TODO\.md/);
  assert.equal(read(root, 'TODO.md'), '# my edits\n');
});
test('init keeps existing CLAUDE.md', () => {
  const root = tmpDir();
  write(root, 'CLAUDE.md', 'mine\n');
  const r = init(['--root', root]);
  assert.match(r.stdout, /skipped: CLAUDE\.md/);
  assert.equal(read(root, 'CLAUDE.md'), 'mine\n');
});
function fakeSkill(profiles) {
  const s = tmpDir();
  fs.cpSync(skillRoot, s, { recursive: true, filter: (p) => !p.includes('tests') && !p.includes(`${path.sep}profiles${path.sep}`) });
  for (const [n, p] of Object.entries(profiles)) {
    write(s, `profiles/${n}/profile.json`, JSON.stringify({ name: n, description: 'd', ...p }));
  }
  return s;
}
test('init with conflicting profiles exits 1 and writes nothing', () => {
  const s = fakeSkill({ a: { conflicts: ['b'] }, b: {} });
  const root = tmpDir();
  const r = spawnSync(process.execPath, [path.join(s, 'scripts/harness-init.mjs'), '--root', root, '--profile', 'a,b'], { encoding: 'utf8' });
  assert.equal(r.status, 1);
  assert.match(r.stderr, /profile conflict: a vs b/);
  assert.deepEqual(fs.readdirSync(root), []);
});
test('init installs profile rules and its checks run in the installed harness-check', () => {
  const s = fakeSkill({ p: { checks: ['always'] } });
  write(s, 'profiles/p/rules/p-rule.md', '# p rule\n');
  write(s, 'profiles/p/checks/always.mjs', "export function run() { return [{ level: 'warn', code: 'P-ALWAYS', file: 'x', message: 'hi' }]; }\n");
  const root = tmpDir();
  const r = spawnSync(process.execPath, [path.join(s, 'scripts/harness-init.mjs'), '--root', root, '--profile', 'p'], { encoding: 'utf8' });
  assert.equal(r.status, 0, r.stderr);
  assert.ok(fs.existsSync(path.join(root, '.claude/rules/p-rule.md')));
  assert.deepEqual(JSON.parse(read(root, 'harness.json')).profiles, ['p']);
  assert.match(check(root).stdout, /WARN P-ALWAYS/);
});
test('--new-profile writes draft from answers, refuses to overwrite', () => {
  const s = fakeSkill({});
  const ans = path.join(tmpDir(), 'a.json');
  fs.writeFileSync(ans, JSON.stringify({ risks: ['data loss'], irreversible: ['schema change'], done_means: 'users can export', verification: 'run export on sample data' }));
  const run = () => spawnSync(process.execPath, [path.join(s, 'scripts/harness-init.mjs'), '--new-profile', 'etl', '--answers', ans], { encoding: 'utf8' });
  assert.equal(run().status, 0);
  const p = JSON.parse(read(s, 'profiles/etl/profile.json'));
  assert.equal(p.status, 'draft');
  assert.equal(p.name, 'etl');
  assert.ok(p.size_signals.some((x) => x.pattern === 'schema change' && x.size === 'L'));
  assert.ok(p.definition_of_done.some((x) => /export/.test(x)));
  assert.equal(run().status, 1);
});
