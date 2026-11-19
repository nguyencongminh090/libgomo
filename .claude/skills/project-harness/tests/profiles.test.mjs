import test from 'node:test';
import assert from 'node:assert/strict';
import { loadProfile, mergeProfiles } from '../scripts/lib/profiles.mjs';
import { tmpDir, write } from './helpers.mjs';

const base = { description: 'd', when_to_use: 'w', conflicts: [], size_signals: [], definition_of_done: [] };
function skillWith(profiles) {
  const root = tmpDir();
  for (const [name, p] of Object.entries(profiles)) {
    write(root, `profiles/${name}/profile.json`, typeof p === 'string' ? p : JSON.stringify({ name, ...base, ...p }));
  }
  return root;
}
const load = (root, ...names) => names.map((n) => loadProfile(root, n));

test('merge keeps listed order', () => {
  const root = skillWith({ a: { definition_of_done: ['A1'] }, b: { definition_of_done: ['B1'] } });
  assert.deepEqual(mergeProfiles(load(root, 'b', 'a'), {}).definitionOfDone, ['B1', 'A1']);
  assert.deepEqual(mergeProfiles(load(root, 'a', 'b'), {}).definitionOfDone, ['A1', 'B1']);
});
test('stricter size wins (M vs L -> L)', () => {
  const root = skillWith({
    a: { size_signals: [{ pattern: 'public api', size: 'M' }] },
    b: { size_signals: [{ pattern: 'public api', size: 'L' }] },
  });
  for (const order of [['a', 'b'], ['b', 'a']]) {
    const s = mergeProfiles(load(root, ...order), {}).sizeSignals;
    assert.equal(s.length, 1);
    assert.equal(s[0].size, 'L');
  }
});
test('conflicting profiles throw', () => {
  const root = skillWith({ a: { conflicts: ['b'] }, b: {} });
  assert.throws(() => mergeProfiles(load(root, 'a', 'b'), {}), /profile conflict: a vs b/);
  assert.throws(() => mergeProfiles(load(root, 'b', 'a'), {}), /profile conflict/);
});
test('weakening core rule throws unless overridden', () => {
  const root = skillWith({ x: { weakens: ['claudeMaxLines'] } });
  assert.throws(() => mergeProfiles(load(root, 'x'), {}), /profile x weakens core rule claudeMaxLines without override/);
  assert.doesNotThrow(() => mergeProfiles(load(root, 'x'), { claudeMaxLines: 100 }));
});
test('invalid profile.json names the file', () => {
  const root = skillWith({ bad: '{ not json', noname: JSON.stringify({ description: 'x' }) });
  assert.throws(() => loadProfile(root, 'bad'), /profiles[\\/]bad[\\/]profile\.json/);
  assert.throws(() => loadProfile(root, 'noname'), /profiles[\\/]noname[\\/]profile\.json/);
  assert.throws(() => loadProfile(root, 'absent'), /profiles[\\/]absent[\\/]profile\.json/);
});
test('rules and checks are collected from the profile folder', () => {
  const root = skillWith({ a: { checks: ['mycheck'] } });
  write(root, 'profiles/a/rules/r.md', '# r');
  const m = mergeProfiles(load(root, 'a'), {});
  assert.equal(m.rules.length, 1);
  assert.match(m.rules[0], /r\.md$/);
  assert.equal(m.checks.length, 1);
  assert.match(m.checks[0], /checks[\\/]mycheck\.mjs$/);
});
