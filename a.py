#!/usr/bin/env python3
import subprocess, time, csv
from pathlib import Path
from statistics import mean, stdev

import pandas as pd
import matplotlib.pyplot as plt


SIZES      = ["1K","10K","100K","1M","5M","10M"]  
TRIALS     = 3                                    
BIMDC      = Path("./BIMDC")                      
RECEIVED   = Path("./received")                   
CLIENT     = Path("./client")                     
RESULTS    = Path("./results")                    
FILENAME_PREFIX = "test_"                         
KEEP_FILES = False                                


def parse_size(s: str) -> int:
    s = s.strip().upper()
    mult = 1
    if s.endswith("K"): mult, s = 1024, s[:-1]
    elif s.endswith("M"): mult, s = 1024**2, s[:-1]
    elif s.endswith("G"): mult, s = 1024**3, s[:-1]
    return int(float(s) * mult)

def sh(cmd, **kw):
    return subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, **kw)

def truncate_file(path: Path, nbytes: int):
    r = sh(["truncate", "-s", str(nbytes), str(path)])
    if r.returncode != 0:
        raise RuntimeError(f"h")

def files_equal(a: Path, b: Path) -> bool:
    return sh(["diff", "-q", str(a), str(b)]).returncode == 0

def main():
    BIMDC.mkdir(parents=True, exist_ok=True)
    RESULTS.mkdir(parents=True, exist_ok=True)
    if not RECEIVED.exists():
        pass

    rows = []
    for size_label in SIZES:
        size_bytes = parse_size(size_label)
        name = f"{FILENAME_PREFIX}{size_label}.bin"
        src = BIMDC / name
        dst = RECEIVED / name

        truncate_file(src, size_bytes)

        times, oks = [], []
        for trial in range(1, TRIALS + 1):
            if dst.exists():
                try: dst.unlink()
                except Exception: pass

            t0 = time.perf_counter()
            proc = sh(["./client", "-f", name])
            t1 = time.perf_counter()
            elapsed = t1 - t0

            if proc.returncode != 0:
                ok = False
            else:
                if not dst.exists():
                    ok = False
                else:
                    ok = files_equal(src, dst)
                    if not ok:
                        pass


            times.append(elapsed)
            oks.append(ok)

        rows.append({
            "size_label": size_label,
            "size_bytes": size_bytes,
            "trials": TRIALS,
            "avg_time_s": mean(times),
            "std_time_s": stdev(times) if len(times) > 1 else 0.0,
            "min_time_s": min(times),
            "max_time_s": max(times),
            "success_rate": sum(oks) / len(oks),
        })

        if not KEEP_FILES:
            try: src.unlink()
            except Exception: pass
            if all(oks) and dst.exists():
                try: dst.unlink()
                except Exception: pass


    df = pd.DataFrame(rows).sort_values("size_bytes")
    csv_path = RESULTS / "transfer_times.csv"
    df.to_csv(csv_path, index=False)

    plt.figure()
    plt.plot(df["size_bytes"], df["avg_time_s"], marker="o")
    plt.xscale("log")
    plt.xlabel("File size (bytes)")
    plt.ylabel("Average transfer time (s)")
    plt.title("File Transfer Time vs File Size")
    plt.grid(True, which="both")
    png_path = RESULTS / "transfer_times.png"
    plt.tight_layout()
    plt.savefig(png_path, dpi=200)

    for r in rows:
        print(f"{r['size_label']:>6}  avg={r['avg_time_s']:.4f}s  "
              f"std={r['std_time_s']:.4f}s  min={r['min_time_s']:.4f}s  "
              f"max={r['max_time_s']:.4f}s  ok={int(r['success_rate']*100)}%")

if __name__ == "__main__":
    print("Running...")
    main()
    print("Done.")
