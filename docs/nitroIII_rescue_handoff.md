# Handoff brief — for the Claude Code session running ON nitroIII

> ⚠️ **Superseded by 2026-06-03 correction:** any "saturation at d=2k+2" or
> obstruction-ceiling claim here is an ARTIFACT of an interval bug (fixed in
> commit 22a7121). See [`modular_obstruction_design.md`](modular_obstruction_design.md) correction notice
> and [`session_2026-06-03_cliff_discovery.md`](session_2026-06-03_cliff_discovery.md). Results at d ≤ 2k remain valid.

You are now running on **nitroIII** (AMD Zen2 production compute box). A separate
Claude Code session is open on **nitroII** (Intel dev box) holding the full project
history and memory; treat that as the coordinator. Your job here is hands-on.

## Who / what
- User: jim. Project: **bi-quad** — a modular-obstruction search (C + GMP + OpenMP)
  hunting obstructions to bi-quadratic emirps `p = 2n²+2n+1`.
- nitroII = Intel dev box (BunsenLabs). Canonical git repo + Claude memory live there.
- nitroIII = THIS machine, AMD Zen2, Fedora 42. Runs the long (weeks-long) jobs.

## The problem you're solving
- nitroIII ran a ~6-week `d=21 / k=12` `mod_obstruct` job. It ran 2+ months with **no
  OS updates applied**. The run was killed; on the **next boot the system drops to the
  emergency/rescue shell**.
- WiFi has been brought up manually. `systemctl start sshd` was failing — expected, because
  in the rescue/emergency target networking + most services aren't started. Not a real
  sshd fault.

## Goals, in priority order
1. **Diagnose** why boot lands in emergency. Top suspects, in order:
   - root (`/`) **full** — a 2-month run filling logs/output;
   - root mounted **read-only** / needs **fsck** after the unclean kill;
   - an **fstab** mount failure (changed UUID / secondary disk absent / missing `nofail`).
2. **Repair in place.** The box still boots (to rescue), so this is almost certainly an
   in-place fix. **Do NOT reinstall or wipe.** Recover before discarding — these runs
   take weeks.
3. **Back up `/home/jim` to an external HD** — do this **locally** (plug exthd into
   nitroIII, `rsync`), no network/ssh needed.

## First commands to run (paste output back / act on it)
```bash
df -h /                         # full root?
df -i /                         # inodes exhausted?
mount | grep ' / '              # is it 'ro'?  -> mount -o remount,rw /
systemctl --failed
journalctl -xb -p err --no-pager | tail -80
cat /etc/fstab
findmnt --verify
dmesg | grep -iE 'ext4|xfs|i/o error|read-only|recovery' | tail
```
If `/` is full → free space (old run logs/output), then `systemctl default` or `reboot`.
If root is `ro`/corrupt → `fsck` (best done from the live USB with root unmounted), or boot
with kernel arg `fsck.mode=force`, then reboot.

## Backup (local, no ssh)
```bash
# plug in exthd; find where it mounted (e.g. /run/media/jim/<LABEL> or /media/jim/<LABEL>)
lsblk -f
rsync -aHv --info=progress2 /home/jim/ /run/media/jim/<EXTHD-LABEL>/nitroIII-home-backup/
```
Grab the bi-quad source tree, its `logs/`, results, and any `*.ckpt` checkpoint files.

## Project state (reference — repo lives on nitroII)
- Branch `master_dev` @ `47917ff`. `mod_obstruct.c` has 3 **committed, validated** fixes:
  1. survivor counter `int → long` (overflowed at k≥10; `-1074376056` was a wrap);
  2. composite-5 prime-eligibility filter on **both** the p-side and q-side (endings in 5
     are divisible by 5 → never prime). This **changes results** vs old "dirty" runs;
  3. saturation level corrected to `3·mod/5` (not `mod`) so the skip fires.
- The killed `d=21/k=12` *dirty* count is recoverable as **3,220,591,240** (display
  artifact, not lost compute). A clean re-run will be lower but still ≫ 0.
- Build: `gcc -O3 -march=znver2 -mtune=znver2 -std=c99 -fopenmp ... -lgmp`.

## Constraints
- No wipe/reinstall without explicit OK. The Fedora 44 (Cinnamon) live USB being made on
  nitroII is a **rescue/backup tool** and possible future **in-place** upgrade medium
  (`dnf system-upgrade` 42→44 preserves everything) — not a fresh install.
- Confirm with jim before anything destructive or hard to reverse.
