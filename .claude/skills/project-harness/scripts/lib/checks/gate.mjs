import path from 'node:path';

// Open questions left in a planning: list items or text under "## Open questions" (the template
// placeholder in parentheses does not count), or a Spec Kit style "[NEEDS CLARIFICATION: ...]" marker anywhere.
// Code spans and whole-line "(...)" placeholders are documentation, never questions.
export function openQuestions(body = '') {
  const qs = [];
  const text = body.replace(/`[^`\n]*`/g, '').split('\n').filter((l) => !/^\s*\(.*\)\s*$/.test(l)).join('\n');
  const sec = /^## Open questions[^\n]*\n([\s\S]*?)(?=^## |(?![\s\S]))/m.exec(text);
  if (sec) {
    for (const line of sec[1].split('\n').map((l) => l.trim())) {
      if (line) qs.push(line);
    }
  }
  for (const m of text.matchAll(/\[NEEDS CLARIFICATION:[^\]]*\]/g)) qs.push(m[0]);
  return qs;
}

export function run(ctx) {
  const out = [];
  const plannings = ctx.items.filter((i) => i.type === 'planning');
  const isOpen = (p) => p.data.status === 'approved' && openQuestions(p.body).length > 0;
  for (const p of plannings) {
    if (isOpen(p)) {
      out.push({ level: 'error', code: 'PLANNING-OPEN', file: p.path, message: `approved planning still has open questions: ${openQuestions(p.body).slice(0, 3).join(' | ')}` });
    }
  }
  const findPlanning = (slug) => plannings.find((p) => p.data.id === slug || path.basename(path.dirname(p.path)) === slug);
  for (const t of ctx.items) {
    if (t.type !== 'todo' || t.archived || t.error === 'missing' || t.error === 'unterminated') continue;
    const slug = typeof t.data.planning === 'string' ? t.data.planning : '';
    // The planning is the L unit (0.3.0): L work lives in features/<slug>/planning.md, split into todos of at most M.
    if (t.data.size === 'L' && ctx.config.overrides?.lNeedsPlanning !== false) {
      out.push({ level: 'error', code: 'TODO-SIZE-L', file: t.path, message: `a todo cannot be size L: L work is a planning in features/<slug>/; split it into todos of at most M that name "planning: <slug>"` });
    }
    // A todo split from a planning may sit in backlog while the planning is a draft, but not start.
    const started = t.data.status === 'active' || t.data.status === 'done';
    if (slug && started) {
      const p = findPlanning(slug);
      let why = '';
      if (!p) why = `planning "${slug}" not found`;
      else if (p.data.status !== 'approved') why = `planning "${slug}" is ${p.data.status ?? 'unset'}, not approved`;
      else if (isOpen(p)) why = `planning "${slug}" is approved but still has open questions`;
      if (why) out.push({ level: 'error', code: 'GATE-L-NO-PLANNING', file: t.path, message: `todo started before its planning was approved: ${why}` });
    }
  }
  return out;
}
