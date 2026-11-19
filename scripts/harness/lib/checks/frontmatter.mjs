const REQUIRED = ['id', 'type', 'size', 'status'];
const SIZES = ['S', 'M', 'L'];
const STATUSES = ['backlog', 'active', 'done', 'archived', 'draft', 'approved', 'superseded'];

export function run(ctx) {
  const out = [];
  const add = (file, message) => out.push({ level: 'error', code: 'FM-MISSING', file, message });
  for (const it of ctx.items) {
    if (it.error === 'missing' || it.error === 'unterminated') { add(it.path, `frontmatter ${it.error}`); continue; }
    if (it.error) add(it.path, `frontmatter has a ${it.error}`);
    for (const k of REQUIRED) if (!(k in it.data)) add(it.path, `frontmatter key "${k}" missing`);
    if (it.data.size && !SIZES.includes(it.data.size)) add(it.path, `size "${it.data.size}" is not S, M or L`);
    if (it.data.status && !STATUSES.includes(it.data.status)) add(it.path, `status "${it.data.status}" is not a known status`);
  }
  return out;
}
