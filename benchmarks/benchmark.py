#!/usr/bin/env python3
"""
Benchmark APAC: mesure le speedup entre execution sequentielle et parallele.
Usage: python3 benchmark.py [--iterations N]

Peut etre lance directement depuis la racine du projet apres un build local
ou depuis /workspace dans le conteneur Docker.
"""

import subprocess
import time
import sys
import os
import statistics
import argparse
import shutil
import tempfile

BENCHMARK_DIR = os.path.dirname(os.path.abspath(__file__))
WORKSPACE = os.path.dirname(BENCHMARK_DIR)  # /workspace
BUILD_DIR = os.path.join(WORKSPACE, "build")
APAC_BIN = os.path.join(BUILD_DIR, "apac")

# Benchmark sources
BENCHMARKS = {
    "readOnlyParam": os.path.join(BENCHMARK_DIR, "readOnlyParam", "bench.cpp"),
    "dotProduct": os.path.join(BENCHMARK_DIR, "dotProduct", "bench.cpp"),
    "callChain": os.path.join(BENCHMARK_DIR, "callChain", "bench.cpp"),
}


def run_cmd(cmd, cwd=None, timeout=300):
    start = time.perf_counter()
    result = subprocess.run(
        cmd, capture_output=True, text=True, cwd=cwd, timeout=timeout
    )
    elapsed = time.perf_counter() - start
    return result.stdout, result.stderr, result.returncode, elapsed


def compile_sequential(src, output):
    cmd = ["g++", "-O2", "-std=c++17", "-o", output, src]
    _, stderr, rc, _ = run_cmd(cmd)
    if rc != 0:
        print(f"  ERREUR compilation sequentielle:\n{stderr}")
        return False
    return True


def compile_parallel(src, output):
    cmd = ["g++", "-O2", "-std=c++17", "-fopenmp", "-o", output, src]
    _, stderr, rc, _ = run_cmd(cmd)
    if rc != 0:
        print(f"  ERREUR compilation parallele:\n{stderr}")
        print(f"  stderr: {stderr}")
        return False
    return True


def transform_with_apac(src, work_dir):
    """Run APAC on source file and returns path to transformed file."""
    base = os.path.basename(src)
    dest = os.path.join(work_dir, base)
    shutil.copy2(src, dest)
    
    cmd = [APAC_BIN, dest, "--"]
    _, stderr, rc, _ = run_cmd(cmd, cwd=work_dir)
    if rc != 0:
        print(f"  ERREUR APAC:\n{stderr}")
        return None
    
    transformed = os.path.join(work_dir, "APAC" + base)
    if not os.path.exists(transformed):
        print(f"  ERREUR: fichier transforme {transformed} non trouve")
        return None
    with open(transformed, 'r') as f:
        content = f.read()
    content = content.replace('\nreturn 0;\n', '\n')
    with open(transformed, 'w') as f:
        f.write(content)
    return transformed


def run_benchmark(executable, iterations=5, env=None):
    """Run the benchmark multiple times and return timing statistics."""
    times = []
    for i in range(iterations):
        start = time.perf_counter()
        stdout, stderr, rc, elapsed = run_cmd([executable], timeout=120)
        if rc != 0:
            print(f"  ERREUR execution (run {i+1}): rc={rc}")
            print(f"  stderr: {stderr}")
            return None
        times.append(elapsed)
    
    return {
        "mean": statistics.mean(times),
        "median": statistics.median(times),
        "stdev": statistics.stdev(times) if len(times) > 1 else 0,
        "min": min(times),
        "max": max(times),
        "times": times,
    }


def print_results(name, seq_stats, par_stats):
    """Print bench results."""
    print(f"\n{'='*60}")
    print(f"  BENCHMARK: {name}")
    print(f"{'='*60}")
    
    if seq_stats:
        print("\n  Sequentiel:")
        print(f"    Moyenne : {seq_stats['mean']:.3f}s")
        print(f"    Mediane : {seq_stats['median']:.3f}s")
        print(f"    Min/Max : {seq_stats['min']:.3f}s / {seq_stats['max']:.3f}s")
    
    if par_stats:
        print("\n  Parallele APAC:")
        print(f"    Moyenne : {par_stats['mean']:.3f}s")
        print(f"    Mediane : {par_stats['median']:.3f}s")
        print(f"    Min/Max : {par_stats['min']:.3f}s / {par_stats['max']:.3f}s")
    
    if seq_stats and par_stats:
        speedup = seq_stats['mean'] / par_stats['mean']
        print(f"\n  SPEEDUP: {speedup:.2f}x")
        if speedup < 1.0:
            print("  - Le code parallele est PLUS LENT")
        elif speedup > 1.5:
            print("  - Acceleration significative")
        else:
            print("  - Acceleration bof")
    
    print(f"{'='*60}\n")
    return seq_stats['mean'] / par_stats['mean'] if seq_stats and par_stats else None


def main():
    parser = argparse.ArgumentParser(description="Benchmark APAC")
    parser.add_argument("--iterations", "-n", type=int, default=5,
                        help="Nombre d'iterations par benchmark (default: 5)")
    parser.add_argument("--threads", "-t", type=int, default=None,
                        help="Nombre de threads OpenMP (default: auto)")
    args = parser.parse_args()
    
    print("=" * 60)
    print("  APAC BENCHMARK")
    print("=" * 60)
    
    # Check APAC binary
    if not os.path.exists(APAC_BIN):
        print(f"ERREUR: binaire APAC non trouve: {APAC_BIN}")
        sys.exit(1)
    
    # CPU info
    try:
        nproc = int(subprocess.check_output(["nproc"]).strip())
        print(f"  CPUs dispo: {nproc}")
    except Exception:
        nproc = 1
    
    if args.threads:
        os.environ["OMP_NUM_THREADS"] = str(args.threads)
        print(f"  OMP_NUM_THREADS: {args.threads}")
    else:
        print(f"  OMP_NUM_THREADS: {os.environ.get('OMP_NUM_THREADS', 'auto')}")
    
    print(f"  Iterations: {args.iterations}")
    
    all_speedups = {}
    
    for name, src in BENCHMARKS.items():
        print(f"\n--- {name} ---")
        
        work_dir = tempfile.mkdtemp(prefix=f"apac_bench_{name}_")
        
        seq_bin = os.path.join(work_dir, f"{name}_seq")
        par_bin = os.path.join(work_dir, f"{name}_par")
        
        # Compile sequential
        print("  Compilation sequentielle")
        if not compile_sequential(src, seq_bin):
            continue
        
        # Transform with APAC
        transformed = transform_with_apac(src, work_dir)
        if not transformed:
            seq_stats = run_benchmark(seq_bin, args.iterations)
            print_results(name, seq_stats, None)
            continue
        
        # Compile parallel
        print("  Compilation parallele")
        if not compile_parallel(transformed, par_bin):
            seq_stats = run_benchmark(seq_bin, args.iterations)
            print_results(name, seq_stats, None)
            continue
        
        # Run benchmarks
        seq_stats = run_benchmark(seq_bin, args.iterations)
        
        par_env = os.environ.copy()
        par_stats = run_benchmark(par_bin, args.iterations, env=par_env)
        
        # Results
        speedup = print_results(name, seq_stats, par_stats)
        if speedup:
            all_speedups[name] = speedup
    
    # Summary
    if all_speedups:
        print("\n" + "=" * 60)
        print("  RESUME")
        print("=" * 60)
        for name, speedup in all_speedups.items():
            print(f"  {name}: {speedup:.2f}x")
        print("=" * 60)


if __name__ == "__main__":
    main()
