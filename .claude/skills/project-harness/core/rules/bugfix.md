# Bug fixes (advisory)

Every bug is sized, then follows one chain: reproduce -> find the root cause -> add a regression check that stays -> write the `fix-log` entry. Scope is strictly what was reported; note other problems as new todos instead of fixing them. When the first fix was the wrong layer, say so in the fix-log entry ("First fix" vs "Real source"), and add the check that would have caught it.
