# Regenerates every CSV in this directory. Run from the repo root:
#   pwsh -File results/reproduce.ps1
# Requires build/bench.exe (see README). Takes about a minute.
$ErrorActionPreference = "Stop"
$bench = Join-Path $PSScriptRoot "..\build\bench.exe"
$out = $PSScriptRoot

function Run($file, $cliArgs) {
    & $bench @cliArgs ("--out=" + (Join-Path $out $file)) > $null
}

$adv = @("--gen=adversary", "--omega=5", "--n=120", "--beam=12", "--trials=20", "--seed=11")

# 1. Baselines, no lookahead, on random instances.
foreach ($algo in "first_fit", "kierstead_trotter", "lookahead") {
    Run "baseline_uniform_$algo.csv" @(
        "--algo=$algo", "--gen=random_uniform", "--n=1000", "--trials=100", "--k=0", "--seed=42")
    Run "baseline_clustered_$algo.csv" @(
        "--algo=$algo", "--gen=random_clustered", "--clusters=8", "--n=1000", "--trials=100",
        "--k=0", "--seed=42")
}

# 2. First-Fit on its own standard bad construction.
Run "ff_worst_case.csv" @("--algo=first_fit", "--gen=ff_worst", "--k=0", "--seed=42")

# 3. Adaptive adversary at zero lookahead, swept over the clique cap.
foreach ($algo in "first_fit", "kierstead_trotter", "lookahead") {
    foreach ($w in 3, 4, 5, 6) {
        Run "adversary_k0_${algo}_w$w.csv" @(
            "--algo=$algo", "--gen=adversary", "--omega=$w", "--n=120", "--beam=12",
            "--trials=20", "--k=0", "--seed=11")
    }
}

# 4. THE HEADLINE. One algorithm, one family of adversarial instances, three
#    treatments, k swept over two orders of magnitude.
#      pad=none   -> the buffer carries real information; the ratio must fall
#      pad=strong -> the padding transform must destroy exactly that gain
#      pad=weak   -> likewise, under the weak model
#    Without the pad=none arm the other two prove nothing: a broken algorithm
#    also produces flat lines.
foreach ($k in 0, 1, 2, 4, 8, 16, 32, 64) {
    Run "sweep_none_k$k.csv"   ($adv + @("--algo=lookahead", "--k=$k", "--pad=none",   "--model=strong"))
    Run "sweep_strong_k$k.csv" ($adv + @("--algo=lookahead", "--k=$k", "--pad=strong", "--model=strong"))
    Run "sweep_weak_k$k.csv"   ($adv + @("--algo=lookahead", "--k=$k", "--pad=weak",   "--model=weak"))
}

# 5. The same sweep for the two baselines, which ignore the buffer entirely and
#    so should be flat in every arm. The negative control for the control.
foreach ($algo in "first_fit", "kierstead_trotter") {
    foreach ($k in 0, 8, 64) {
        Run "sweep_none_${algo}_k$k.csv" ($adv + @("--algo=$algo", "--k=$k", "--pad=none", "--model=strong"))
    }
}

# 6. Lookahead against the whole future, on random instances. Must be exactly 1.
foreach ($gen in "random_uniform", "random_clustered") {
    Run "full_information_$gen.csv" @(
        "--algo=lookahead", "--gen=$gen", "--clusters=4", "--n=200", "--trials=40",
        "--k=200", "--model=weak", "--seed=42")
}

# 7. Exhaustive game solver. Not a CSV -- it prints verdicts. omega = 2 only;
#    anything larger exceeds the state space. See docs/proof_k1.md section 7.
$solve = Join-Path $PSScriptRoot "../build/solve.exe"
if (Test-Path $solve) {
    Write-Host "`n--- exhaustive game solver, omega = 2 ---"
    & $solve --cells=7 --omega=2 --colours=3 --k=0 --moves=8
    & $solve --cells=7 --omega=2 --colours=4 --k=0 --moves=10
    & $solve --cells=9 --omega=2 --colours=3 --k=1 --moves=14 --budget=90000000
}

Write-Host "done"
