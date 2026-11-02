#!/usr/bin/env python3
"""Prepare original APAC benchmarks, then run a portable verified timing suite."""
import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import shlex
import shutil
import statistics
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
NAMES = ('fibonacci','quicksort','mergesort','nqueens','dotproduct','matmul','nbody','jacobi')
# size, grain, steps; grain is a leaf size, except Fibonacci's n cutoff and
# N-Queens' search depth cutoff. No benchmark input is borrowed from BOTS.
PROFILES = {
    'smoke': ((20,8,1),(257,16,1),(257,16,1),(8,3,1),(257,16,1),(13,4,1),(17,4,3),(13,3,3)),
    'small': ((30,15,1),(65536,32,1),(65536,32,1),(11,4,1),(262144,1024,1),(128,16,1),(256,32,4),(256,16,20)),
    'standard': ((40,20,1),(1048576,32,1),(1048576,32,1),(13,5,1),(8388608,4096,1),(512,32,1),(2048,64,8),(1024,32,100)),
}
PATTERNS = ('random','sorted','reverse','duplicates')

ENTRY = r'''
#if defined(__GNUC__) && !defined(__clang__)
#define BENCH_ENTRY_ATTR __attribute__((noipa))
#else
#define BENCH_ENTRY_ATTR
#endif
extern "C" BENCH_ENTRY_ATTR unsigned long long bench_entry(int n,int grain,int steps,
    int *keys,int *aux,double *x,double *y,double *z,double *tmp) {
  return benchmark_kernel(n,grain,steps,keys,aux,x,y,z,tmp);
}
'''


def save_json(path, value):
    path.write_text(json.dumps(value, indent=2, allow_nan=False)+'\n')


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def command(argv, *, cwd=None, env=None, timeout=300, log=None):
    argv = [str(value) for value in argv]
    try:
        result = subprocess.run(argv, cwd=cwd, env=env, text=True, capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired as error:
        if log:
            def decoded(value):
                return value.decode(errors='replace') if isinstance(value, bytes) else value or ''
            log.write_text(shlex.join(argv)+'\nTIMEOUT\n'+decoded(error.stdout)+decoded(error.stderr))
        raise RuntimeError('command timed out: '+shlex.join(argv)) from error
    if log:
        log.write_text(shlex.join(argv)+'\n'+result.stdout+result.stderr)
    if result.returncode:
        raise RuntimeError(f'command exited {result.returncode}: {shlex.join(argv)}\n'
                           f'{result.stderr[-4000:]}\nLog: {log}')
    return result.stdout


def selected(args, available):
    names = args.cases or list(available)
    missing = set(names)-set(available)
    if missing:
        raise ValueError('benchmarks not available: '+', '.join(sorted(missing)))
    return list(dict.fromkeys(names))


def prepare(args):
    root = args.output_dir.resolve()
    if root.exists() and any(root.iterdir()):
        raise ValueError(f'prepare requires an empty output directory: {root}')
    root.mkdir(parents=True, exist_ok=True)
    build = args.build_dir.resolve()
    apac = build/'apac'
    if not apac.is_file():
        raise ValueError(f'APAC executable missing: {apac}')
    manifest = {'schema':1, 'complete':False, 'created_utc':datetime.now(timezone.utc).isoformat(),
                'benchmarks':{}, 'profiles':PROFILES, 'patterns':PATTERNS,
                'apac_sha256':sha256(apac), 'runner_sha256':sha256(Path(__file__))}
    try:
        manifest['git_commit'] = command(['git','rev-parse','HEAD'],cwd=ROOT).strip()
        manifest['git_dirty'] = bool(command(['git','status','--porcelain'],cwd=ROOT).strip())
    except RuntimeError:
        manifest['git_commit'] = None
    for name in selected(args, NAMES):
        folder = root/name
        folder.mkdir()
        for header in ('support.hpp','driver.hpp'):
            shutil.copyfile(ROOT/'common'/header,folder/header)
        source = folder/'kernel.cpp'
        shutil.copyfile(ROOT/'kernels'/(name+'.cpp'),source)
        before = sha256(source)
        command([apac,source,'--','-std=c++17'],cwd=folder,timeout=args.timeout,log=folder/'transform.log')
        if sha256(source)!=before:
            raise RuntimeError(f'APAC changed the original source: {source}')
        generated = folder/'APACkernel.cpp'
        if not generated.is_file() or '#pragma omp task ' not in generated.read_text():
            raise RuntimeError(f'APAC did not generate tasks for {name}; inspect {folder}')
        files = [source,generated,folder/'support.hpp',folder/'driver.hpp']
        files += sorted(folder.glob('_apac*.hpp'))
        manifest['benchmarks'][name] = {'kind':NAMES.index(name),
            'source':'kernel.cpp','transformed':'APACkernel.cpp',
            'files':{path.name:sha256(path) for path in files}}
        save_json(root/'manifest.json',manifest)
        print('PREPARED '+name,flush=True)
    shutil.copyfile(Path(__file__).resolve(),root/'run.py')
    shutil.copyfile(ROOT/'README.md',root/'README.md')
    manifest['complete']=True
    save_json(root/'manifest.json',manifest)
    print('Portable benchmark bundle: '+str(root),flush=True)


def statistics_for(samples, workloads, threads):
    result=[]
    for workload in workloads:
        records=[row for row in samples if row['workload']==workload]
        medians={}
        for variant,count in [('sequential',1)]+[('apac',value) for value in threads]:
            values=[row['seconds'] for row in records if row['variant']==variant and row['threads']==count]
            if values:
                medians[(variant,count)]=statistics.median(values)
        for (variant,count),median in medians.items():
            values=[row['seconds'] for row in records if row['variant']==variant and row['threads']==count]
            result.append({'workload':workload,'variant':variant,'threads':count,'samples':len(values),
                'median_seconds':median,'min_seconds':min(values),'max_seconds':max(values),
                'stdev_seconds':statistics.stdev(values) if len(values)>1 else 0,
                'speedup_vs_sequential':medians[('sequential',1)]/median,
                'speedup_vs_apac_1':medians.get(('apac',1),median)/median if variant=='apac' else None,
                'short_measurement':median<0.01})
    return result


def run(args):
    root = args.output_dir.resolve()
    manifest = json.loads((root/'manifest.json').read_text())
    if manifest.get('schema')!=1 or manifest.get('complete') is not True:
        raise ValueError('unsupported or incomplete benchmark bundle')
    names = selected(args, manifest['benchmarks'])
    if any(value is not None for value in (args.size,args.grain,args.steps)) and len(names)!=1:
        raise ValueError('--size/--grain/--steps require exactly one --cases benchmark')
    threads = list(dict.fromkeys([1]+args.threads))
    if any(value<1 for value in threads) or args.repeat<1 or args.warmups<0:
        raise ValueError('thread counts and repeat must be positive; warmups must be nonnegative')
    result_dir = root/'results'/datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    result_dir.mkdir(parents=True)
    binaries = result_dir/'bin'; binaries.mkdir()
    flags = shlex.split(args.cxxflags)
    env = os.environ.copy()
    env['OMP_DYNAMIC']='FALSE'
    env.setdefault('OMP_PROC_BIND','close')
    env.setdefault('OMP_PLACES','cores')
    env['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
    metadata = {'platform':platform.platform(),'machine':platform.machine(),'python':sys.version,
        'cpu_count':os.cpu_count(),'cpu_affinity':sorted(os.sched_getaffinity(0)) if hasattr(os,'sched_getaffinity') else None,
        'compiler':command([args.compiler,'--version']).splitlines()[0],
        'compiler_path':shutil.which(args.compiler),
        'flags':['-std=c++17','-fopenmp',*flags], 'linked_libraries':{},
        'environment':{key:value for key,value in env.items() if key.startswith(('OMP_','GOMP_'))
                       or key in ('SLURM_JOB_ID','SLURM_CPUS_PER_TASK','SLURM_CPUS_ON_NODE')},
        'runner_sha256':sha256(Path(__file__)),'profile':args.profile,'threads':threads,'warmups':args.warmups,'repeat':args.repeat,
        'measurement':'kernel + task synchronization; excludes setup, oracle and OpenMP team creation'}
    if shutil.which('lscpu'):
        metadata['lscpu']=command(['lscpu','--json'])
    cpuinfo = Path('/proc/cpuinfo')
    if cpuinfo.exists():
        metadata['cpu_model'] = next((line.split(':',1)[1].strip() for line in cpuinfo.read_text().splitlines()
                                      if line.startswith('model name')),None)
    report = {'schema':1,'complete':False,'bundle':manifest,'metadata':metadata,'samples':[],'summary':[]}
    workloads=[]
    configurations=[('sequential',1)]+[('apac',value) for value in threads]
    try:
        with (result_dir/'samples.csv').open('w',newline='') as stream:
            fields=['workload','benchmark','profile','size','grain','steps','seed','pattern','variant','threads',
                    'repeat','seconds','wall_seconds','verified','scalar','checksum','team_threads','max_abs_error']
            writer=csv.DictWriter(stream,fieldnames=fields); writer.writeheader()
            for name in names:
                info=manifest['benchmarks'][name]; folder=root/name
                for filename,expected in info['files'].items():
                    if sha256(folder/filename)!=expected:
                        raise RuntimeError(f'prepared source changed: {folder/filename}; prepare a new bundle')
                for variant,key in [('sequential','source'),('apac','transformed')]:
                    source=binaries/(name+'-'+variant+'.cpp')
                    source.write_text((folder/info[key]).read_text()+'\n'+ENTRY)
                    harness=binaries/(name+'-'+variant+'-driver.cpp')
                    harness.write_text('#include "support.hpp"\n#include "driver.hpp"\n')
                    command([args.compiler,'-std=c++17','-fopenmp',*flags,'-I',folder,
                             '-DBENCH_KIND='+str(info['kind']),'-DBENCH_PARALLEL='+str(int(variant=='apac')),
                             source,harness,'-o',binaries/(name+'-'+variant)],
                            timeout=args.timeout,log=binaries/(name+'-'+variant+'-compile.log'))
                    if shutil.which('ldd'):
                        metadata['linked_libraries'][name+'-'+variant]=command(['ldd',binaries/(name+'-'+variant)])
                size,grain,steps=manifest['profiles'][args.profile][info['kind']]
                size=args.size if args.size is not None else size
                grain=args.grain if args.grain is not None else grain
                steps=args.steps if args.steps is not None else steps
                patterns=PATTERNS if name=='quicksort' and args.pattern=='all' else [
                    'random' if args.pattern=='all' else args.pattern]
                for pattern in patterns:
                    workload=name+'/'+pattern; workloads.append(workload)
                    expected_integer=None
                    def sample(variant,count,repeat,warmup):
                        nonlocal expected_integer
                        process_env={**env,'OMP_NUM_THREADS':str(count)}
                        start=time.perf_counter()
                        output=command([binaries/(name+'-'+variant),size,grain,steps,args.seed,PATTERNS.index(pattern)],
                            env=process_env,timeout=args.timeout,
                            log=result_dir/f'{name}-{pattern}-{variant}-{count}-{repeat}-{"warmup" if warmup else "sample"}.log')
                        wall=time.perf_counter()-start
                        record=json.loads(output)
                        if record.get('verified') is not True or not isinstance(record.get('seconds'),(int,float)) or \
                           not math.isfinite(record['seconds']) or record['seconds']<=0:
                            raise RuntimeError(f'invalid/unverified measurement for {workload}: {record}')
                        if record['team_threads']!=count:
                            raise RuntimeError(f'OpenMP supplied {record["team_threads"]} threads instead of {count}; check allocation/OMP_THREAD_LIMIT')
                        if info['kind']<4:
                            identity=(record['scalar'],record['checksum'])
                            if expected_integer is None: expected_integer=identity
                            if expected_integer!=identity: raise RuntimeError(f'inconsistent integer result: {workload}')
                        if not warmup:
                            row={'workload':workload,'benchmark':name,'profile':args.profile,'size':size,'grain':grain,
                                 'steps':steps,'seed':args.seed,'pattern':pattern,'variant':variant,'threads':count,
                                 'repeat':repeat,'wall_seconds':wall,**record}
                            report['samples'].append(row); writer.writerow(row); stream.flush()
                    for variant,count in configurations:
                        for warmup in range(args.warmups): sample(variant,count,warmup,True)
                    for repeat in range(args.repeat):
                        # Rotate order to reduce a fixed bias from temperature/frequency drift.
                        order=configurations[repeat%len(configurations):]+configurations[:repeat%len(configurations)]
                        for variant,count in order: sample(variant,count,repeat,False)
                    report['summary']=statistics_for(report['samples'],workloads,threads)
                    save_json(result_dir/'report.json',report)
                    print('VERIFIED '+workload,flush=True)
            report['complete']=True
    except BaseException as error:
        report['error']=str(error)
        raise
    finally:
        save_json(result_dir/'report.json',report)
        print('Benchmark results: '+str(result_dir),flush=True)
    for row in report['summary']:
        print(f'{row["workload"]:24s} {row["variant"]:10s} {row["threads"]:3d} threads '
              f'{row["median_seconds"]:.6f}s speedup={row["speedup_vs_sequential"]:.3f}x'
              +(' [short measurement]' if row['short_measurement'] else ''))
    return result_dir


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest='action',required=True)
    for action in ('prepare','run','all'):
        item=sub.add_parser(action)
        item.add_argument('--output-dir',type=Path,required=True)
        item.add_argument('--cases',nargs='+',choices=NAMES)
        item.add_argument('--timeout',type=int,default=300)
        if action in ('prepare','all'):
            item.add_argument('--build-dir',type=Path,default=Path(os.environ.get('APAC_BUILD_DIR',ROOT.parent/'build')))
        if action in ('run','all'):
            item.add_argument('--profile',choices=PROFILES,default='small')
            item.add_argument('--threads',nargs='+',type=int,default=[1,2,4,8])
            item.add_argument('--repeat','--iterations','-n',type=int,default=5)
            item.add_argument('--warmups',type=int,default=1)
            item.add_argument('--compiler',default='g++')
            item.add_argument('--cxxflags',default='-O2')
            item.add_argument('--size',type=int)
            item.add_argument('--grain',type=int)
            item.add_argument('--steps',type=int)
            item.add_argument('--seed',type=int,default=17)
            item.add_argument('--pattern',choices=('all',*PATTERNS),default='all')
    args=parser.parse_args()
    if args.timeout<1: parser.error('--timeout must be positive')
    try:
        if args.action in ('prepare','all'): prepare(args)
        if args.action in ('run','all'): run(args)
    except (OSError,ValueError,RuntimeError,KeyError) as error:
        print('benchmark: '+str(error),file=sys.stderr)
        return 1
    return 0

if __name__=='__main__':
    sys.exit(main())
