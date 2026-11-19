import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
export const here = path.dirname(fileURLToPath(import.meta.url));
export const skillRoot = path.resolve(here, '..');
export const fixture = (n) => path.join(here, 'fixtures', n);
export function tmpCopy(name) {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'harness-'));
  fs.cpSync(fixture(name), dir, { recursive: true });
  return dir;
}
export function tmpDir() { return fs.mkdtempSync(path.join(os.tmpdir(), 'harness-')); }
export const read = (root, rel) => fs.readFileSync(path.join(root, rel), 'utf8');
export const write = (root, rel, text) => {
  fs.mkdirSync(path.dirname(path.join(root, rel)), { recursive: true });
  fs.writeFileSync(path.join(root, rel), text);
};
import { loadConfig, scanItems } from '../scripts/lib/workspace.mjs';
export function ctxFor(root) {
  let indexText = '';
  try { indexText = fs.readFileSync(path.join(root, 'TODO.md'), 'utf8'); } catch {}
  return { root, config: loadConfig(root), items: scanItems(root), indexText };
}
