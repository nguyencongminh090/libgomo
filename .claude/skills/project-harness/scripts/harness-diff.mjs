#!/usr/bin/env node
// harness-diff [--root <dir>] [--skill <path>]   Report only; never writes.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadConfig } from './lib/workspace.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
let root = process.cwd(); let skill = path.resolve(here, '..');
const argv = process.argv.slice(2);
for (let i = 0; i < argv.length; i++) {
  if (argv[i] === '--root') root = path.resolve(argv[++i]);
  else if (argv[i] === '--skill') skill = path.resolve(argv[++i]);
}
if (!fs.existsSync(path.join(skill, 'VERSION'))) {
  console.error(`not a project-harness skill folder: ${skill} (pass --skill <path>)`);
  process.exit(1);
}
const lines = [];
const skillVersion = fs.readFileSync(path.join(skill, 'VERSION'), 'utf8').trim();
try {
  const cfg = loadConfig(root);
  if (cfg.version !== skillVersion) lines.push(`VERSION installed=${cfg.version} skill=${skillVersion}`);
} catch (e) {
  lines.push(fs.existsSync(path.join(root, 'harness.json')) ? `INVALID harness.json: ${e.message}` : 'MISSING harness.json');
}

function compare(srcDir, destRel, skip = []) {
  if (!fs.existsSync(srcDir)) return;
  for (const f of fs.readdirSync(srcDir, { withFileTypes: true })) {
    if (!f.isFile() || skip.includes(f.name)) continue;
    const dest = path.join(root, destRel, f.name);
    const rel = `${destRel}/${f.name}`;
    if (!fs.existsSync(dest)) lines.push(`MISSING ${rel}`);
    else if (!fs.readFileSync(dest).equals(fs.readFileSync(path.join(srcDir, f.name)))) lines.push(`CHANGED ${rel}`);
  }
}
compare(path.join(skill, 'templates'), 'docs/templates', ['CLAUDE.md']);
compare(path.join(skill, 'core', 'rules'), '.claude/rules');
compare(path.join(skill, 'scripts'), 'scripts/harness');
compare(path.join(skill, 'scripts', 'lib'), 'scripts/harness/lib');
compare(path.join(skill, 'scripts', 'lib', 'checks'), 'scripts/harness/lib/checks');
for (const l of lines) console.log(l);
console.log(lines.length ? `${lines.length} difference(s); nothing was changed` : 'no differences');
