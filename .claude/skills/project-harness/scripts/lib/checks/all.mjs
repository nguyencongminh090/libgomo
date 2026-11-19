import * as frontmatter from './frontmatter.mjs';
import * as codes from './codes.mjs';
import * as index from './index.mjs';
import * as links from './links.mjs';
import * as layout from './layout.mjs';
import * as gate from './gate.mjs';
import * as done from './done.mjs';
import * as features from './features.mjs';

export const CORE_CHECKS = [frontmatter, codes, index, links, layout, gate, done, features];

export function runAll(ctx, extra = []) {
  return [...CORE_CHECKS, ...extra].flatMap((c) => c.run(ctx));
}
