#!/usr/bin/env node
// harness-track <CODE> [--root <dir>]  prints the item and, for a todo, its features from docs/features.json
import fs from 'node:fs';
import path from 'node:path';
import { scanItems, codeOf } from './lib/workspace.mjs';
import { loadFeatures, featuresOf, FEATURES_FILE } from './lib/checks/features.mjs';

const argv = process.argv.slice(2);
let root = process.cwd(); let code = '';
for (let i = 0; i < argv.length; i++) {
  if (argv[i] === '--root') root = path.resolve(argv[++i]); else code = argv[i];
}
const items = scanItems(root).filter((i) => codeOf(i));
const want = code.toUpperCase();
const hit = items.find((i) => codeOf(i) === want || String(i.data.id).toUpperCase() === want);
if (hit) {
  process.stdout.write(fs.readFileSync(path.join(root, hit.path), 'utf8'));
  const mine = featuresOf(loadFeatures(root).features, codeOf(hit));
  if (mine.length) {
    process.stdout.write(`\n## Features (${FEATURES_FILE})\n`);
    for (const f of mine) process.stdout.write(`- [${f.passes === true ? 'x' : ' '}] ${f.id} ${f.passes === true ? 'PASS' : 'FAIL'}: ${f.description}${f.checked_by ? ` (checked by: ${f.checked_by})` : ''}\n`);
  }
  process.exit(0);
}
const domain = want.split('-')[0];
const near = items.map(codeOf).filter((c, i, a) => a.indexOf(c) === i)
  .filter((c) => c.startsWith(domain + '-') || c.includes(want)).slice(0, 8);
console.error(`unknown code: ${code || '(none)'}`);
if (near.length) console.error('did you mean: ' + near.join(', '));
process.exit(1);
