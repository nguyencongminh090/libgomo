# Library profile (low confidence: no owner project informed it)

- Any change to the public API is size L, even a small one: planning, then an ADR if it breaks compatibility.
- Follow semantic versioning; a breaking change needs a major-version note in the changelog.
- Public behaviour needs a test that a consumer would write, not only internal tests.
