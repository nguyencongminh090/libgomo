---
name: cross-error-handling-strategy
description: An error-handling strategy across a system - classifying transient versus permanent faults, where to retry, fail fast, degrade, standard error responses (RFC 9457), not leaking internals, and logging errors at the right level
domain: software-architecture
tags: errors,error-handling,fault-tolerance,problem-details,logging,fail-fast
apply_when: "defining how services report and react to failures; API errors are inconsistent; retry and logging behavior differ per team; designing what users see when a dependency fails"
sources: "Microsoft - Recommendations for handling transient faults, Azure Well-Architected Framework (read); IETF RFC 9457 Problem Details for HTTP APIs, 2023 (read); Beyer et al. - SRE, ch. 22 Addressing Cascading Failures (read); Saltzer and Schroeder 1975, fail-safe defaults (read); Microsoft - Retry and Circuit Breaker patterns (read)"
last_reviewed: 2026-09-25
confidence: medium
---

# Error-handling strategy

Medium confidence: the pieces (transient faults, error format, cascading failure) are sourced;
their combination into one strategy is this note's synthesis. Language-level exception design
is out of scope and belongs to `software-design`.

## 1. Classify the failure first

Azure: an application must detect faults and judge whether they are transient, long-lasting or
terminal, and this differs by resource and by operation (reading versus writing). Transient
faults (momentary network loss, busy service timeouts) are self-correcting; invalid
operations and fatal errors are not.

| Class | Response |
|---|---|
| Transient | retry with bounded attempts and backoff (`dist-resilience-patterns`) |
| Persistent dependency failure | fail fast, circuit breaker, fallback or degrade |
| Client error (invalid input, missing item) | do not retry; return a precise error |
| Bug or internal error | log with context, return a generic error, alert |

Services should help clients decide: Azure suggests error codes or a flag (such as
`isTransient`) and a suggested delay; HTTP `503` with `Retry-After` is an example.

## 2. Decide where handling lives

- Retry in one layer only, where the full context is known; lower layers fail fast and report
  (Azure). Nested retries multiply (3 x 3 = 9 attempts).
- Never retry endlessly; never immediately retry more than once; add jitter so clients do not
  retry in step. Interactive paths use few, short retries; background work can wait longer.
- Keep the total latency of all retries inside the end-to-end requirement.
- Non-idempotent operations must not be retried blindly (`dist-idempotency-retries`).
- After retries are exhausted: dead-letter the message, queue for later, degrade, or tell the
  user plainly (Azure lists these options).

## 3. Report errors consistently

RFC 9457 defines a machine-readable HTTP error body (`application/problem+json`) with `type`
(URI naming the problem class), `title`, `status`, `detail` (specific to this occurrence,
helps the client fix it) and `instance`, plus extension members that clients must ignore if
unknown. Adopt one format across APIs (`dist-api-design`). The RFC warns against leaking
exploitable information, such as stack traces or internal details.

## 4. Fail safely

Saltzer and Schroeder's fail-safe defaults: when a check fails or is unavailable, default to
denial, not access (`cross-security-architecture`). For availability the opposite may apply
to read-only degraded features; decide explicitly per feature (inference).

## 5. Log and measure at the right level

Log transient faults that succeeded on retry as warnings or information, and only the final
failure as an error, so monitors do not raise false alerts; record whether retries came from
throttling or connection faults; measure retry count and elapsed time; alert when retry rates
rise, which signals a growing problem (Azure). Include correlation ids (`cross-observability`).

## 6. Avoid amplification

SRE: naive retries turn overload into a cascading failure; use randomized exponential
backoff, per-process retry budgets, distinct codes for retriable versus permanent errors,
propagated deadlines, and load shedding.

## Use when

- Defining platform-wide conventions for services, clients and messaging consumers.

## Do not use when

- Do not wrap every call in generic retry: retry on invalid requests or internal errors only
  wastes resources (Azure).

## Trade-offs

- More retries raise success rates and load on a struggling dependency.
- Detailed errors help clients and attackers alike; balance with what the caller needs.

## Common mistakes

- Catching and swallowing errors without a log or metric.
- Different error formats per service.
- Retry policies hard-coded in many places (Azure suggests central, configurable policies:
  `cross-configuration`).
- Returning stack traces to clients.

## Related

- Notes: `dist-resilience-patterns`, `dist-idempotency-retries`, `dist-api-design`,
  `cross-observability`, `cross-security-architecture`, `cross-configuration`
