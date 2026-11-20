# archive/ — retired sources

Kept for history and for the documents that cite them. **Not tools:**
nothing builds them, no Makefile target, no script calls them, and no
current result depends on them. Moved here 2026-11-19 by the bounds audit
(`docs/audit_2026-11.md`, Tier 2).

| file | why retired |
|---|---|
| `check_d21.c` | Single-d (d = 21) GMP enumerator. Never built; `hunt` covers d = 21 (`hunt 21 21`). |
| `palhunt_opt.c` | Superseded prime-palindrome hunter (`siege_plan` item 16). Carries the long-double leading-digit filter flagged as 2026-09-04 audit Finding 2 — **do not revive without removing it**. |
| `mod_obstruct_bkup.c` | 2026-09-06 local backup of `mod_obstruct.c`, previously git-ignored yet cited by docs. Carries the 4096 Hensel cap and `long long` overflow behind the false k=10 d=21 OBSTRUCTION — **do not revive**. Banner added; otherwise verbatim. |
| `palhunt_gmp.zig`, `palhunt_opt.zig` | Zig-vs-C experiment, 2026-06-06 (`docs/zig_experiment_2026-06-06.md`). Concluded. |

To build one anyway, compile it from this directory with `-I..` so it finds
the headers in the repo root.
