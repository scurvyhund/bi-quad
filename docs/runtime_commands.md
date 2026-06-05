# mod_obstruct Runtime Commands Reference

Quick reference for launching, monitoring, and diagnosing mod_obstruct runs.

---

## Starting a run

```bash
# Fresh run (defaults: max_d=50, max_k=6)
./mod_obstruct

# Specify max_d and max_k
./mod_obstruct 50 10

# Resume from specific k and d
./mod_obstruct 50 10 10 21

# Auto-resume from checkpoint (if mod_obstruct.ckpt exists and matches params)
./mod_obstruct 50 10

# Run in background with logging
nohup ./mod_obstruct 50 10 > logs/run.log 2>&1 &

# Run with output unbuffered (ensures log is always current)
stdbuf -oL ./mod_obstruct 50 10 > logs/run.log 2>&1 &
```

## Watching output

```bash
# Follow log in real time
tail -f logs/run_clean.log

# See last N lines
tail -20 logs/run_clean.log

# Search for obstructions
grep 'OBSTRUCTION' logs/run_clean.log

# Search for specific d values
grep 'd=21' logs/run_clean.log
```

## Process monitoring

```bash
# Is it running? Shows PID and command
pgrep -a mod_obstruct

# CPU and memory overview (all threads show as one process)
top -p $(pgrep mod_obstruct)

# One-shot snapshot: PID, RSS (KB), VSZ (KB)
ps -o pid,rss,vsz,%mem,%cpu,etime,comm -p $(pgrep mod_obstruct)

# Thread view — see individual OpenMP threads
ps -eLo pid,tid,%cpu,comm -p $(pgrep mod_obstruct)

# htop filtered to mod_obstruct (if htop installed)
htop -p $(pgrep mod_obstruct)
```

## Memory diagnostics

```bash
# System-wide memory and swap usage
free -h

# Swap usage for mod_obstruct specifically
grep VmSwap /proc/$(pgrep mod_obstruct)/status

# Full memory breakdown for the process
grep -E 'VmPeak|VmRSS|VmSwap|VmSize' /proc/$(pgrep mod_obstruct)/status

# Watch for swap activity (si/so columns = swap in/out per second)
# si/so should be 0 during normal operation; nonzero means thrashing
vmstat 1

# Memory map summary (shared libs, heap, stack per thread)
pmap -x $(pgrep mod_obstruct) | tail -5
```

## CPU diagnostics

```bash
# Per-core utilization (1-second intervals) — shows if all 8 cores pegged
mpstat -P ALL 1

# Load average (should be ~8.0 for 8 threads)
uptime

# I/O wait — high %iowait with low %user suggests swap thrashing
iostat 1
```

## Checkpoint

```bash
# View current checkpoint (next k,d to process)
cat mod_obstruct.ckpt

# Format: max_d max_k next_k next_d
# Example: "50 10 10 22" means next run resumes at k=10 d=22
```

## Killing / pausing

```bash
# Graceful pause (SIGSTOP) — freezes process, resume with CONT
kill -STOP $(pgrep mod_obstruct)
kill -CONT $(pgrep mod_obstruct)

# Kill (checkpoint saved for last completed d)
kill $(pgrep mod_obstruct)
```

## Quick health check (copy-paste one-liner)

```bash
# Shows: is it running, CPU%, memory, swap, and last log line
pgrep -a mod_obstruct && ps -o rss,%mem,%cpu,etime -p $(pgrep mod_obstruct) && grep VmSwap /proc/$(pgrep mod_obstruct)/status && tail -1 logs/run_clean.log
```

## Interpreting signs of trouble

| Symptom | Likely cause | Check with |
|---|---|---|
| Machine unresponsive, fans quiet | Swap thrashing (CPU waiting on disk) | `vmstat 1` — look for high si/so |
| 795%+ CPU, fans loud | Normal — all 8 cores working | `top` — expected behavior |
| RSS growing over days | GMP heap fragmentation | `grep VmRSS /proc/.../status` |
| Log stuck on one d value | Large d with many active residues | Normal for d near 2k+1 |
| Log shows "(saturated)" | n_range > mod, search done for this k | Expected at d = 2k+2 |
