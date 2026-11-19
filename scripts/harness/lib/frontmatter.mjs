// Flat frontmatter parser: `key: value`, "quoted" values, [a, b] arrays. Never throws.
function parseValue(raw) {
  const v = raw.trim();
  if (v.startsWith('[') && v.endsWith(']')) {
    const inner = v.slice(1, -1).trim();
    if (!inner) return [];
    return inner.split(',').map((s) => unquote(s.trim()));
  }
  return unquote(v);
}
function unquote(s) {
  if (s.length >= 2 && s.startsWith('"') && s.endsWith('"')) return s.slice(1, -1);
  if (s.length >= 2 && s.startsWith("'") && s.endsWith("'")) return s.slice(1, -1);
  return s;
}

export function parseFrontmatter(text) {
  const src = typeof text === 'string' ? text : '';
  const lines = src.split(/\r?\n/);
  if (lines[0] !== '---') return { data: {}, body: src, error: 'missing' };
  let end = -1;
  for (let i = 1; i < lines.length; i++) {
    if (lines[i] === '---') { end = i; break; }
  }
  if (end === -1) return { data: {}, body: src, error: 'unterminated' };
  const data = {};
  let error;
  for (const line of lines.slice(1, end)) {
    if (!line.trim() || line.trim().startsWith('#')) continue;
    const m = /^([A-Za-z_][\w-]*):(.*)$/.exec(line);
    if (!m) { error = 'malformed-line'; continue; }
    data[m[1]] = parseValue(m[2]);
  }
  const body = lines.slice(end + 1).join('\n');
  return error ? { data, body, error } : { data, body };
}

function fmt(v) {
  if (Array.isArray(v)) return `[${v.join(', ')}]`;
  const s = String(v ?? '');
  return /[:#\[\]]/.test(s) || s !== s.trim() ? `"${s}"` : s;
}

export function stringifyFrontmatter(data) {
  return '---\n' + Object.entries(data).map(([k, v]) => `${k}: ${fmt(v)}`).join('\n') + '\n---\n';
}
