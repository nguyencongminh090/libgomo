import fs from 'node:fs';
import path from 'node:path';
import { parseFrontmatter } from './frontmatter.mjs';

const FOLDER_TYPES = { todo: 'todo', design: 'design', adr: 'adr', audit: 'audit', notes: 'note', archive: null };
const SKIP = new Set(['node_modules', '.git']);

export function loadConfig(root) {
  const file = path.join(root, 'harness.json');
  if (!fs.existsSync(file)) throw new Error('harness.json not found');
  const c = JSON.parse(fs.readFileSync(file, 'utf8'));
  return {
    version: c.version ?? '', profiles: c.profiles ?? [], domains: c.domains ?? [], overrides: c.overrides ?? {},
  };
}

function walk(dir, out) {
  let entries;
  try { entries = fs.readdirSync(dir, { withFileTypes: true }); } catch { return; }
  for (const e of entries) {
    if (SKIP.has(e.name)) continue;
    const p = path.join(dir, e.name);
    if (e.isDirectory()) walk(p, out);
    else if (e.isFile() && e.name.endsWith('.md')) out.push(p);
  }
}

function toItem(root, file, folderType, archived) {
  let text = '';
  try { text = fs.readFileSync(file, 'utf8'); } catch (e) { text = ''; }
  const fm = parseFrontmatter(text);
  const declared = typeof fm.data.type === 'string' ? fm.data.type : null;
  const item = {
    path: path.relative(root, file).split(path.sep).join('/'),
    type: folderType ?? declared ?? 'unknown',
    data: fm.data, body: fm.body, archived,
  };
  if (fm.error) item.error = fm.error;
  return item;
}

export function scanItems(root) {
  const items = [];
  for (const [folder, type] of Object.entries(FOLDER_TYPES)) {
    const files = [];
    walk(path.join(root, 'docs', folder), files);
    for (const f of files) items.push(toItem(root, f, type, folder === 'archive'));
  }
  const featDir = path.join(root, 'features');
  if (fs.existsSync(featDir)) {
    for (const e of fs.readdirSync(featDir, { withFileTypes: true })) {
      const p = path.join(featDir, e.name, 'planning.md');
      if (e.isDirectory() && fs.existsSync(p)) items.push(toItem(root, p, 'planning', false));
    }
  }
  return items;
}

export const codeOf = (item) => {
  const id = typeof item.data.id === 'string' ? item.data.id : path.basename(item.path, '.md');
  const m = /^([A-Z][A-Z0-9]*-\d+)(?:-|$)/.exec(id);
  return m ? m[1] : null;
};

export const titleOf = (item) => {
  const m = /^#\s+(.+)$/m.exec(item.body || '');
  return m ? m[1].trim() : String(item.data.id ?? item.path);
};

export function nextCode(items, domain) {
  let max = 0;
  for (const it of items) {
    const c = codeOf(it);
    if (!c) continue;
    const [d, n] = [c.slice(0, c.lastIndexOf('-')), Number(c.slice(c.lastIndexOf('-') + 1))];
    if (d === domain && n > max) max = n;
  }
  return `${domain}-${String(max + 1).padStart(2, '0')}`;
}
