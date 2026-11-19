import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { parseFrontmatter } from '../scripts/lib/frontmatter.mjs';
import { skillRoot } from './helpers.mjs';

const KEYS = ['id', 'type', 'size', 'status', 'owner', 'created', 'supersedes', 'links'];
const SECTIONS = {
  note: ['What', 'Outcome'],
  planning: ['Goal', 'Non-goals', 'Constraints', 'Options', 'Decision', 'Open questions'],
  todo: ['Scope', "Don't", 'Traps', 'Done-when', 'Links'],
  design: ['Intent', 'Structure', 'Interfaces', 'Trade-offs'],
  audit: ['Context', 'Decision', 'Status', 'Supersedes'],
  adr: ['Context', 'Decision', 'Consequences'],
  'fix-log': ['Symptom', 'First fix', 'Real source', 'Corrected fix', 'Check that would have caught it'],
  handoff: ['Doing now', 'Next step', 'Uncertain', 'Blockers'],
};
const tpl = (n) => fs.readFileSync(path.join(skillRoot, 'templates', n + '.md'), 'utf8');

test('every template has required frontmatter keys', () => {
  for (const name of Object.keys(SECTIONS)) {
    const fm = parseFrontmatter(tpl(name));
    assert.equal(fm.error, undefined, name);
    for (const k of KEYS) assert.ok(k in fm.data, `${name} lacks ${k}`);
    assert.equal(fm.data.type, name);
    if (name === 'todo') assert.ok('planning' in fm.data, 'todo lacks planning');
  }
});
test('required sections present', () => {
  for (const [name, secs] of Object.entries(SECTIONS)) {
    const body = tpl(name);
    for (const s of secs) assert.match(body, new RegExp(`^## ${s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}`, 'm'), `${name}: ${s}`);
  }
});
test('todo Done-when points to features.json; planning states open-questions rule', () => {
  assert.match(tpl('todo'), /## Done-when[\s\S]*features\.json[\s\S]*"passes": false/);
  assert.match(tpl('planning'), /Open questions must be empty to approve/);
});
// v0.2: the size table lives only in core/rules/task-size.md (installed to .claude/rules/, loaded at
// launch); repeating it in CLAUDE.md doubled the context cost. CLAUDE.md keeps a one-line summary + pointer.
test('CLAUDE.md template <= 60 lines, sizes summarised with pointer, record-dont-build', () => {
  const t = tpl('CLAUDE');
  assert.ok(t.split('\n').length <= 60);
  assert.match(t, /S \/ M \/ L/); assert.match(t, /task-size\.md/);
  assert.match(fs.readFileSync(path.join(skillRoot, 'core/rules/task-size.md'), 'utf8'), /\| L \|/);
  assert.match(t, /do not build/i);
  assert.match(t, /advisory/i);
});
