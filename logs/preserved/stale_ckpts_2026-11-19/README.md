# Stale checkpoints, preserved 2026-11-19

Moved here from the repo root by the bounds audit
(`docs/audit_2026-11.md`, remediation review). Neither can be resumed:
the checkpoint formats and file names changed on 2026-11-19, and both
tools now report a file of the old name as legacy and never read it.
Kept for what they record. Byte-identical to the originals
(`SHA256SUMS`); mtimes preserved by `mv`.

| file | written | what it records |
|---|---|---|
| `palbrute_d31.ckpt` | 2026-09-05 20:09 | An interrupted **d=31 lead-1** `palbrute` sweep. Old format `zone n_next found visited`: `0 716761781186548 1 9655000000000` — zone slot 0 (lead 1), next n = 716,761,781,186,548, **found = 1** so far, 9.655·10¹² n visited. |
| `mod_obstruct.ckpt` | 2026-08-29 23:52 | `50 6 6 16`: max_d=50, max_k=6, next k=6 d=16 — left by an interrupted k=6 sanity run (`50 6 6 11`). Keyed only by (max_d, max_k), it made a plain `./mod_obstruct 50 6` start at k=6 d=16, skipping k=3..5 (fixed `7320ce5`). |
