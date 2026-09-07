# The model

This document is normative. It is precise enough to implement without asking a
question, and every result in [proof_k1.md](proof_k1.md) is stated against it.
Changing anything here invalidates every earlier result and every CSV in
`results/`.

---

## 1. Instances

An **interval** is a closed real interval `I = [l, r]` with `l ≤ r`. Two
intervals are **adjacent** iff they intersect:

```
overlaps(a, b)  <=>  a.l <= b.r  and  b.l <= a.r
```

Endpoints are compared exactly, and touching counts: `[0,1]` and `[1,2]` are
adjacent. (`Offline.TouchingEndpointsOverlap` pins this down. It matters: the
opposite convention changes ω on hand-built instances.)

An **instance** is a finite sequence `σ = (I_1, …, I_n)`, the arrival order,
chosen by an adversary. `G(σ)` is the interval graph on these intervals and
`ω(σ) = χ(G(σ))` is the maximum number of intervals covering a common point.

A **colouring** assigns `c(I_t) ∈ ℕ` to each interval. It is **valid** iff
adjacent intervals get different colours. `ALG(σ)` is the number of *distinct*
colours the algorithm uses on σ; `OPT(σ) = ω(σ)`.

An algorithm is **c-competitive** if `ALG(σ) ≤ c · OPT(σ) + b` for all σ and a
constant b independent of σ.

Algorithms are **deterministic** throughout. Every lower bound below is stated
against deterministic algorithms.

---

## 2. The online rule

At step t the algorithm is shown `I_t` and a **buffer** `B_t` of future
intervals, and must commit `c(I_t)` before step t+1. Commitments are
irrevocable. The algorithm sees the *endpoints* of the buffered intervals; it
never sees their colours, and it does not learn anything about intervals outside
the buffer.

`f_M(k)` is the infimum of `c` over algorithms with lookahead of size `k` in
model `M`. It is known that `f(0) = 3`: Kierstead–Trotter's algorithm attains
`3ω − 2` and no online algorithm does better
(see [references.md](references.md) §1).

---

## 3. Weak lookahead

> **WL-k.** `B_t = (I_{t+1}, …, I_{min(t+k, n)})` — the literal next k arrivals,
> whatever they are. Near the end of the sequence the buffer runs short.

Implemented by `lookahead_buffer(inst, t, k, Lookahead::Weak)`.

---

## 4. Strong lookahead

> **SL-k.** Let `R_t = (I_j : j > t, I_j ∩ I_t ≠ ∅)` be the future arrivals
> **relevant** to `I_t`, in arrival order. Then `B_t` is the first `k` elements
> of `R_t`, or all of `R_t` if `|R_t| < k`.

Implemented by `lookahead_buffer(inst, t, k, Lookahead::Strong)`.

Three decisions inside that definition carry the whole model. They are the
reason this file exists.

### 4.1 Relevance means "overlaps I_t", not "overlaps the active region"

The brief offered a second candidate: the next k intervals meeting the *active
region*, the union of intervals the sweep has not yet passed. Rejected, for two
reasons.

1. **It presumes a sweep the arrival order does not have.** The adversary
   chooses the arrival order, and it is not left-to-right; an adversary is free
   to open a region, abandon it, and return to it 500 requests later. "Not yet
   passed by the sweep" is then either everything or nothing, depending on how
   you define "passed", and both readings degenerate: everything makes SL-k
   equal to seeing the next k arrivals anywhere in the live part of the line
   (near-WL-k), nothing makes the buffer empty.
2. **It is not the quantity the decision depends on.** The colour of `I_t` is
   constrained by exactly the intervals adjacent to `I_t`. A model of "relevant"
   that admits intervals which can never constrain `c(I_t)` is smuggling
   padding back into the definition — the very thing strong lookahead exists to
   forbid.

The chosen definition is also the exact analogue of Albers' paging model, where
the window must contain ℓ distinct pages *other than the one being served*.

### 4.2 The buffer holds the relevant intervals, not the window containing them

Albers' paging buffer is a *prefix of the request sequence* that happens to
contain ℓ distinct pages, so the algorithm also sees the irrelevant requests
interleaved with them. Transplanting that literally here breaks the model.

Consider a tiny interval `S` that nothing later ever touches — `R_S = ∅`. Under
a prefix-based definition, "the shortest prefix containing k relevant intervals"
is *the entire remaining sequence*. An algorithm could then plant nothing at
all, wait for such an interval to arrive, and read off the whole future. The
model would be worth 1, not 3, for a silly reason.

So: `B_t` contains the relevant intervals and nothing else, and when fewer than
k exist the buffer is simply shorter (possibly empty). No prefix, no leak.

`Buffers.StrongLookaheadSkipsIrrelevantArrivals` and
`Buffers.RunsShortAtTheEndOfTheSequence` fix this in the test suite.

### 4.3 The buffer is a set of endpoints, not a schedule

The algorithm learns *which* intervals are coming, not *when*. It does not learn
how many irrelevant arrivals sit between them, and it does not learn `n`.
Anything else would leak information about the parts of the sequence the model
is supposed to be hiding.

---

## 5. Adaptive adversaries under lookahead

A lookahead model constrains the adversary as much as the algorithm. To show the
algorithm `B_t` at step t, the adversary must **commit** to k future intervals
before it has seen `c(I_t)`. A fully adaptive adversary in the k = 0 sense does
not exist for k ≥ 1.

Horizon handles this the way the proof does, rather than working around it:

1. `adversary(alg, ω, budget, seed)` builds an adaptive hard instance against
   `alg` at k = 0, where full adaptivity is available. It is restricted to
   **stub-friendly** instances (§6) — the class the padding transform applies to.
2. `pad_strong(inst, k)` (or `pad_weak`) transforms that instance into one where
   every buffer is a fixed function of what the algorithm has *already* been
   shown. The adversary can therefore commit to the buffer with no loss of power.
3. The harness replays the padded instance at lookahead k.

Step 2 is the construction from the lower bound, not a convenience. See
[proof_k1.md](proof_k1.md), Lemma 2 and Lemma 3.

---

## 6. Stub-friendliness

> An instance σ is **stub-friendly** if for every t there is a point
> `p_t ∈ I_t` covered by no `I_s` with `s > t`.

That is: no interval is entirely swallowed by its own future. This is the
hypothesis the strong-lookahead padding needs, and `adversary()` enforces it
while searching (`stub_friendly()` in `src/generators.cpp`). `pad_strong` reports
`leaks > 0` on any instance that violates it, so the condition is checked rather
than assumed on every run.

---

## 7. Validity is enforced, always

`run()` validates the colouring after every run and throws on failure —
in release builds too, not under `assert`. An invalid colouring that uses few
colours produces a beautiful and completely wrong competitive ratio, and that is
the one failure mode that would silently invalidate the entire project.
