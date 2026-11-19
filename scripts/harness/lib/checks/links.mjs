import fs from 'node:fs';
import path from 'node:path';

// inline [t](target "title") and reference definitions [id]: target
export const INLINE_LINK = /(\[[^\]]*\]\(\s*<?)([^)\s>]+)(>?(?:\s+"[^"]*")?\s*\))/g;
export const REF_LINK = /^(\s{0,3}\[[^\]]+\]:\s*<?)(\S+?)(>?(?:\s+.*)?)$/;

function linksIn(text) {
  const found = [];
  let fenced = false;
  for (const line of text.split('\n')) {
    if (/^\s*(```|~~~)/.test(line)) { fenced = !fenced; continue; }
    if (fenced) continue;
    const plain = line.replace(/`[^`]*`/g, '');
    for (const m of plain.matchAll(INLINE_LINK)) found.push(m[2]);
    const ref = REF_LINK.exec(plain);
    if (ref) found.push(ref[2]);
  }
  return found;
}

const SKIP = new Set(['node_modules', '.git']);
function walkMd(dir, rootDir, out) {
  let entries;
  try { entries = fs.readdirSync(dir, { withFileTypes: true }); } catch { return; }
  for (const e of entries) {
    if (SKIP.has(e.name)) continue;
    const p = path.join(dir, e.name);
    if (e.isDirectory()) walkMd(p, rootDir, out);
    else if (e.isFile() && e.name.endsWith('.md')) out.push(path.relative(rootDir, p).split(path.sep).join('/'));
  }
}

// Every markdown file whose links we check or rewrite: root *.md, docs/**, features/**.
export function filesToScan(ctx) {
  const files = ctx.items.map((i) => i.path);
  try {
    for (const e of fs.readdirSync(ctx.root, { withFileTypes: true })) {
      if (e.isFile() && e.name.endsWith('.md') && e.name !== 'TODO.md') files.push(e.name);
    }
  } catch {}
  walkMd(path.join(ctx.root, 'docs'), ctx.root, files);
  walkMd(path.join(ctx.root, 'features'), ctx.root, files);
  return [...new Set(files)];
}

export function safeDecode(s) {
  try { return decodeURI(s); } catch { return s; }
}

export function run(ctx) {
  const out = [];
  for (const rel of filesToScan(ctx)) {
    let text;
    try { text = fs.readFileSync(path.join(ctx.root, rel), 'utf8'); } catch { continue; }
    for (const target of linksIn(text)) {
      if (/^([a-z][a-z0-9+.-]*:|#|\/)/i.test(target)) continue;
      const clean = safeDecode(target.split('#')[0].split('?')[0]);
      if (!clean) continue;
      if (!fs.existsSync(path.resolve(ctx.root, path.dirname(rel), clean))) {
        out.push({ level: 'error', code: 'LINK-BROKEN', file: rel, message: `link target not found: ${target}` });
      }
    }
  }
  return out;
}
