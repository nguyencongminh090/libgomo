// Acceptance lives in docs/features.json (Anthropic long-running harness pattern: JSON is rewritten less
// often than Markdown; agents flip only `passes`). Todo frontmatter stays the lifecycle truth.
import fs from 'node:fs';
import path from 'node:path';
import { codeOf } from '../workspace.mjs';

export const FEATURES_FILE = 'docs/features.json';

// -> { features: [], error?: string }. A missing file is an empty list, not an error.
export function loadFeatures(root) {
  const file = path.join(root, FEATURES_FILE);
  if (!fs.existsSync(file)) return { features: [], missing: true };
  let j;
  try { j = JSON.parse(fs.readFileSync(file, 'utf8')); } catch (e) { return { features: [], error: `not valid JSON: ${e.message}` }; }
  if (!j || typeof j !== 'object' || Array.isArray(j) || !Array.isArray(j.features)) {
    return { features: [], error: 'expected an object { "features": [ ... ] }' };
  }
  return { features: j.features };
}

export const featuresOf = (features, code) => features.filter((f) => f && f.todo === code);

export function run(ctx) {
  const out = [];
  const { features, error, missing } = loadFeatures(ctx.root);
  if (error) return [{ level: 'error', code: 'FEATURES-INVALID', file: FEATURES_FILE, message: error }];
  const todos = ctx.items.filter((i) => i.type === 'todo' && codeOf(i));
  const codes = new Set(todos.map(codeOf));
  const seen = new Set();
  features.forEach((f, n) => {
    const where = `feature #${n + 1}${f && f.id ? ` (${f.id})` : ''}`;
    const bad = !f || typeof f !== 'object' ? ['object'] : [
      ...['id', 'todo', 'description'].filter((k) => typeof f[k] !== 'string' || !f[k]),
      ...(Array.isArray(f.steps) ? [] : ['steps']),
      ...(typeof f.passes === 'boolean' ? [] : ['passes']),
    ];
    if (bad.length) { out.push({ level: 'error', code: 'FEATURE-FIELDS', file: FEATURES_FILE, message: `${where}: missing or wrong type: ${bad.join(', ')}` }); return; }
    if (seen.has(f.id)) out.push({ level: 'error', code: 'FEATURE-DUP', file: FEATURES_FILE, message: `${where}: id used twice` });
    seen.add(f.id);
    if (!codes.has(f.todo)) out.push({ level: 'error', code: 'FEATURE-ORPHAN', file: FEATURES_FILE, message: `${where}: no todo with code ${f.todo}` });
  });
  for (const t of todos) {
    if (t.archived) continue;
    const mine = featuresOf(features, codeOf(t));
    const st = t.data.status;
    if ((st === 'done' || st === 'archived') && ctx.config.overrides?.doneNeedsChecks !== false) {
      const failing = mine.filter((f) => f.passes !== true).map((f) => f.id);
      if (failing.length) out.push({ level: 'error', code: 'DONE-FEATURE-FAILING', file: t.path, message: `done but failing in ${FEATURES_FILE}: ${failing.join(', ')}` });
    }
    // Done with zero features would make "all features pass" vacuously true: an error once the project
    // uses features.json (init creates it). Active, or a legacy project without the file: warn only.
    if ((st === 'active' || st === 'done') && t.data.size === 'M' && !mine.length) {
      const strict = st === 'done' && !missing && ctx.config.overrides?.doneNeedsChecks !== false;
      out.push({ level: strict ? 'error' : 'warn', code: 'FEATURE-NONE', file: t.path, message: `no acceptance feature in ${FEATURES_FILE} for ${codeOf(t)}` });
    }
  }
  return out;
}

// PreToolUse guard: an edit to features.json that changes anything besides `passes` needs the owner.
// The edit is applied to the current text and the JSON is compared as data (whitespace and key layout are not changes).
// A non-boolean `passes` is a change. Unknown tools or unparseable JSON mean ask. An old_string that is not in the
// file makes the tool itself reject the edit (nothing is written), so it is only judged by whether old/new differ beyond `passes`.
const semantic = (text) => {
  const j = JSON.parse(text);
  for (const f of Array.isArray(j?.features) ? j.features : []) if (f && typeof f === 'object' && typeof f.passes === 'boolean') f.passes = null;
  return JSON.stringify(j);
};
const normPasses = (t) => t.replace(/("passes"\s*:\s*)(true|false)/g, '$1_');
function applyEdit(text, e) {
  if (typeof e?.old_string !== 'string' || typeof e?.new_string !== 'string' || !e.old_string) return null;
  if (!text.includes(e.old_string)) return normPasses(e.old_string) === normPasses(e.new_string) ? text : null;
  return e.replace_all === true ? text.split(e.old_string).join(e.new_string) : text.replace(e.old_string, () => e.new_string);
}
export function onlyPassesChanged(toolName, input, currentText) {
  try {
    let next;
    if (toolName === 'Write') next = input.content;
    else if (toolName === 'Edit' || toolName === 'MultiEdit') {
      next = currentText;
      for (const e of toolName === 'MultiEdit' ? (input.edits ?? []) : [input]) {
        next = applyEdit(next, e);
        if (next === null) return false;
      }
    } else return false;
    return semantic(next) === semantic(currentText);
  } catch { return false; }
}
