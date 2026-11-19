import { codeOf, titleOf } from './workspace.mjs';

export const MARKERS = { backlog: ' ', active: '~', done: 'x' };
const STATUS_BY_MARKER = { ' ': 'backlog', '~': 'active', x: 'done' };
const LINE = /^- \[( |~|x)\] \*\*([A-Z][A-Z0-9]*-\d+)\.\*\* (.+) - size ([SML]) - \[detail\]\((.+)\)$/;

export function parseIndexLine(line) {
  const m = LINE.exec(String(line).replace(/\r$/, ''));
  if (!m) return null;
  return { status: STATUS_BY_MARKER[m[1]], code: m[2], title: m[3], size: m[4], link: m[5] };
}

export function indexableTodos(items) {
  return items.filter((i) => i.type === 'todo' && !i.archived && codeOf(i) && MARKERS[i.data.status] !== undefined && ['S', 'M', 'L'].includes(i.data.size));
}

export function renderLine(item) {
  return `- [${MARKERS[item.data.status]}] **${codeOf(item)}.** ${titleOf(item)} - size ${item.data.size} - [detail](${item.path})`;
}

export function renderIndex(items) {
  const todos = indexableTodos(items);
  const out = ['# TODO', ''];
  for (const [status, heading] of [['backlog', 'Backlog'], ['active', 'Active'], ['done', 'Done']]) {
    out.push(`## ${heading}`);
    todos.filter((t) => t.data.status === status)
      .sort((a, b) => codeOf(a).localeCompare(codeOf(b), 'en', { numeric: true }))
      .forEach((t) => out.push(renderLine(t)));
    out.push('');
  }
  return out.join('\n');
}
