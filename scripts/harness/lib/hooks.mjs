// Merge core/hooks/settings.sample.json into a project's .claude/settings.json without clobbering it.
const isHarness = (group) => JSON.stringify(group).includes('harness-check.mjs');

// -> { settings, added: string[] }. Existing hooks and permissions stay; ours are added once.
export function mergeSettings(existing, sample) {
  const settings = structuredClone(existing ?? {});
  const added = [];
  settings.hooks ??= {};
  for (const [event, groups] of Object.entries(sample.hooks ?? {})) {
    const have = Array.isArray(settings.hooks[event]) ? settings.hooks[event] : [];
    if (have.some(isHarness)) continue;
    settings.hooks[event] = [...have, ...groups];
    added.push(`hooks.${event}`);
  }
  const deny = sample.permissions?.deny ?? [];
  if (deny.length) {
    settings.permissions ??= {};
    const cur = Array.isArray(settings.permissions.deny) ? settings.permissions.deny : [];
    const fresh = deny.filter((d) => !cur.includes(d));
    if (fresh.length) { settings.permissions.deny = [...cur, ...fresh]; added.push(`permissions.deny(+${fresh.length})`); }
  }
  return { settings, added };
}
