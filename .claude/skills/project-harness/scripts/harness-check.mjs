#!/usr/bin/env node
// harness-check [--root <dir>] [--fix] [--json] | --gate | --stop | --session-start | --prompt (hook modes, read hook JSON on stdin)
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { loadConfig, scanItems } from './lib/workspace.mjs';
import { renderIndex } from './lib/index.mjs';
import { parseFrontmatter } from './lib/frontmatter.mjs';
import { FEATURES_FILE, loadFeatures, featuresOf, onlyPassesChanged } from './lib/checks/features.mjs';
import { codeOf } from './lib/workspace.mjs';
import { runAll } from './lib/checks/all.mjs';
import * as CORE_GATE from './lib/checks/gate.mjs';

export function buildCtx(root) {
  let config; const pre = [];
  try { config = loadConfig(root); } catch (e) {
    config = { version: '', profiles: [], domains: [], overrides: {} };
    pre.push({ level: 'warn', code: 'CONFIG-MISSING', file: 'harness.json', message: e.message });
  }
  const readIndex = () => { try { return fs.readFileSync(path.join(root, 'TODO.md'), 'utf8'); } catch { return ''; } };
  return { ctx: { root, config, items: scanItems(root), indexText: readIndex() }, pre };
}

const here = path.dirname(fileURLToPath(import.meta.url));

// Profile names come from harness.json and reach import(): only plain kebab-case names are accepted.
export const PROFILE_NAME = /^[a-z0-9][a-z0-9-]*$/;
export const invalidProfileNames = (profiles) => (Array.isArray(profiles) ? profiles : []).filter((p) => typeof p !== 'string' || !PROFILE_NAME.test(p));

const realOrNull = (p) => { try { return fs.realpathSync(p); } catch { return null; } };
const inside = (dir, parent) => { const a = realOrNull(dir), b = realOrNull(parent); return !!a && !!b && a.startsWith(b + path.sep); };

// Profile checks: installed copy (profile-checks/<p>/*.mjs) or running from the skill (../profiles/<p>/checks/*.mjs).
// Invalid names are skipped, and so is a directory whose real path leaves its allowed root (symlink escape).
export async function loadProfileChecks(profiles, base = here) {
  const mods = [];
  const bad = new Set(invalidProfileNames(profiles));
  for (const p of profiles) {
    if (bad.has(p)) continue;
    for (const [parent, dir] of [['profile-checks', path.join(base, 'profile-checks', p)], ['profiles', path.join(base, '..', 'profiles', p, 'checks')]]) {
      if (!fs.existsSync(dir)) continue;
      const root = parent === 'profile-checks' ? path.join(base, 'profile-checks') : path.join(base, '..', 'profiles');
      if (!inside(dir, root)) break;
      for (const f of fs.readdirSync(dir).filter((x) => x.endsWith('.mjs')).sort()) {
        mods.push(await import(pathToFileURL(path.join(dir, f)).href));
      }
      break;
    }
  }
  return mods;
}

// PreToolUse hook. features.json: an edit that changes more than `passes` asks the owner (JSON
// permissionDecision "ask"). Code: exit 2 blocks while an active todo is L or its planning is unapproved.
// Real path of the target (symlinks and ".." resolved, also through a not-yet-existing tail), relative to the real root,
// with "/" separators; always case-folded (fail closed): a volume under the root may be case-insensitive even when the root is not,
// so "Docs/Features.JSON" must not dodge the match. On a case-sensitive FS this only means a harmless extra ask.
const realOf = (p) => {
  let cur = path.resolve(p); const rest = [];
  for (;;) {
    try { return path.join(fs.realpathSync.native(cur), ...rest.reverse()); } catch {
      const up = path.dirname(cur);
      if (up === cur) return path.resolve(p);
      rest.push(path.basename(cur)); cur = up;
    }
  }
};
export function canonicalRel(root, target) {
  return path.relative(realOf(root), realOf(target)).split(path.sep).join('/').toLowerCase();
}
const ask = (reason) => console.log(JSON.stringify({ hookSpecificOutput: { hookEventName: 'PreToolUse', permissionDecision: 'ask', permissionDecisionReason: reason } }));
const CONTROL = /^(harness\.json$|\.claude\/|docs\/|features\/[^/]+\/planning\.md$)/;
const EXEMPT = /^docs\/(handoff\.md$|fix-log\.md$|todo\/|notes\/)/;
function gate(root) {
  let hook = {};
  try { hook = JSON.parse(fs.readFileSync(0, 'utf8')); } catch {}
  const target = hook.tool_input?.file_path ?? hook.tool_input?.notebook_path ?? '';
  const rel = target ? canonicalRel(root, target) : '';
  if (rel === FEATURES_FILE) {
    let current = '';
    try { current = fs.readFileSync(path.join(root, FEATURES_FILE), 'utf8'); } catch {}
    if (!onlyPassesChanged(hook.tool_name, hook.tool_input ?? {}, current)) {
      ask(`${FEATURES_FILE}: agents may only flip "passes"; adding, editing or removing features is the owner's call.`);
    }
    return 0;
  }
  // Control files: overrides, hooks, rules, planning status and decisions are the owner's call. Daily working docs are exempt.
  if (CONTROL.test(rel) && !EXEMPT.test(rel)) { ask(`${rel}: harness control file; changing it is the owner's call.`); return 0; }
  if (/^(docs\/|features\/|\.claude\/|TODO\.md$|CLAUDE\.md$|harness\.json$)/.test(rel)) return 0;
  const { ctx } = buildCtx(root);
  ctx.items = ctx.items.filter((i) => i.data.status === 'active' || i.type === 'planning');
  const blocked = CORE_GATE.run(ctx).filter((f) => f.code === 'GATE-L-NO-PLANNING' || f.code === 'TODO-SIZE-L');
  if (!blocked.length) return 0;
  for (const f of blocked) console.error(`${f.code} ${f.file}: ${f.message}`);
  console.error('Write and approve the planning (docs only) before editing code.');
  return 2;
}

async function findingsFor(ctx, pre) {
  const extra = await loadProfileChecks(ctx.config.profiles);
  const names = invalidProfileNames(ctx.config.profiles).map((p) => ({ level: 'error', code: 'PROFILE-NAME', file: 'harness.json', message: `invalid profile name ${JSON.stringify(p)}: use lower-case letters, digits and "-"` }));
  return [...pre, ...names, ...runAll(ctx, extra)];
}
const line = (f) => `${f.level.toUpperCase()} ${f.code} ${f.file}: ${f.message}`;
const readStdinJson = () => { try { return JSON.parse(fs.readFileSync(0, 'utf8') || '{}'); } catch { return {}; } };

// Stop hook: Claude Code blocks only on exit 2 (stderr goes to Claude). Exit 1 would just be shown
// to the user. stop_hook_active means we already blocked once this turn: let it stop (loop guard).
// Baseline: SessionStart records the errors that already exist (per session_id, in the OS temp dir);
// Stop then blocks only on errors that are new since. No baseline (hook not wired) = block on all errors.
const fingerprint = (f) => `${f.code}|${f.file}|${f.message}`;
const baselineFile = (sid) => path.join(os.tmpdir(), `harness-baseline-${String(sid).replace(/[^\w-]/g, '_')}.json`);

async function stop(root) {
  const input = readStdinJson();
  if (input.stop_hook_active === true) return 0;
  const { ctx, pre } = buildCtx(root);
  let errors = (await findingsFor(ctx, pre)).filter((f) => f.level === 'error');
  let known = 0;
  if (input.session_id) {
    try {
      const base = new Set(JSON.parse(fs.readFileSync(baselineFile(input.session_id), 'utf8')));
      const fresh = errors.filter((f) => !base.has(fingerprint(f)));
      known = errors.length - fresh.length; errors = fresh;
    } catch {}
  }
  if (!errors.length) return 0;
  for (const f of errors) console.error(line(f));
  console.error(`${errors.length} new harness error(s)${known ? ` (${known} older ones ignored)` : ''}: fix them (node scripts/harness/harness-check.mjs) before finishing.`);
  return 2;
}

const section = (body, name) => {
  const m = new RegExp(`^## ${name}[^\\n]*\\n([\\s\\S]*?)(?=^## |(?![\\s\\S]))`, 'm').exec(body || '');
  return m ? m[1].trim() : '';
};

// SessionStart hook: stdout is added to the agent's context. Keep it short: where the work stands,
// what is active, and whether the workspace is healthy (Anthropic's session-start routine).
async function sessionStart(root) {
  const { ctx, pre } = buildCtx(root);
  const sid = readStdinJson().session_id;
  const out = ['## Harness session start'];
  let handoff = '';
  try { handoff = parseFrontmatter(fs.readFileSync(path.join(root, 'docs', 'handoff.md'), 'utf8')).body; } catch {}
  if (handoff) {
    for (const s of ['Doing now', 'Next step', 'Blockers']) {
      const t = section(handoff, s);
      if (t) out.push(`${s}: ${t.split('\n').slice(0, 5).join(' / ')}`);
    }
  } else out.push('No handoff (docs/handoff.md).');
  const active = ctx.items.filter((i) => !i.archived && i.data.status === 'active' && i.type !== 'handoff');
  const { features } = loadFeatures(root);
  const label = (i) => {
    const mine = codeOf(i) ? featuresOf(features, codeOf(i)) : [];
    return String(i.data.id ?? i.path) + (mine.length ? ` (features ${mine.filter((f) => f.passes === true).length}/${mine.length} passing)` : '');
  };
  out.push(`Active: ${active.length ? active.map(label).join(', ') : 'none'}`);
  const findings = await findingsFor(ctx, pre);
  const errors = findings.filter((f) => f.level === 'error');
  if (sid && !fs.existsSync(baselineFile(sid))) {
    try { fs.writeFileSync(baselineFile(sid), JSON.stringify(errors.map(fingerprint))); } catch {}
  }
  out.push(`harness-check: ${findings.length} finding(s), ${errors.length} error(s)`);
  for (const f of errors.slice(0, 5)) out.push('  ' + line(f));
  out.push('Read the active todo before editing code, and run the Smoke command from CLAUDE.md before new work.');
  console.log(out.join('\n'));
  return 0;
}

// UserPromptSubmit hook: a short standing reminder, as additionalContext (must sit under hookSpecificOutput).
// Advisory rules alone were skipped by the smallest model in evals (idea raised in passing was never recorded).
const PROMPT_REMINDER = 'Harness reminder: if this message raises a NEW requirement or idea (even in passing, e.g. "we will probably want X"), record it first as a backlog todo (docs/templates/todo.md; a planning if it is L-size) and do NOT build it until the owner says to (do this now / implement / fix it). Answer anything else asked as usual.';
async function promptSubmit() {
  readStdinJson();
  console.log(JSON.stringify({ hookSpecificOutput: { hookEventName: 'UserPromptSubmit', additionalContext: PROMPT_REMINDER } }));
  return 0;
}

async function main(argv) {
  const args = { root: process.cwd(), fix: false, json: false, gate: false, stop: false, sessionStart: false, prompt: false };
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '--root') args.root = path.resolve(argv[++i]);
    else if (argv[i] === '--fix') args.fix = true;
    else if (argv[i] === '--json') args.json = true;
    else if (argv[i] === '--gate') args.gate = true;
    else if (argv[i] === '--stop') args.stop = true;
    else if (argv[i] === '--session-start') args.sessionStart = true;
    else if (argv[i] === '--prompt') args.prompt = true;
  }
  // A crashing hook must not fail silently (exit 1 is non-blocking): warn the user and let the action go on.
  const hook = async (fn) => {
    try { process.exit(await fn(args.root)); } catch (e) {
      console.log(JSON.stringify({ systemMessage: `harness hook crashed and was skipped: ${e.message}` }));
      process.exit(0);
    }
  };
  if (args.gate) await hook(gate);
  if (args.stop) await hook(stop);
  if (args.sessionStart) await hook(sessionStart);
  if (args.prompt) await hook(promptSubmit);
  let { ctx, pre } = buildCtx(args.root);
  if (args.fix) {
    fs.writeFileSync(path.join(args.root, 'TODO.md'), renderIndex(ctx.items));
    ({ ctx, pre } = buildCtx(args.root));
  }
  const findings = await findingsFor(ctx, pre);
  if (args.json) console.log(JSON.stringify(findings, null, 2));
  else {
    for (const f of findings) console.log(line(f));
    const errors = findings.filter((f) => f.level === 'error').length;
    console.log(`${findings.length} finding(s), ${errors} error(s)`);
  }
  process.exit(findings.some((f) => f.level === 'error') ? 1 : 0);
}
if (process.argv[1] && import.meta.url === pathToFileURL(fs.realpathSync(process.argv[1])).href) await main(process.argv.slice(2));
