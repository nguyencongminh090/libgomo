---
name: style-pipe-filter
description: Pipes and filters - build a processing task as independent single-purpose filters joined by pipes; reuse, reordering and parallel scaling, plus idempotency and duplicate handling when distributed
domain: software-architecture
tags: architecture,style,pipe-filter,pipeline,stream-processing,messaging
apply_when: "a task is a sequence of independent transformation steps, steps differ in scaling needs, or steps must be reordered or reused across flows"
sources: "Microsoft - Azure Architecture Center, Pipes and Filters pattern (read); Hohpe, Woolf - Enterprise Integration Patterns, Pipes and Filters, enterpriseintegrationpatterns.com and the book chapter (read; book re-read 2026-09-26: problem of chaining independent processing steps flexibly, order example with decrypt, authenticate, de-duplicate); Microsoft - Azure Architecture Center, Event-driven architecture style (read, for combinations)"
last_reviewed: 2026-09-26
confidence: high
---

# Pipes and filters

## Intent

Divide a larger processing task into a sequence of smaller, independent steps (filters)
connected by channels (pipes), so steps can be reused, reordered, replaced and scaled
separately (Enterprise Integration Patterns wording).

## Structure

- **Filter**: one processing step with a simple interface; reads from an inbound pipe,
  transforms or tests the message, writes to an outbound pipe. Independent, self-contained,
  typically stateless, unaware of other filters, only aware of its input and output schemas.
- **Pipe**: connects filters and passes messages on. It does no routing and no other logic.
- Filters can be arranged in any order as long as each output schema matches the next input
  schema; a standard schema for all filters makes reordering easy.
- Implementations range from in-process function chains to queues between separately hosted
  workers. Azure's example is an image pipeline (content moderation, resize, watermark,
  metadata removal, publish) with a queue as each pipe and a small reference (claim check)
  in each message instead of the image itself.

## Why it helps (Azure)

- Reuse: the same filters compose into new pipelines.
- Independent change: replace or update one filter's logic.
- Different hardware per filter, and parallel instances of a slow filter. Throughput of a
  request depends on the slowest filter, so scale that one.
- If input and output are streams, filters can work in parallel before the first one
  finishes.

## Use when

- The work splits into independent steps that may have different scaling needs, or steps you
  want to add, remove or reorder.
- Processing can be spread across servers, and you want a step's failure to have limited
  effect while data is in flight.
- Batch or streaming data transformation: ingest, validate, enrich, convert, publish.

## Do not use when

- The flow is request-response and the whole task must finish within the initial request
  (Azure lists this as a poor fit).
- Steps are not independent or must run in one transaction.
- A step needs so much context or state that loading and saving it per filter costs more than
  the design saves; database contention can result.
- The task is small and stable: a plain function composition is enough and distribution only
  adds messaging overhead.

## Trade-offs and design points (Azure)

- **Monolithic nature**: usually the whole pipeline is tested end to end after any change,
  and if a filter or pipe fails the pipeline is likely to fail as a whole.
- **Idempotency**: if a filter fails after posting its result but before confirming, another
  instance may repeat the work, so filters should be idempotent.
- **Repeated messages**: detect and drop duplicates; some queue services offer duplicate
  detection.
- **Message tolerance**: a filter should ignore and pass along fields it does not use.
- **Reliability**: the pipe infrastructure must not lose messages between filters.
- **Error handling**: every filter decides whether a breaking error fails the pipeline or is
  propagated.
- **Complexity** grows when filters are spread over servers.

## Common mistakes

- Pipes that route or transform, hiding logic in the wiring.
- Filters that share hidden state or depend on a particular predecessor.
- Distributing filters that always run together on one machine, paying network cost for no
  scaling benefit (Azure notes filters that scale together can be grouped in one process).

## Verification note

Two independent sources describe the same structure and cautions. The classic formulation in
Shaw and Garlan (1996) and POSA vol. 1 was not read; the Unix pipe history is not claimed.

## Related

- Notes: `style-event-driven`, `style-choosing`, `principle-coupling-cohesion`
- See also: `dist-idempotency-retries`, `data-transactions-sagas`
