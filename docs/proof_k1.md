# Lower bounds under bounded lookahead

All notation is from [model.md](model.md). All algorithms are deterministic.
`f_M(k)` is the best competitive ratio achievable with lookahead k in model M,
and `f(0) = 3` is Kierstead–Trotter ([references.md](references.md) §1).

**Summary of what is and is not established here.**

| Claim | Status |
| --- | --- |
| Theorem 1: `f_weak(k) = 3` for every constant k | Proved |
| Lemma 3: strong-lookahead padding is information-free on stub-friendly instances | Proved |
| Theorem 2: strong lookahead k buys nothing against stub-friendly constructions | Proved |
| Corollary 3: `f_strong(k) = 3` for every constant k | **Conjecture** — needs the Kierstead–Trotter lower-bound construction to be stub-friendly, which I could not check (§5) |

---

## 1. The shape of the argument

Both theorems are simulation arguments with the same skeleton.

Take an algorithm `A` that has lookahead. Build a padded sequence `π(σ)` in which
every buffer `A` ever sees is a **fixed function of the requests `A` has already
been shown**. Then a lookahead-free algorithm `A'` can run `A` inside itself:
`A'` always knows enough to compute `A`'s buffer, so it can feed `A` the padded
sequence and echo back `A`'s answers on the real intervals. `A'` is a legal
zero-lookahead algorithm, so the Kierstead–Trotter lower bound applies to it, and
`A` inherits it.

The whole content is in choosing padding that (i) fills the buffer, (ii) tells
the algorithm nothing, and (iii) does not move ω.

Rescale, once and for all: an interval system's graph and clique number are
invariant under any strictly increasing map of the line, and any finite adaptive
construction can be carried out on dyadic rationals, so we may assume every
adversarial construction lives inside `[0, 1]`.

---

## 2. Theorem 1 — weak lookahead is useless

> **Theorem 1.** For every constant `k ≥ 0`, `f_weak(k) = 3`.

**Upper bound.** Kierstead–Trotter ignores the buffer and is 3-competitive, so
`f_weak(k) ≤ 3`.

**Lower bound.** Let `A` be any deterministic algorithm with WL-k lookahead.

*The padding.* Fix `M = 2`, to the right of every construction. Define dummies

```
D_t^j  =  [ M + 2c , M + 2c + 1 ],     c = (t-1)k + j - 1
```

so that all dummies are pairwise disjoint and disjoint from `[0,1]`. For a
sequence `σ = (I_1, …, I_n)` in `[0,1]` put

```
π(σ)  =  I_1, D_1^1, …, D_1^k,  I_2, D_2^1, …, D_2^k,  …,  I_n, D_n^1, …, D_n^k.
```

**Lemma 1.** (a) `ω(π(σ)) = ω(σ)`. (b) Under WL-k, the buffer of `I_t` in `π(σ)`
is exactly `(D_t^1, …, D_t^k)`. (c) The buffer of any dummy contains at most one
interval of `σ`, namely `I_{t+1}`.

*Proof.* (a) The dummies are pairwise disjoint and disjoint from `[0,1]`, so
every clique they belong to has size 1, and `ω ≥ 1` always. (b) `I_t` sits at
position `(t-1)(k+1) + 1` and is followed immediately by its own k dummies. (c) A
dummy `D_t^j` is followed by `D_t^{j+1}, …, D_t^k, I_{t+1}, D_{t+1}^1, …`. Reaching
`I_{t+2}` from `D_t^j` would need a window of length at least `k + 2`, and the
window has length k. ∎

Part (b) is the point: `D_t^1, …, D_t^k` depend only on `t`. When `A` commits
`c(I_t)`, its buffer carries **zero** bits about the future of `σ`.

*The simulation.* Define a zero-lookahead algorithm `A'` on `σ`. `A'` maintains a
simulated copy of `A` and a queue of dummies it has not yet fed in. On receiving
`I_t`:

1. Feed `A` the pending dummies `D_{t-1}^1, …, D_{t-1}^k` (for `t ≥ 2`). Their
   buffers, by Lemma 1(c), mention at most `I_t` and dummies — all of which `A'`
   now knows. Discard the colours `A` returns.
2. Feed `A` the interval `I_t` with buffer `(D_t^1, …, D_t^k)`, which `A'` can
   compute from `t` alone.
3. Output whatever colour `A` returned for `I_t`.

At the end, feed the last block of dummies with truncated buffers.

The deferral in step 1 is what makes this legal: `A'` never needs to know
anything it has not already been given. `A'` is a deterministic zero-lookahead
algorithm.

*The count.* `A`'s colouring of `π(σ)` is valid, so its restriction to `σ` — which
is exactly `A'`'s output — is valid too, and

```
ALG_A(π(σ))  >=  #{ colours A uses on the intervals of σ }  =  ALG_A'(σ).
```

By Kierstead–Trotter there is an adaptive adversary that forces `A'` to use at
least `3ω − 2` colours on some `σ`. Running that adversary and applying `π`
gives a sequence on which `A` uses at least `3ω(π(σ)) − 2` colours. Hence
`f_weak(k) ≥ 3`. ∎

**Remark.** The adversary is adaptive and `A` is deterministic, so "there is a
σ" is constructive: the adversary simulates `A'`, which simulates `A`.

**Remark (why this is the small result).** Lemma 1(b) is the entire argument, and
it is available because WL-k lets the adversary put anything at all in the
buffer. This is the interval-colouring twin of Albers' observation for paging
([references.md](references.md) §3), and it is what motivates the strong model.

---

## 3. Strong lookahead: the padding that survives SL-k

Under SL-k the adversary can no longer park the padding at infinity — the buffer
must overlap `I_t`. The replacement is to nest the padding *inside* `I_t`, in a
place the rest of the construction will never touch.

Recall (model.md §6) that σ is **stub-friendly** if every `I_t` contains a point
`p_t` covered by no `I_s` with `s > t`.

**Lemma 2 (existence of stubs).** Let σ be stub-friendly. For each t let `G_t` be
the widest connected component of `I_t \ ⋃_{s>t} I_s`; by hypothesis `|G_t| > 0`.
Place k closed intervals `S_t^1, …, S_t^k` inside `G_t`, centred at
`inf G_t + j·w` with `w = |G_t| / (k+1)` and half-width `w/4`. Then:

- each `S_t^j ⊂ G_t ⊂ I_t`, so `S_t^j` overlaps `I_t`;
- `S_t^1, …, S_t^k` are pairwise disjoint (spacing `w`, width `w/2`);
- no `I_s` with `s > t` overlaps any `S_t^j`, by definition of `G_t`;
- for `t < t'`, `S_t^j ∩ S_{t'}^{j'} = ∅`, because `S_{t'}^{j'} ⊂ I_{t'}` and
  `S_t^j` avoids `I_{t'}`.

*Proof:* each bullet is the sentence after it. The arithmetic for the second:
the rightmost stub ends at `w·k + w/4 = |G_t|·(4k+1)/(4k+4) < |G_t|`, and the
leftmost begins at `w − w/4 > 0`. ∎

Now define

```
π_k(σ)  =  I_1, S_1^1, …, S_1^k,  I_2, S_2^1, …, S_2^k,  …,  I_n, S_n^1, …, S_n^k.
```

**Lemma 3 (the padding is information-free).** For stub-friendly σ:

**(a)** `ω(π_k(σ)) ≤ ω(σ) + 1`.

**(b)** Under SL-k, the buffer of `I_t` is exactly `(S_t^1, …, S_t^k)`, and the
buffer of every stub is **empty**.

**(c)** Hence every buffer in `π_k(σ)` is a fixed function of `I_1, …, I_t`.

*Proof.* (a) A point inside some stub is covered by that one stub — stubs are
pairwise disjoint — plus the intervals of σ covering it, at most `ω(σ)`. A point
inside no stub is covered only by intervals of σ. So `ω(π_k(σ)) ≤ ω(σ) + 1`.

(b) The intervals of `π_k(σ)` that overlap `I_t` and arrive after it are:
`S_t^1, …, S_t^k` (immediately), possibly some later `I_s`, and possibly stubs of
later intervals. The first k in arrival order are the k stubs of `I_t`. For a
stub `S_t^j`: by Lemma 2 nothing arriving after it overlaps it — not the sibling
stubs (pairwise disjoint), not any later `I_s` (excluded by `G_t`), not any later
stub (contained in a later `I_s`). So `R_{S_t^j} = ∅` and, by model.md §4.2, the
buffer is empty rather than "the rest of the sequence".

(c) `S_t^1, …, S_t^k` are determined by `I_t` and by `⋃_{s>t} I_s`. The second is
future information — but the adversary, not the algorithm, computes it, and it
commits to the stubs at the moment it presents `I_t`. What the algorithm sees is
determined by the intervals it has already been shown, which is what the
simulation in §4 needs. ∎

Part (b) is where model.md §4.2 earns its keep. Under a prefix-style buffer
definition, a stub's buffer would be the whole remaining sequence and Lemma 3
would be false — catastrophically, not marginally.

---

## 4. Theorem 2 — strong lookahead is useless on stub-friendly constructions

> **Theorem 2.** Let `A` be deterministic with SL-k lookahead, and let `X` be any
> adaptive adversary that produces stub-friendly sequences and forces every
> zero-lookahead algorithm to use at least `g(ω)` colours at clique number ω.
> Then `A` can be forced to use at least `g(ω)` colours on an instance of clique
> number at most `ω + 1`.

*Proof.* Define a zero-lookahead algorithm `A'`: on receiving `I_t`, feed `A`
the interval `I_t` with buffer `(S_t^1, …, S_t^k)`, output `A`'s colour, then
feed `A` the stubs `S_t^1, …, S_t^k` with empty buffers (Lemma 3(b)) and discard
their colours.

For this to be legal `A'` must be able to compute the stubs at step t, i.e. know
`⋃_{s>t} I_s`. It can, because `A'` runs *against the adversary `X`*, and `X`
knows its own future: formally, run `X` against `A'` and let `X` supply the stub
positions along with each `I_t`. No deferral is needed here — unlike Theorem 1,
no buffer in `π_k(σ)` mentions anything `A'` has not been shown.

`A`'s colouring of `π_k(σ)` restricted to σ is a valid colouring of σ and is
exactly `A'`'s output, so `A` uses at least as many colours on `π_k(σ)` as `A'`
does on σ, which is at least `g(ω)` by hypothesis. Lemma 3(a) bounds the clique
number of the instance actually served by `ω + 1`. ∎

> **Corollary 3 (conjecture).** If the Kierstead–Trotter lower-bound family is
> stub-friendly, then `f_strong(k) = 3` for every constant k: taking
> `g(ω) = 3ω − 2` gives `ALG ≥ 3ω − 2` on an instance of clique number `ω' ≤ ω + 1`,
> so `ALG ≥ 3ω' − 5`, and the ratio tends to 3.

So: **the answer to the central question is expected to be no.** `f(1) = f(0) = 3`,
and `f(k) = 3` for every constant k. Lookahead should only start to pay when it
scales with ω — exactly as in paging, where ℓ-strong lookahead is worthless until
ℓ is a constant fraction of the cache size ([references.md](references.md) §3).

---

## 5. What is missing, and why it is stated as a conjecture

Theorem 2 is conditional on the hard family being stub-friendly. Congressus
Numerantium 33 is not available online, so I could not check whether the
Kierstead–Trotter construction has that property, and I will not assert it from a
secondary source that does not describe the construction.

Two honest routes to closing it:

1. **Get the 1981 paper** and check whether every interval in the construction
   retains a point untouched by its own future. If the construction refines
   downward — later intervals nested inside earlier ones, which is the usual
   shape for such adversaries — it is stub-friendly and Corollary 3 is a theorem.
2. **Prove a `3ω − o(ω)` lower bound within the stub-friendly class directly.**
   Stub-friendliness is a mild-looking restriction (it forbids only total
   swallowing), and Theorem 2 then applies unconditionally.

Route 2 is the better use of time: it is self-contained, and it is what the
simulator can actually attack.

---

## 6. What the simulator measured

**The simulator's job here is falsification, not discovery.** A flat curve only
counts as evidence if the same setup can produce a non-flat one, so the central
experiment has three arms: one algorithm, one family of adversarial instances,
`k` swept over two orders of magnitude, and the padding switched on and off.

Measured quantity is `colours_original / omega_base` — the colours spent on the
intervals of the underlying hard instance, against that instance's own optimum.
Twenty adversarial instances per cell, `results/sweep_*.csv`.

| buffer | k=0 | k=1 | k=2 | k=4 | k=8 | k=16 | k=32 | k=64 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| honest (`pad=none`) | 1.400 | 1.200 | 1.200 | 1.200 | **1.000** | 1.000 | 1.000 | 1.000 |
| padded, strong | 1.400 | 1.400 | 1.400 | 1.400 | 1.400 | 1.400 | 1.400 | 1.400 |
| padded, weak | 1.400 | 1.400 | 1.400 | 1.400 | 1.400 | 1.400 | 1.400 | 1.400 |

Row 1 is the control and it is the load-bearing one: given an honest buffer the
algorithm reaches the offline optimum. Rows 2 and 3 are Lemma 1 and Lemma 3 —
the padding removes exactly that gain, and `leaks = 0` throughout, so no stub was
ever overlapped by its own future.

Two further controls:

- **Full information.** Under the weak model with `k = n` the lookahead algorithm
  scores exactly 1.0000 on every random instance tested — provably, since its
  first window is the whole instance and left-endpoint First-Fit is optimal
  offline. Pinned by `Colourers.LookaheadIsOfflineOptimalGivenTheWholeFuture`.
- **Negative control.** First-Fit and Kierstead–Trotter ignore the buffer by
  construction and stay flat in *all three* arms, including the honest one
  (1.400 and 1.800 at `k = 0, 8, 64`). The movement in row 1 is the algorithm
  using its buffer, not an artefact of the harness.

### Two failed attempts, kept because they were informative

**The first lookahead algorithm did not use its buffer.** It ranked legal
colours by the highest index First-Fit would then need on the buffer, which
First-Fit almost always already minimises, so the tie-break essentially never
fired. Given `k = n` it still scored 1.2 where the optimum was 1.0. Every test in
the suite passed anyway — none of them asserted that lookahead beats First-Fit.
That is why the control tests in §6 exist.

**The obvious repair made it worse.** Since left-endpoint First-Fit is optimal
offline, sorting `{current} ∪ buffer` that way and reading off `current`'s colour
looks right. It regressed to 1.4 unpadded and 1.5 on clustered instances under
full lookahead. The reason: the offline sweep is optimal only when applied
consistently to a whole instance. Recomputing a fresh window sweep at every
arrival optimises each step for an ordering that never happened, and fights the
colours already committed.

The fix is to make the plan **persistent**: colour the whole window, *reserve*
the results, and honour a reservation when that interval arrives rather than
re-planning. Consistency, not a better objective, was the missing ingredient.

### What the simulator does not do

`adversary()` is a beam search, not the Kierstead–Trotter construction. Given a
clique cap it forces roughly `ω + 1` to `ω + 2` colours — against First-Fit
6 colours at `ω = 4`, 8 at `ω = 6`; against Kierstead–Trotter 9 at `ω = 5` and 11
at `ω = 6`. Beam search beats the greedy version it replaced, which plateaued at
`ω + 1`, but widening the beam past about 12 stops helping.

It does not come near `3ω − 2`, and it does not reproduce the `5ω` family for
First-Fit. That is local search failing on constructions that need long runs of
locally neutral preparatory moves — **not** evidence against the cited bounds,
and arguably a small piece of evidence for why those proofs are as intricate as
they are. Anyone extending this should implement the Chrobak–Ślusarek family
explicitly rather than expect a search to find it.

So the absolute ratios in `results/` are lower bounds on the true worst case,
nothing more. The three-arm comparison is the measurement that carries weight.

---

## 7. Exhaustive verification on small parameters

`bench/solve.cpp` searches the whole adversary-versus-algorithm game tree rather
than measuring one algorithm. On a grid of `cells` elementary cells it answers
exactly whether the adversary can drive **every** online algorithm past a given
palette size.

Two observations make it tractable. The future depends on the past only through
which colours live in which cell, so the state is one bitmask per cell rather
than a list of intervals; and the game is symmetric under renaming colours, so
states are canonicalised by first appearance before memoising. Restricting the
adversary to a finite grid only ever makes it weaker, so a "forced" answer is a
genuine lower bound on the unrestricted game.

### Result 1 — Kierstead–Trotter is exactly right at ω = 2

| palette | grid | verdict |
| --- | --- | --- |
| 3 | 7 cells, 8 moves | **forced** — every online algorithm needs ≥ 4 |
| 4 | 7 cells, 10 moves | survivable — some algorithm holds out |

So the forcible count at `ω = 2` is **exactly 4 = 3ω − 2**, both directions, by
exhaustion over every strategy for both players. This matters beyond the number:
§1 of [references.md](references.md) could only cite the Kierstead–Trotter lower
bound at *secondary* confidence, because Congressus Numerantium 33 is not
available online. For `ω = 2` it is now verified independently of that source.

Eight intervals suffice; seven do not.

### Result 2 — lookahead does not change it, and the proof said why

Running the same query at `k = 1` on the **same** 7-cell grid reports
*survivable*: the adversary can no longer force 4. Taken at face value that
contradicts Theorem 1.

It does not, and the resolution is the one the proof hands you. The padding in
Lemma 1 needs somewhere to put its dummies — pairwise disjoint, and disjoint from
the construction. Intervals never expire, so `k` dummies per decision consume
grid cells that the real construction also needs. At 7 cells the adversary
cannot afford the padding, so it is playing a strictly weaker game than the
theorem describes.

Give it the room and the effect disappears:

| k | grid | verdict |
| --- | --- | --- |
| 0 | 7 cells, 8 moves | forced |
| 1 | 7 cells, 11 moves | survivable — too little room for the padding |
| 1 | **9 cells, 14 moves** | **forced** |
| 2 | up to 10 cells, 17 moves | inconclusive (node budget) |

The forcible count at `ω = 2` is 4 with lookahead as well as without. Lookahead
bought the algorithm two extra cells' worth of construction and nothing else.

This is the discipline the brief asked for, playing out for real: the simulator
disagreed with the proof, and the proof was right. Checking the simulator first
would have produced a false negative and a wasted week.

### What it cannot do

`ω = 3` needs a wider grid than the search can afford — at 7 cells it reaches
only 5 of the expected 7 — and `k = 2` is inconclusive at every grid tried. The
state space is roughly `(1 + ω·palette)^cells`, so this method is confined to
`ω = 2` and small `k`. It is a spot check on the smallest interesting case, not
a general verification.
