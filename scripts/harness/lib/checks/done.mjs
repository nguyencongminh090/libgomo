export function doneWhenSection(body) {
  const lines = (body || '').split('\n');
  const start = lines.findIndex((l) => /^##\s+Done-when\b/.test(l));
  if (start === -1) return [];
  const rest = lines.slice(start + 1);
  const end = rest.findIndex((l) => /^##\s/.test(l));
  return end === -1 ? rest : rest.slice(0, end);
}

export function run(ctx) {
  const out = [];
  if (ctx.config.overrides?.doneNeedsChecks === false) return out;
  for (const t of ctx.items) {
    if (t.type !== 'todo' || t.error === 'missing' || t.error === 'unterminated') continue;
    if (t.data.status !== 'done' && t.data.status !== 'archived') continue;
    const open = doneWhenSection(t.body).filter((l) => /^\s*- \[ \]/.test(l)).length;
    if (open) out.push({ level: 'error', code: 'DONE-UNCHECKED', file: t.path, message: `${open} unchecked Done-when item(s) on a ${t.data.status} todo` });
  }
  return out;
}
