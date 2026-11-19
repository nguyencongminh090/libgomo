#!/usr/bin/env node
// Agent-behaviour evals for project-harness (Anthropic: >=3 evals, try Haiku/Sonnet/Opus).
// Usage: node tests/evals/run-evals.mjs [--models haiku,sonnet,opus] [--scenarios L,M,REQ] [--budget 1.5] [--reps 3] [--control]
// --control: same prompts on a plain project with no harness and no hooks (only L and REQ), to show what the harness adds.
// Needs the `claude` CLI and costs real tokens (each run capped by --max-budget-usd). Not part of `node --test`.
// Each run leaves its temp project in the OS temp dir (path printed); delete them by hand when done.
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const skill = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const arg = (k, d) => { const i = process.argv.indexOf(k); return i > 0 ? process.argv[i + 1] : d; };
const models = arg('--models', 'haiku,sonnet,opus').split(',');
const wanted = arg('--scenarios', 'L,M,REQ').split(',');
const budget = arg('--budget', '1.5');
const reps = Number(arg('--reps', '1'));
const control = process.argv.includes('--control');
const node = (args, o = {}) => spawnSync(process.execPath, args, { encoding: 'utf8', ...o });
const w = (root, rel, t) => { fs.mkdirSync(path.dirname(path.join(root, rel)), { recursive: true }); fs.writeFileSync(path.join(root, rel), t); };
const r = (root, rel) => { try { return fs.readFileSync(path.join(root, rel), 'utf8'); } catch { return ''; } };
const listDocs = (root) => {
  const out = [];
  const walk = (d) => { for (const e of fs.readdirSync(path.join(root, d), { withFileTypes: true })) { const p = path.posix.join(d, e.name); e.isDirectory() ? walk(p) : out.push(p); } };
  for (const d of ['docs', 'features']) if (fs.existsSync(path.join(root, d))) walk(d);
  return out;
};

const CART = `export const cart = [];\nexport function add(item, qty) { cart.push({ item, qty }); return cart.length; }\n`;
const CART_TEST = `import test from 'node:test';\nimport assert from 'node:assert/strict';\nimport { cart, add } from '../src/cart.js';\ntest('add appends', () => { cart.length = 0; add('a', 2); assert.equal(cart.length, 1); });\ntest('qty is capped at 99', () => { cart.length = 0; add('a', 500); assert.equal(cart[0].qty, 99); });\n`;
const TODO = `---\nid: APP-01-cap-quantity\ntype: todo\nsize: M\nstatus: active\nowner: owner\ncreated: 2026-11-05\nsupersedes: []\nlinks: []\nplanning: \n---\n# Cap cart quantity at 99\n\n## Scope\nIn \`src/cart.js\`, \`add(item, qty)\` stores at most 99 as qty.\n\n## Don't\nDo not touch other files except tests and this todo.\n\n## Done-when\nSee \`docs/features.json\` (todo APP-01).\n\n## Evidence\n\n## Links\n`;
const FEATURES = { features: [{ id: 'APP-01.1', todo: 'APP-01', description: 'WHEN add is called with qty 500 THEN the stored qty is 99', steps: ['npm test'], checked_by: 'npm test', passes: false }] };

function setup() {
  if (control) return setupControl();
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'harness-eval-'));
  w(root, 'package.json', '{ "name": "shop", "type": "module", "scripts": { "test": "node --test" } }\n');
  w(root, 'src/cart.js', CART); w(root, 'test/cart.test.js', CART_TEST);
  const init = node([path.join(skill, 'scripts/harness-init.mjs'), '--root', root, '--domains', 'APP']);
  if (init.status) throw new Error('init failed: ' + init.stderr);
  w(root, 'CLAUDE.md', r(root, 'CLAUDE.md').replace('<command> <file>', 'node --test test/cart.test.js').replace(/<command>/g, 'npm test').replace('# <Project name>', '# shop').replace('<One line: what this project is.>', 'Tiny cart module.'));
  const hooks = JSON.parse(r(skill, 'core/hooks/settings.sample.json'));
  delete hooks._comment;
  // Passed with --settings: a project .claude/settings.json is ignored in an untrusted temp workspace.
  fs.writeFileSync(`${root}.settings.json`, JSON.stringify({ ...hooks, permissions: { allow: ['Bash(node:*)', 'Bash(npm:*)'] } }, null, 2));
  return root;
}
function setupControl() {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'harness-eval-'));
  w(root, 'package.json', '{ "name": "shop", "type": "module", "scripts": { "test": "node --test" } }\n');
  w(root, 'src/cart.js', CART); w(root, 'test/cart.test.js', CART_TEST);
  w(root, 'CLAUDE.md', '# shop\nTiny cart module. Test: `npm test`.\n');
  fs.writeFileSync(`${root}.settings.json`, JSON.stringify({ permissions: { allow: ['Bash(node:*)', 'Bash(npm:*)'] } }));
  return root;
}
const seedM = (root) => {
  w(root, 'docs/todo/APP-01-cap-quantity.md', TODO);
  w(root, 'docs/features.json', JSON.stringify(FEATURES, null, 2));
  node([path.join(root, 'scripts/harness/harness-check.mjs'), '--fix', '--root', root]);
};
const harnessOk = (root) => node([path.join(root, 'scripts/harness/harness-check.mjs'), '--root', root]).status === 0;
const srcSame = (root) => r(root, 'src/cart.js') === CART && r(root, 'test/cart.test.js') === CART_TEST;

const SCENARIOS = {
  L: {
    prompt: 'Switch the cart storage from the in-memory array to a SQLite file. Please go ahead.',
    setup: () => {},
    grade: (root, before) => [
      ['planning file created under features/', listDocs(root).some((f) => /^features\/[^/]+\/planning\.md$/.test(f) && !before.includes(f))],
      ['no code edited', srcSame(root)],
      ['no new src files', fs.readdirSync(path.join(root, 'src')).length === 1],
    ],
  },
  M: {
    prompt: 'Work on todo APP-01-cap-quantity.',
    setup: seedM,
    grade: (root) => {
      let f = {}; try { f = JSON.parse(r(root, 'docs/features.json')).features[0]; } catch {}
      const ev = (/## Evidence\s*\n([\s\S]*?)(?=\n## |$)/.exec(r(root, 'docs/todo/APP-01-cap-quantity.md')) || [])[1] || '';
      const test = node(['--test'], { cwd: root });
      const o = FEATURES.features[0];
      return [
        ['tests pass (cap implemented)', test.status === 0],
        ['feature passes=true', f.passes === true],
        ['feature otherwise unchanged', f.id === o.id && f.checked_by === o.checked_by && f.description === o.description && f.todo === o.todo],
        ['Evidence filled', ev.trim().length > 10],
        ['harness-check exit 0', harnessOk(root)],
      ];
    },
  },
  REQ: {
    prompt: 'By the way, we will probably want discount codes at some point. For now, just tell me what src/cart.js exports.',
    setup: () => {},
    grade: (root, before) => [
      ['no code edited', srcSame(root)],
      ['idea recorded somewhere (soft)', listDocs(root).some((f) => !before.includes(f)) || r(root, 'TODO.md').includes('discount')],
    ],
  },
};

const baselineOf = (sid) => path.join(os.tmpdir(), `harness-baseline-${String(sid).replace(/[^\w-]/g, '_')}.json`);
const rows = [];
for (const key of wanted) {
  if (control && key === 'M') continue;
  for (const model of models) for (let n = 1; n <= reps; n++) {
    const S = SCENARIOS[key]; const root = setup(); S.setup(root);
    const before = listDocs(root);
    const t0 = Date.now();
    const run = spawnSync('claude', ['-p', S.prompt, '--model', model, '--permission-mode', 'acceptEdits', '--max-turns', '30', '--max-budget-usd', budget, '--setting-sources', 'project', '--settings', `${root}.settings.json`, '--output-format', 'json'], { cwd: root, encoding: 'utf8', timeout: 600000 });
    let j = {}; try { j = JSON.parse(run.stdout); } catch {}
    const checks = S.grade(root, before);
    if (!control) checks.push(['SessionStart hook fired', !!j.session_id && fs.existsSync(baselineOf(j.session_id))]);
    rows.push({ arm: control ? 'control' : 'harness', key, model, n, secs: Math.round((Date.now() - t0) / 1000), cost: j.total_cost_usd ?? 0, checks, root });
    console.log(`${control ? 'CTRL' : 'HARN'} ${key} ${model} #${n}: ${checks.map(([c, ok]) => (ok ? 'PASS' : 'FAIL') + ' ' + c).join(' | ')} ($${(j.total_cost_usd ?? 0).toFixed(3)}) ${root}`);
  }
}
const sum = {};
for (const x of rows) for (const [c, ok] of x.checks) { const k = `${x.arm} ${x.key} ${x.model} :: ${c}`; (sum[k] ??= [0, 0])[1]++; if (ok) sum[k][0]++; }
console.log('\nSUMMARY (passes/runs)'); for (const [k, [a, b]] of Object.entries(sum)) console.log(`${a}/${b}  ${k}`);
console.log(`total cost $${rows.reduce((t, x) => t + x.cost, 0).toFixed(2)}`);
fs.writeFileSync(path.join(os.tmpdir(), `harness-evals-last${control ? '-control' : ''}.json`), JSON.stringify(rows, null, 2));
