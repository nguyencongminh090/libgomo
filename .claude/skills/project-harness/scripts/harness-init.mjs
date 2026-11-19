#!/usr/bin/env node
// harness-init --root <dir> [--profile a,b] [--domains X,Y] [--hooks]
// harness-init --new-profile <name> --answers <file.json>
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadProfile, mergeProfiles, assertProfileName } from './lib/profiles.mjs';
import { renderIndex } from './lib/index.mjs';
import { mergeSettings } from './lib/hooks.mjs';

const scriptsDir = path.dirname(fileURLToPath(import.meta.url));
const skillRoot = path.resolve(scriptsDir, '..');

function parseArgs(argv) {
  const a = { root: process.cwd(), profile: [], domains: [], overrides: {} };
  for (let i = 0; i < argv.length; i++) {
    const k = argv[i];
    if (k === '--root') a.root = path.resolve(argv[++i]);
    else if (k === '--profile') a.profile = argv[++i].split(',').filter(Boolean);
    else if (k === '--domains') a.domains = argv[++i].split(',').filter(Boolean);
    else if (k === '--override') {
      const [key, ...v] = argv[++i].split('=');
      const raw = v.join('=');
      try { a.overrides[key] = JSON.parse(raw); } catch { a.overrides[key] = raw; }
    }
    else if (k === '--hooks') a.hooks = true;
    else if (k === '--new-profile') a.newProfile = argv[++i];
    else if (k === '--answers') a.answers = argv[++i];
  }
  return a;
}
const list = (v) => (Array.isArray(v) ? v : v ? [v] : []).map(String);

function newProfile(a) {
  assertProfileName(a.newProfile);
  if (!a.answers) throw new Error('--new-profile needs --answers <file.json>');
  const dir = path.join(skillRoot, 'profiles', a.newProfile);
  const file = path.join(dir, 'profile.json');
  if (fs.existsSync(file)) throw new Error(`profile already exists: ${file}`);
  const ans = JSON.parse(fs.readFileSync(a.answers, 'utf8'));
  const profile = {
    name: a.newProfile,
    description: `Draft profile for ${a.newProfile} projects (from the init interview)`,
    when_to_use: list(ans.risks).length ? `Main risks: ${list(ans.risks).join('; ')}` : '',
    conflicts: [],
    size_signals: [
      ...list(ans.irreversible).map((pattern) => ({ pattern, size: 'L' })),
      ...list(ans.risks).map((pattern) => ({ pattern, size: 'M' })),
    ],
    definition_of_done: [...list(ans.done_means), ...list(ans.verification).map((v) => `verified by: ${v}`)],
    status: 'draft',
    confidence: 'low',
  };
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(file, JSON.stringify(profile, null, 2) + '\n');
  console.log(`wrote draft ${path.relative(process.cwd(), file)} - the owner must approve it (set "status": "approved")`);
}

function copyTree(src, put, rel) {
  for (const e of fs.readdirSync(src, { withFileTypes: true })) {
    if (e.isDirectory()) copyTree(path.join(src, e.name), put, rel + e.name + '/');
    else put(rel + e.name, fs.readFileSync(path.join(src, e.name)));
  }
}

function init(a) {
  const profiles = a.profile.map((n) => loadProfile(skillRoot, n));
  let existing = {};
  try { existing = JSON.parse(fs.readFileSync(path.join(a.root, 'harness.json'), 'utf8')).overrides ?? {}; } catch {}
  const overrides = { ...existing, ...a.overrides };
  const merged = mergeProfiles(profiles, overrides);
  const created = []; const skipped = [];
  const put = (rel, content) => {
    const dest = path.join(a.root, rel);
    if (fs.existsSync(dest)) { skipped.push(rel); return; }
    fs.mkdirSync(path.dirname(dest), { recursive: true });
    fs.writeFileSync(dest, content);
    created.push(rel);
  };
  const file = (...p) => fs.readFileSync(path.join(skillRoot, ...p));
  for (const t of fs.readdirSync(path.join(skillRoot, 'templates'))) {
    if (t === 'CLAUDE.md') continue;
    put(`docs/templates/${t}`, file('templates', t));
  }
  const rulesDir = path.join(skillRoot, 'core', 'rules');
  if (fs.existsSync(rulesDir)) for (const r of fs.readdirSync(rulesDir)) put(`.claude/rules/${r}`, file('core', 'rules', r));
  const agentsDir = path.join(skillRoot, 'core', 'agents');
  if (fs.existsSync(agentsDir)) for (const r of fs.readdirSync(agentsDir)) put(`.claude/agents/${r}`, file('core', 'agents', r));
  for (const r of merged.rules) put(`.claude/rules/${path.basename(r)}`, fs.readFileSync(r));
  for (const p of profiles) {
    const td = path.join(p.dir, 'templates');
    if (fs.existsSync(td)) for (const t of fs.readdirSync(td)) put(`docs/templates/${t}`, fs.readFileSync(path.join(td, t)));
    const cd = path.join(p.dir, 'checks');
    if (fs.existsSync(cd)) for (const c of fs.readdirSync(cd)) put(`scripts/harness/profile-checks/${p.name}/${c}`, fs.readFileSync(path.join(cd, c)));
  }
  put('CLAUDE.md', file('templates', 'CLAUDE.md'));
  put('docs/README.md', file('core', 'docs-README.md'));
  put('docs/handoff.md', file('templates', 'handoff.md'));
  put('docs/fix-log.md', '# Fix-log\n\nAppend-only. One entry per wrong-layer bug; see `templates/fix-log.md`.\n');
  put('TODO.md', renderIndex([]));
  put('docs/features.json', JSON.stringify({ features: [] }, null, 2) + '\n');
  for (const d of ['features', 'docs/todo', 'docs/design', 'docs/adr', 'docs/audit', 'docs/notes', 'docs/archive', 'docs/reports']) put(`${d}/.gitkeep`, '');
  const sd = path.join(skillRoot, 'scripts');
  for (const e of fs.readdirSync(sd, { withFileTypes: true })) {
    if (e.isDirectory()) copyTree(path.join(sd, e.name), (rel, c) => put('scripts/harness/' + rel, c), e.name + '/');
    else if (e.name.endsWith('.mjs')) put('scripts/harness/' + e.name, file('scripts', e.name));
  }
  put('harness.json', JSON.stringify({
    version: file('VERSION').toString().trim(), profiles: a.profile,
    domains: a.domains.length ? a.domains : ['APP'], overrides,
  }, null, 2) + '\n');
  const ignore = path.join(a.root, '.gitignore');
  const wanted = ['CLAUDE.local.md', '.claude/settings.local.json'];
  const has = fs.existsSync(ignore) ? fs.readFileSync(ignore, 'utf8').split(/\r?\n/).map((l) => l.trim()) : [];
  const missing = wanted.filter((l) => !has.includes(l));
  if (missing.length) {
    const cur = fs.existsSync(ignore) ? fs.readFileSync(ignore, 'utf8') : '';
    fs.writeFileSync(ignore, cur + (cur && !cur.endsWith('\n') ? '\n' : '') + missing.join('\n') + '\n');
    console.log(`.gitignore: added ${missing.join(', ')}`);
  }
  let hookNote = 'Hooks are not installed; run again with --hooks or see core/hooks/settings.sample.json.';
  if (a.hooks) {
    const target = path.join(a.root, '.claude', 'settings.json');
    let current = {};
    if (fs.existsSync(target)) {
      try { current = JSON.parse(fs.readFileSync(target, 'utf8')); } catch (e) { throw new Error(`.claude/settings.json is not valid JSON, not touching it: ${e.message}`); }
    }
    const sample = JSON.parse(file('core', 'hooks', 'settings.sample.json').toString());
    const { settings, added } = mergeSettings(current, sample);
    if (added.length) { fs.mkdirSync(path.dirname(target), { recursive: true }); fs.writeFileSync(target, JSON.stringify(settings, null, 2) + '\n'); }
    hookNote = added.length ? `.claude/settings.json: added ${added.join(', ')}.` : '.claude/settings.json already has the harness hooks.';
  }
  for (const c of created) console.log(`created: ${c}`);
  for (const s of skipped) console.log(`skipped: ${s}`);
  console.log(`${created.length} created, ${skipped.length} skipped. ${hookNote}`);
}

try {
  const a = parseArgs(process.argv.slice(2));
  if (a.newProfile) newProfile(a); else init(a);
} catch (e) {
  console.error(e.message);
  process.exit(1);
}
