import test from 'node:test';
import assert from 'node:assert/strict';
import { parseFrontmatter, stringifyFrontmatter } from '../scripts/lib/frontmatter.mjs';

test('parses flat keys', () => {
  const r = parseFrontmatter('---\nid: WEB-01-x\ntype: todo\n---\nbody\n');
  assert.equal(r.data.id, 'WEB-01-x');
  assert.equal(r.data.type, 'todo');
  assert.equal(r.body, 'body\n');
  assert.equal(r.error, undefined);
});
test('parses [a, b] as array', () => {
  const r = parseFrontmatter('---\nlinks: [a, b]\nempty: []\n---\n');
  assert.deepEqual(r.data.links, ['a', 'b']);
  assert.deepEqual(r.data.empty, []);
});
test('strips quotes', () => {
  const r = parseFrontmatter('---\ntitle: "hello: world"\n---\n');
  assert.equal(r.data.title, 'hello: world');
});
test('no block -> error missing', () => {
  assert.equal(parseFrontmatter('# just text\n').error, 'missing');
});
test('unterminated block -> error unterminated', () => {
  assert.equal(parseFrontmatter('---\nid: a\nno end\n').error, 'unterminated');
});
test('malformed line sets error but keeps parsing', () => {
  const r = parseFrontmatter('---\nid: a\ngarbage line\ntype: todo\n---\n');
  assert.equal(r.error, 'malformed-line');
  assert.equal(r.data.type, 'todo');
});
test('roundtrip keeps key order', () => {
  const data = { id: 'a', type: 'todo', links: ['x', 'y'], size: 'M' };
  const out = stringifyFrontmatter(data);
  const back = parseFrontmatter(out + 'b\n');
  assert.deepEqual(Object.keys(back.data), Object.keys(data));
  assert.deepEqual(back.data, data);
});
test('never throws on binary-looking input', () => {
  const junk = Buffer.from([0, 255, 10, 45, 45, 45, 10, 1, 2, 3]).toString('latin1');
  assert.doesNotThrow(() => parseFrontmatter(junk));
  assert.doesNotThrow(() => parseFrontmatter(''));
  assert.doesNotThrow(() => parseFrontmatter(undefined));
});
