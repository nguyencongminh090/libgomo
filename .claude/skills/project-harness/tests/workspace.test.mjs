import test from 'node:test';
import assert from 'node:assert/strict';
import { loadConfig, scanItems, nextCode } from '../scripts/lib/workspace.mjs';
import { fixture, tmpDir, write } from './helpers.mjs';

test('scanItems finds todo+archive+planning in clean', () => {
  const items = scanItems(fixture('clean'));
  const paths = items.map((i) => i.path).sort();
  assert.deepEqual(paths, [
    'docs/archive/2026/WEB-00-old.md', 'docs/todo/WEB-01-login.md', 'docs/todo/WEB-02-cart.md',
    'docs/todo/WEB-03-search.md', 'features/feat-x/planning.md']);
  assert.equal(items.find((i) => i.path.includes('feat-x')).type, 'planning');
  assert.equal(items.find((i) => i.path.includes('WEB-00')).archived, true);
});
test('scanItems records error not throw in broken', () => {
  const items = scanItems(fixture('broken'));
  assert.equal(items.length, 2);
  assert.ok(items.every((i) => i.error));
  assert.ok(items.every((i) => i.type === 'todo'));
});
test('nextCode counts archived codes', () => {
  assert.equal(nextCode(scanItems(fixture('clean')), 'WEB'), 'WEB-04');
});
test('nextCode starts at 01', () => {
  assert.equal(nextCode([], 'API'), 'API-01');
});
test('loadConfig reads harness.json and throws when absent', () => {
  assert.equal(loadConfig(fixture('clean')).domains[0], 'WEB');
  assert.throws(() => loadConfig(tmpDir()), /harness\.json not found/);
});
test('scanItems 1500 generated files under 3s', () => {
  const root = tmpDir();
  for (let i = 0; i < 1500; i++) {
    write(root, `docs/archive/2026/WEB-${i}-x.md`, `---\nid: WEB-${i}-x\ntype: todo\nstatus: archived\n---\n# t\n`);
  }
  write(root, 'docs/shot.png', 'x'); write(root, 'docs/data.json', '{}');
  const t = Date.now();
  const items = scanItems(root);
  assert.equal(items.length, 1500);
  assert.ok(Date.now() - t < 3000);
});
