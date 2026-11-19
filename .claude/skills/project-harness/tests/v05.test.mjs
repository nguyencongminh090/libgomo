// 0.3.2: init --hooks, reviewer agent, .gitignore, NotebookEdit gate, session-loop rule, exec-form hooks.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { skillRoot, tmpDir, tmpCopy, write, read } from './helpers.mjs';
import { mergeSettings } from '../scripts/lib/hooks.mjs';

const sample = JSON.parse(read(skillRoot, 'core/hooks/settings.sample.json'));
const init = (root, ...extra) => spawnSync(process.execPath, [path.join(skillRoot, 'scripts/harness-init.mjs'), '--root', root, '--domains', 'APP', ...extra], { encoding: 'utf8' });
const checkOf = (root) => spawnSync(process.execPath, [path.join(root, 'scripts/harness/harness-check.mjs'), '--root', root], { encoding: 'utf8' });

test('sample hooks use exec form, a timeout and the NotebookEdit matcher', () => {
  for (const g of Object.values(sample.hooks).flat()) for (const h of g.hooks) {
    assert.equal(h.command, 'node'); assert.ok(Array.isArray(h.args) && h.args[0].includes('harness-check.mjs')); assert.ok(h.timeout > 0);
  }
  assert.match(sample.hooks.PreToolUse[0].matcher, /NotebookEdit/);
  assert.ok(sample.permissions.deny.includes('Read(./.env)'));
});
test('init --hooks creates .claude/settings.json with the hooks; without the flag it does not', () => {
  const a = tmpDir(); assert.equal(init(a).status, 0);
  assert.equal(fs.existsSync(path.join(a, '.claude/settings.json')), false);
  const b = tmpDir(); const r = init(b, '--hooks'); assert.equal(r.status, 0, r.stderr);
  const j = JSON.parse(read(b, '.claude/settings.json'));
  assert.deepEqual(Object.keys(j.hooks).sort(), ['PreToolUse', 'SessionStart', 'Stop', 'UserPromptSubmit']);
  assert.ok(j.permissions.deny.includes('Read(./secrets/**)'));
  assert.equal(JSON.stringify(j).includes('_comment'), false);
});
test('init --hooks keeps existing settings, adds once, and refuses invalid JSON', () => {
  const root = tmpDir();
  write(root, '.claude/settings.json', JSON.stringify({ model: 'opus', hooks: { Stop: [{ hooks: [{ type: 'command', command: 'echo mine' }] }] }, permissions: { deny: ['Read(./x)'] } }));
  assert.equal(init(root, '--hooks').status, 0);
  const j = JSON.parse(read(root, '.claude/settings.json'));
  assert.equal(j.model, 'opus'); assert.equal(j.hooks.Stop.length, 2); assert.ok(j.permissions.deny.includes('Read(./x)'));
  const before = read(root, '.claude/settings.json');
  assert.equal(init(root, '--hooks').status, 0);
  assert.equal(read(root, '.claude/settings.json'), before);
  const bad = tmpDir(); write(bad, '.claude/settings.json', '{ nope');
  const r = init(bad, '--hooks'); assert.equal(r.status, 1); assert.equal(read(bad, '.claude/settings.json'), '{ nope');
});
test('mergeSettings does not mutate its input', () => {
  const cur = { hooks: {} }; mergeSettings(cur, sample); assert.deepEqual(cur, { hooks: {} });
});
test('init installs the reviewer agent (read-only tools), session-loop rule, gitignore, and the workspace stays clean', () => {
  const root = tmpDir(); init(root);
  const agent = read(root, '.claude/agents/reviewer.md');
  assert.match(agent, /^---\nname: reviewer\n/); assert.match(agent, /tools: Read, Grep, Glob\n/); assert.doesNotMatch(agent, /tools:.*(Edit|Write|Bash)/);
  assert.match(read(root, '.claude/rules/session-loop.md'), /one feature per session/);
  assert.match(read(root, '.gitignore'), /CLAUDE\.local\.md/);
  assert.equal(checkOf(root).status, 0, checkOf(root).stdout);
});
test('init appends to an existing .gitignore once', () => {
  const root = tmpDir(); write(root, '.gitignore', 'node_modules');
  init(root); init(root);
  const lines = read(root, '.gitignore').split('\n').filter(Boolean);
  assert.deepEqual(lines, ['node_modules', 'CLAUDE.local.md', '.claude/settings.local.json']);
});
test('--gate reads notebook_path for NotebookEdit', () => {
  const root = tmpCopy('clean');
  write(root, 'features/feat-x/planning.md', '---\nid: feat-x\ntype: planning\nsize: L\nstatus: draft\nowner: o\ncreated: 2026-11-05\nsupersedes: []\nlinks: []\n---\n# P\n## Open questions\nwhat?\n');
  write(root, 'docs/todo/WEB-09-nb.md', '---\nid: WEB-09-nb\ntype: todo\nsize: M\nstatus: active\nowner: o\ncreated: 2026-11-05\nsupersedes: []\nlinks: []\nplanning: feat-x\n---\n# t\n');
  const run = (tool_input) => spawnSync(process.execPath, [path.join(skillRoot, 'scripts/harness-check.mjs'), '--gate', '--root', root], { encoding: 'utf8', input: JSON.stringify({ tool_name: 'NotebookEdit', tool_input }) });
  assert.equal(run({ notebook_path: path.join(root, 'analysis.ipynb') }).status, 2);
  assert.equal(run({ notebook_path: path.join(root, 'docs/notes/a.ipynb') }).status, 0);
});
test('CLAUDE.md template has a smoke line and stays under the line cap', () => {
  const t = read(skillRoot, 'templates/CLAUDE.md');
  assert.match(t, /Start \/ smoke/); assert.ok(t.split('\n').length <= 50);
});

test('--prompt emits additionalContext under hookSpecificOutput (top level would be ignored)', () => {
  const r = spawnSync(process.execPath, [path.join(skillRoot, 'scripts/harness-check.mjs'), '--prompt', '--root', tmpCopy('clean')], { encoding: 'utf8', input: JSON.stringify({ prompt: 'hi' }) });
  assert.equal(r.status, 0);
  const j = JSON.parse(r.stdout);
  assert.equal(j.additionalContext, undefined);
  assert.equal(j.hookSpecificOutput.hookEventName, 'UserPromptSubmit');
  assert.match(j.hookSpecificOutput.additionalContext, /backlog todo/);
  assert.ok(j.hookSpecificOutput.additionalContext.length < 500);
});
test('sample wires UserPromptSubmit and init --hooks installs it', () => {
  const root = tmpDir(); init(root, '--hooks');
  assert.match(JSON.stringify(JSON.parse(read(root, '.claude/settings.json')).hooks.UserPromptSubmit), /"--prompt"/);
});
