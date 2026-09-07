# References

Every entry below was checked against the literature before any benchmark was
written. Where I could not reach the primary source I say so explicitly and name
the secondary sources that state the result, because a benchmark calibrated
against a misremembered constant is worse than no benchmark.

Confidence key:

- **primary** — read the paper or its arXiv full text.
- **secondary** — the primary source is offline or paywalled; the statement is
  taken from at least two independent peer-reviewed papers that cite it, and the
  constants agree across them.

---

## 1. Kierstead–Trotter: the zero-lookahead optimum

> H. A. Kierstead and W. T. Trotter. *An extremal problem in recursive
> combinatorics.* Congressus Numerantium **33** (1981), 143–153.

**Confirmed:** the paper gives an online algorithm that colours any interval
graph with at most **3ω − 2** colours, and a matching lower bound: for every
ω there is a request sequence on which **every** online algorithm uses at least
3ω − 2 colours. So the zero-lookahead competitive ratio is exactly 3, and it is
achieved.

**Confidence: secondary.** Congressus Numerantium 33 is not available online. The
statement, with these exact constants and with the lower bound attributed to the
same paper, appears in:

- *Online coloring of short interval graphs and two-count interval graphs*,
  arXiv:2412.17193, Theorem 1.1 — "uses at most 3ω − 2 colors to color any
  interval graph with clique number at most ω", and "no online algorithm that
  uses less than 3ω − 2 colors on any interval graph with clique number ω".
- *A tight analysis of Kierstead–Trotter algorithm for online unit interval
  coloring*, arXiv:1609.09031 (IEICE Trans. E99-A(10), 2016) — restates both
  bounds for the general case before specialising to unit intervals.

**Operational form of the algorithm** (from arXiv:1609.09031, which reproduces
it in order to analyse it). Each arriving vertex `v_i` is first given a *level*:

```
level(v_i) = argmin { j : omega( G_{1,j}(i), v_i ) <= j }
```

where `G_{1,j}(i)` is the subgraph induced by the already-arrived vertices of
level at most `j`, and `omega(H, v)` is the size of the largest clique of
`H ∪ {v}` that contains `v`. The vertex is then coloured by First-Fit within
`G_{level,level}(i)` alone, over a palette private to that level.

Level 1 is by definition an independent set and needs 1 colour; every level
`j ≥ 2` needs at most 3; levels never exceed ω. Total: `1 + 3(ω − 1) = 3ω − 2`.

This is implemented verbatim in [`src/kierstead_trotter.cpp`](../src/kierstead_trotter.cpp).
The "at most 3 per level" claim is not assumed — the implementation throws if a
level ever needs a fourth colour, and the test suite exercises it on 13,000
instances without that firing.

**Also confirmed, and used as a sanity check:** on *unit* interval graphs the
same algorithm uses at most 3ω − 3, and that is tight (arXiv:1609.09031, the
paper's main result). Horizon does not restrict to unit intervals, so the
relevant bound here is 3ω − 2.

---

## 2. First-Fit on interval graphs

First-Fit is O(1)-competitive on interval graphs. The constant has been
tightened repeatedly; as of this writing the interval is **5 ≤ R ≤ 8**.

**Upper bound R ≤ 8:**

> N. S. Narayanaswamy and R. Subhash Babu. *A note on first-fit coloring of
> interval graphs.* Order **25**(1) (2008), 49–53.

The 8ω bound is also attributed to unpublished work of Brightwell, Kierstead and
Trotter. **Confidence: secondary** (Springer paywall); the constant 8 is stated
identically in arXiv:2412.17193 and in arXiv:1506.00192.

**Lower bound R ≥ 5:**

> H. A. Kierstead, D. A. Smith and W. T. Trotter. *First-fit coloring on
> interval graphs has performance ratio at least 5.* European Journal of
> Combinatorics **51** (2016), 236–254. arXiv:1506.00192.

**Confidence: primary** (arXiv full text). This superseded the earlier 4.99…
lower bounds; check for newer work before quoting 5 as final.

**Unit intervals:** First-Fit uses at most 2ω − 1 and there is a unit-interval
instance forcing exactly 2ω − 1.

> M. Chrobak and M. Ślusarek. *On some packing problems related to dynamic
> storage allocation.* RAIRO Inform. Théor. Appl. **22** (1988), 487–499.

**Confidence: secondary** (arXiv:2412.17193, Theorem 1.2).

The 2ω − 1 figure is what [`first_fit_worst_case_omega2()`](../src/generators.cpp)
reproduces at ω = 2, and what
`Colourers.FirstFitHitsKnownWorstCaseAtOmegaTwo` pins down. Horizon does **not**
reproduce the 5ω family — see [proof_k1.md](proof_k1.md), "What the simulator
does not do".

---

## 3. Lookahead in online algorithms

> S. Albers. *On the influence of lookahead in competitive paging algorithms.*
> Algorithmica **18**(3) (1997), 283–305. Conference version: ESA 1993, LNCS 726,
> 399–410.

**Confirmed:**

- **Weak lookahead** — seeing the literal next ℓ requests — does **not** improve
  the competitive ratio of paging. The adversary pads the buffer with requests
  the algorithm already knows the answer to (repeats of the current page), so the
  buffer carries no information about the decision at hand.
- **Strong lookahead** — seeing a window that contains ℓ *pairwise distinct pages
  different from the current one* — does improve it. Albers introduced the notion
  precisely to escape the weak model's triviality, and it was the first lookahead
  model with that property.

**Confidence: secondary** for the exact theorem statements (Springer paywall);
both claims are restated consistently in the Algorithmica abstract, in the ESA
proceedings entry, and in later surveys.

**The result Horizon's conjecture is modelled on**, and the reason the answer
here is expected to be negative for constant k:

> Z. Jiang, D. Panigrahi and K. Sun. *Online algorithms for weighted paging with
> predictions.* ICALP 2020; ACM Trans. Algorithms 18(4), 2022. arXiv:2006.09509.

For unweighted paging with ℓ-strong lookahead and ℓ ≤ k − 2 (k = cache size),
every deterministic algorithm is Ω(k − ℓ)-competitive and every randomised one
is Ω(log(k − ℓ))-competitive. **Confidence: primary** (arXiv full text). So even
*strong* lookahead of constant size buys nothing asymptotically — it only helps
once ℓ is a constant fraction of k, the parameter that controls the hardness.

Horizon's clique number ω plays the role of the cache size k. That analogy is
what [proof_k1.md](proof_k1.md) turns into a theorem for the interval-colouring
case.

---

## 4. Prior work on the question this project actually asks

This section exists because the sections above only verify the *ingredients*.
The first thing anyone should ask of a project like this is what is already
known about lookahead in online **colouring** specifically, and the honest
answer is: more than the framing above implies.

> S. Albers and S. Schraink. *Tight Bounds for Online Coloring of Basic Graph
> Classes.* ESA 2017, LIPIcs 87; Algorithmica **82** (2020). arXiv:1702.07172.

**Confidence: primary** (arXiv abstract, verified directly). For general online
graph colouring the `Θ(n / log^2 n)` guarantee "cannot be improved if an online
algorithm has a lookahead of size `O(n / log n)` or access to a reordering buffer
of size `n^(1-ε)`, for any `0 < ε ≤ 1`."

So the headline conclusion here — *lookahead does not help online colouring* — is
**not new in spirit**. Albers and Schraink established it for general graphs, and
with far larger lookahead than any constant `k`. Note also that it is Albers
again: the same author who introduced strong lookahead for paging.

> S. Irani. *Coloring inductive graphs on-line.* Algorithmica **11** (1994),
> 53–72. FOCS 1990.

**Confidence: secondary.** For `d`-inductive graphs, deterministic online
algorithms with lookahead `ℓ` have best competitiveness `Θ(min{log n, n/ℓ})`.
Interval graphs with clique number `ω` are `(ω-1)`-inductive, so this is adjacent
— but the regime is different. On `d`-inductive graphs First-Fit already needs
`Θ(d log n)` colours, whereas on interval graphs it is `O(1)`-competitive, so
Irani's bounds do not specialise to anything useful here.

> W. Kordecki and A. Łyczkowska-Hańćkowiak. *Greedy online colouring with
> buffering.* arXiv:1601.00252.

**Confidence: primary** (full text read). Treats small reordering buffers
(`b = 1..4`) on crown graphs and Kneser graphs, with simulations. Does **not**
cover interval graphs and does not distinguish weak from strong lookahead.

### Where that leaves this project

What I did **not** find, after searching for it, is a treatment of:

- online colouring of **interval graphs** specifically under lookahead, and
- the **weak versus strong** lookahead distinction transplanted from paging, and
- **constant** `k`, rather than lookahead growing with `n`.

That is the gap this project sits in, and it is a narrow one. The correct
framing is *not* "is this an open question" — it is "the general answer is known
to be negative; does the perfect, `O(1)`-competitive case behave the same way,
and does Albers' own strong/weak distinction change anything here?" The answer
found here is that it does not, which is consistent with the general result
rather than independent of it.

Absence of evidence is not evidence of absence: I searched, I did not find, and
that is all I can claim. Anyone taking this further should do a proper citation
search on Albers–Schraink and on Kierstead–Trotter before asserting novelty.

---

## 5. Background used without needing verification

Interval graphs are perfect, so χ = ω, and for an interval system ω is the
maximum number of intervals covering a single point. This is standard
(Hajós 1957 / Gilmore–Hoffman 1964) and is the basis of
[`exact_chromatic_number`](../src/offline.cpp). It is not taken on trust in the
code either: `Offline.ExhaustiveTinyMatchesBruteForce` and
`Offline.RandomSmallMatchesBruteForce` check the sweep against a backtracking
graph-colouring solver that knows nothing about intervals.
