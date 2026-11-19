import fs from 'node:fs';
import path from 'node:path';

const SIZE_RANK = { M: 1, L: 2 };

export function assertProfileName(name) {
  if (!/^[A-Za-z0-9][A-Za-z0-9_-]*$/.test(String(name))) throw new Error(`invalid profile name: ${name}`);
}

export function profileFile(skillRoot, name) {
  assertProfileName(name);
  return path.join(skillRoot, 'profiles', name, 'profile.json');
}

export function loadProfile(skillRoot, name) {
  const file = profileFile(skillRoot, name);
  let raw;
  try { raw = JSON.parse(fs.readFileSync(file, 'utf8')); } catch (e) {
    throw new Error(`cannot read ${file}: ${e.message}`);
  }
  const bad = (why) => new Error(`invalid ${file}: ${why}`);
  if (!raw || typeof raw !== 'object' || Array.isArray(raw)) throw bad('not an object');
  for (const k of ['name', 'description']) if (typeof raw[k] !== 'string' || !raw[k]) throw bad(`"${k}" missing`);
  for (const k of ['conflicts', 'size_signals', 'definition_of_done']) {
    if (raw[k] !== undefined && !Array.isArray(raw[k])) throw bad(`"${k}" must be an array`);
  }
  for (const s of raw.size_signals ?? []) {
    if (typeof s.pattern !== 'string' || !(s.size in SIZE_RANK)) throw bad('size_signals entries need pattern and size M|L');
  }
  const dir = path.dirname(file);
  const rulesDir = path.join(dir, 'rules');
  const rules = fs.existsSync(rulesDir)
    ? fs.readdirSync(rulesDir).filter((f) => f.endsWith('.md')).sort().map((f) => path.join(rulesDir, f)) : [];
  return {
    name: raw.name, description: raw.description, when_to_use: raw.when_to_use ?? '',
    conflicts: raw.conflicts ?? [], weakens: raw.weakens ?? [], size_signals: raw.size_signals ?? [],
    definition_of_done: raw.definition_of_done ?? [], checks: raw.checks ?? [],
    status: raw.status ?? 'approved', confidence: raw.confidence ?? 'normal', dir, rules,
  };
}

export function mergeProfiles(profiles, overrides = {}) {
  for (let i = 0; i < profiles.length; i++) {
    for (let j = i + 1; j < profiles.length; j++) {
      const a = profiles[i]; const b = profiles[j];
      if (a.conflicts.includes(b.name) || b.conflicts.includes(a.name)) throw new Error(`profile conflict: ${a.name} vs ${b.name}`);
    }
  }
  for (const p of profiles) {
    for (const rule of p.weakens) {
      if (!(rule in overrides)) throw new Error(`profile ${p.name} weakens core rule ${rule} without override`);
    }
  }
  const signals = new Map();
  const dod = []; const rules = []; const checks = [];
  for (const p of profiles) {
    for (const s of p.size_signals) {
      const prev = signals.get(s.pattern);
      if (!prev || SIZE_RANK[s.size] > SIZE_RANK[prev.size]) signals.set(s.pattern, { ...s, profile: p.name });
    }
    for (const d of p.definition_of_done) if (!dod.includes(d)) dod.push(d);
    rules.push(...p.rules);
    for (const c of p.checks) checks.push(path.join(p.dir, 'checks', c.endsWith('.mjs') ? c : c + '.mjs'));
  }
  return { sizeSignals: [...signals.values()], definitionOfDone: dod, rules, checks };
}
