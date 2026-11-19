import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { loadProfile, mergeProfiles } from '../scripts/lib/profiles.mjs';
import { skillRoot, tmpDir, tmpCopy, write, read } from './helpers.mjs';

const SEEDS = ['webapp', 'software-app', 'engine-perf', 'module-architecture', 'library', 'ai-implementation'];
const node = (args, opts = {}) => spawnSync(process.execPath, args, { encoding: 'utf8', ...opts });

test('every seed profile loads and has a rule file', () => {
  for (const s of SEEDS) {
    const p = loadProfile(skillRoot, s);
    assert.equal(p.name, s);
    assert.ok(p.rules.length >= 1, s);
    assert.ok(p.when_to_use, s);
  }
  for (const s of ['library', 'ai-implementation']) assert.equal(loadProfile(skillRoot, s).confidence, 'low');
});
test('all pairs merge without throwing except declared conflicts', () => {
  for (const a of SEEDS) for (const b of SEEDS) {
    if (a >= b) continue;
    const pa = loadProfile(skillRoot, a); const pb = loadProfile(skillRoot, b);
    if (pa.conflicts.includes(b) || pb.conflicts.includes(a)) assert.throws(() => mergeProfiles([pa, pb], {}));
    else assert.doesNotThrow(() => mergeProfiles([pa, pb], {}), `${a}+${b}`);
  }
});
test('seed signals: library API change is L; ai prompt change is M with eval run', () => {
  const lib = mergeProfiles([loadProfile(skillRoot, 'library')], {});
  assert.ok(lib.sizeSignals.some((s) => /public API/i.test(s.pattern) && s.size === 'L'));
  const ai = mergeProfiles([loadProfile(skillRoot, 'ai-implementation')], {});
  assert.ok(ai.sizeSignals.some((s) => /prompt/i.test(s.pattern) && s.size === 'M'));
  assert.ok(ai.definitionOfDone.includes('eval run recorded'));
});
for (const s of SEEDS) {
  test(`init profile ${s} into a temp dir then harness-check exits 0`, () => {
    const root = tmpDir();
    const i = node([path.join(skillRoot, 'scripts/harness-init.mjs'), '--root', root, '--profile', s]);
    assert.equal(i.status, 0, i.stderr);
    assert.ok(fs.readdirSync(path.join(root, '.claude/rules')).length >= 6);
    const c = node([path.join(root, 'scripts/harness/harness-check.mjs'), '--root', root]);
    assert.equal(c.status, 0, c.stdout);
  });
}
test('core rules mark behavioural rules advisory', () => {
  for (const f of ['task-size', 'bugfix', 'spike', 'handoff', 'tracking-files']) {
    assert.ok(fs.existsSync(path.join(skillRoot, 'core/rules', f + '.md')), f);
  }
  assert.match(read(skillRoot, 'core/rules/spike.md'), /advisory/i);
  assert.match(read(skillRoot, 'core/rules/task-size.md'), /record it, do not build/i);
});
test('hook sample runs check on Stop and a gate on PreToolUse', () => {
  const j = JSON.parse(read(skillRoot, 'core/hooks/settings.sample.json'));
  assert.match(JSON.stringify(j.hooks.Stop), /harness-check/);
  assert.match(JSON.stringify(j.hooks.PreToolUse), /harness-check\.mjs.*"--gate"/);
});
test('--gate blocks code edits while an active split todo has an unapproved planning, allows docs edits', () => {
  const root = tmpCopy('clean');
  write(root, 'docs/todo/WEB-03-search.md', read(root, 'docs/todo/WEB-03-search.md').replace('status: backlog', 'status: active'));
  write(root, 'features/feat-x/planning.md', read(root, 'features/feat-x/planning.md').replace('approved', 'draft'));
  const gate = (file) => node([path.join(skillRoot, 'scripts/harness-check.mjs'), '--gate', '--root', root],
    { input: JSON.stringify({ tool_input: { file_path: path.join(root, file) } }) });
  assert.equal(gate('src/app.js').status, 2);
  assert.match(gate('src/app.js').stderr, /GATE-L-NO-PLANNING/);
  assert.equal(gate('features/feat-x/planning.md').status, 0);
  assert.equal(gate('docs/todo/WEB-03-search.md').status, 0);
  write(root, 'features/feat-x/planning.md', read(root, 'features/feat-x/planning.md').replace('draft', 'approved'));
  assert.equal(gate('src/app.js').status, 0);
});
