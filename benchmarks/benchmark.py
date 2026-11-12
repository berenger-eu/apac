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
import tarfile
import time

ROOT = Path(__file__).resolve().parent
NAMES = ('fibonacci','quicksort','mergesort','nqueens','matmul','nbody','jacobi')
DEFAULT_CASES = tuple(name for name in NAMES if name != 'nqueens')
KINDS = dict(zip(NAMES, (0,1,2,3,5,6,7)))
# Sequential computational leaves are selected outside the human-readable input.
IGNORED = {
    'fibonacci': (),
    'quicksort': ('insertion_sort','partition'),
    'mergesort': ('insertion_sort','merge'),
    'nqueens': ('search_queens',),
    'matmul': ('multiply_tile',),
    'nbody': ('body_acceleration','accelerate_bodies','drift_body','drift_bodies','kick_body','kick_bodies'),
    'jacobi': ('relax_rows',),
}
ADAPTERS = {
    'fibonacci': 'return fibonacci(n);',
    'quicksort': 'quick_sort(keys,n,grain); return 0;',
    'mergesort': 'merge_sort(keys,aux,n,grain); return 0;',
    'nqueens': 'return nqueens(n);',
    'matmul': 'multiply_matrices(x,y,z,n,grain); return 0;',
    'nbody': 'simulate_particles(x,y,z,tmp,n,steps,grain,0.001); return 0;',
    'jacobi': 'jacobi(x,y,n,steps,grain); return 0;',
}
# size, grain, steps. Grain is unused only by Fibonacci and N-Queens.
# Runnable kernels are independent implementations; imported references are separate.
PROFILES = {
    'smoke': ((20,1,1),(257,16,1),(257,16,1),(8,1,1),(13,4,1),(17,4,3),(13,3,3)),
    'small': ((24,1,1),(65536,32,1),(65536,32,1),(11,1,1),(128,16,1),(1024,64,4),(256,16,20)),
    'standard': ((43,1,1),(1048576,32,1),(1048576,32,1),(13,1,1),(512,32,1),(8192,128,8),(1024,32,100)),
    'coarse': ((43,1,1),(1048576,32,1),(1048576,32,1),(13,1,1),(1024,128,1),(8192,128,8),(8192,128,20)),
}
PATTERNS = ('random','sorted','reverse','duplicates')
VARIANTS = ('apac-unlimited', 'apac-depth', 'apac-cost')
# The existing runtime accepts integer depth limits; INT_MAX makes the depth
# cutoff unreachable in practice, without adding model code to the baseline.
UNLIMITED_DEPTH = str(2**31-1)
DEFAULT_COST_THRESHOLDS = [100_000, 1_000_000, 100_000_000, 1_000_000_000]


def variant_environment(env, variant, cost_threshold):
    result = dict(env)
    result.pop('APAC_DEPTH_MAX', None)
    result['APAC_COST_THRESHOLD'] = '0'
    if variant in ('apac-unlimited', 'apac-cost'):
        result['APAC_DEPTH_MAX'] = UNLIMITED_DEPTH
    if variant == 'apac-cost':
        result['APAC_COST_THRESHOLD'] = str(cost_threshold)
    return result


ENTRY = r'''
#if defined(__GNUC__) && !defined(__clang__)
#define BENCH_ENTRY_ATTR __attribute__((noipa))
#else
#define BENCH_ENTRY_ATTR
#endif
extern "C" BENCH_ENTRY_ATTR unsigned long long bench_entry(int n,int grain,int steps,
    int *keys,int *aux,double *x,double *y,double *z,double *tmp) {
  BENCHMARK_CALL
}
'''


def save_json(path, value):
    path.write_text(json.dumps(value, indent=2, allow_nan=False)+'\n')


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class CommandTimeout(RuntimeError):
    """A single subprocess exceeded its wall-clock budget."""


def command(argv, *, cwd=None, env=None, timeout=300, log=None):
    argv = [str(value) for value in argv]
    try:
        result = subprocess.run(argv, cwd=cwd, env=env, text=True, capture_output=True, timeout=timeout)
    except subprocess.TimeoutExpired as error:
        if log:
            def decoded(value):
                return value.decode(errors='replace') if isinstance(value, bytes) else value or ''
            log.write_text(shlex.join(argv)+'\nTIMEOUT\n'+decoded(error.stdout)+decoded(error.stderr))
        raise CommandTimeout('command timed out: '+shlex.join(argv)) from error
    if log:
        log.write_text(shlex.join(argv)+'\n'+result.stdout+result.stderr)
    if result.returncode:
        raise RuntimeError(f'command exited {result.returncode}: {shlex.join(argv)}\n'
                           f'{result.stderr[-4000:]}\nLog: {log}')
    return result.stdout


def selected(args, available, defaults=None):
    names = args.cases or defaults or list(available)
    missing = set(names)-set(available)
    if missing:
        raise ValueError('benchmarks not available: '+', '.join(sorted(missing)))
    return list(dict.fromkeys(names))


def prepare(args):
    root = args.output_dir.resolve()
    archive = args.archive.resolve() if args.archive else None
    if archive is not None:
        if archive == root or root in archive.parents:
            raise ValueError('--archive must be outside the output directory')
        if archive.exists():
            raise ValueError(f'archive already exists: {archive}')
    if root.exists() and any(root.iterdir()):
        raise ValueError(f'prepare requires an empty output directory: {root}')
    root.mkdir(parents=True, exist_ok=True)
    build = args.build_dir.resolve()
    apac = build/'apac'
    if not apac.is_file():
        raise ValueError(f'APAC executable missing: {apac}')
    manifest = {'schema':2, 'complete':False, 'created_utc':datetime.now(timezone.utc).isoformat(),
                'benchmarks':{}, 'default_cases':[], 'case_order':NAMES, 'variants':VARIANTS, 'profiles':PROFILES, 'patterns':PATTERNS,
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
        command([apac,source,'--ignore='+','.join(IGNORED[name]),'--','-std=c++17'],cwd=folder,timeout=args.timeout,log=folder/'transform.log')
        if sha256(source)!=before:
            raise RuntimeError(f'APAC changed the original source: {source}')
        generated = folder/'APACkernel.cpp'
        if not generated.is_file() or '#pragma omp task ' not in generated.read_text():
            raise RuntimeError(f'APAC did not generate tasks for {name}; inspect {folder}')
        # Keep a source without estimator code for the unlimited/depth policies.
        base_text = generated.read_bytes()
        command([apac,source,'--ignore='+','.join(IGNORED[name]),'--static-cost','--','-std=c++17'],
                cwd=folder,timeout=args.timeout,log=folder/'transform-cost.log')
        if sha256(source)!=before:
            raise RuntimeError(f'APAC changed the original source: {source}')
        cost_generated = folder/'APACcost.cpp'
        generated.replace(cost_generated)
        generated.write_bytes(base_text)
        if '__apac_cost_threshold' not in cost_generated.read_text():
            raise RuntimeError(f'APAC did not generate the cost model for {name}')
        files = [source,generated,cost_generated,folder/'support.hpp',folder/'driver.hpp']
        files += sorted(folder.glob('_apac*.hpp'))
        manifest['benchmarks'][name] = {'kind':KINDS[name], 'adapter':ADAPTERS[name],
            'role':'sequential-search control' if name=='nqueens' else 'benchmark',
            'ignored_functions':IGNORED[name],
            'source':'kernel.cpp','transformed':'APACkernel.cpp','cost_transformed':'APACcost.cpp',
            'files':{path.name:sha256(path) for path in files}}
        save_json(root/'manifest.json',manifest)
        print('PREPARED '+name+(' [sequential-search control]' if name=='nqueens' else ''),flush=True)
    shutil.copyfile(Path(__file__).resolve(),root/'run.py')
    shutil.copyfile(ROOT/'README.md',root/'README.md')
    for batch in ('apac-bora.sbatch','apac-suroit.sbatch'):
        shutil.copyfile(ROOT.parent/batch,root/batch)
    manifest['default_cases']=[name for name in DEFAULT_CASES if name in manifest['benchmarks']]
    manifest['complete']=True
    save_json(root/'manifest.json',manifest)
    print('Portable benchmark bundle: '+str(root),flush=True)
    if archive is not None:
        archive.parent.mkdir(parents=True, exist_ok=True)
        with tarfile.open(archive, 'x:gz') as bundle:
            bundle.add(root, arcname=root.name)
        print('Portable benchmark archive: '+str(archive),flush=True)


def statistics_for(samples, workloads, configurations, timeouts):
    result=[]
    failed={(r['workload'],r['variant'],r['threads'],r['cost_threshold']) for r in timeouts}
    for workload in workloads:
        records=[r for r in samples if r['workload']==workload]
        groups={(v,t,c):[r['seconds'] for r in records if (r['variant'],r['threads'],r['cost_threshold'])==(v,t,c)]
                for v,t,c in configurations}
        medians={key:statistics.median(values) for key,values in groups.items() if values}
        for variant,count,threshold in configurations:
            values=groups[(variant,count,threshold)]
            median=medians.get((variant,count,threshold))
            status=('partial' if values else 'timeout') if (workload,variant,count,threshold) in failed else 'ok'
            baseline=medians.get(('sequential',1,0))
            own_baseline=medians.get((variant,1,threshold))
            result.append({'workload':workload,'variant':variant,'threads':count,'samples':len(values),
                'status':status,'median_seconds':median,
                'min_seconds':min(values) if values else None,'max_seconds':max(values) if values else None,
                'stdev_seconds':statistics.stdev(values) if len(values)>1 else 0 if values else None,
                'speedup_vs_sequential':baseline/median if baseline is not None and median is not None else None,
                'speedup_vs_variant_1':own_baseline/median if variant!='sequential' and own_baseline is not None and median is not None else None,
                'depth_limit':1+(count-1).bit_length() if variant=='apac-depth' else None,
                'cost_threshold':threshold,
                'short_measurement':median<0.01 if median is not None else None})
    return result


def run(args):
    root = args.output_dir.resolve()
    manifest = json.loads((root/'manifest.json').read_text())
    if manifest.get('schema')!=2 or manifest.get('complete') is not True:
        raise ValueError('unsupported or incomplete benchmark bundle; prepare a new bundle with this runner')
    names = selected(args, manifest['benchmarks'], manifest.get('default_cases'))
    if any(value is not None for value in (args.size,args.grain,args.steps)) and len(names)!=1:
        raise ValueError('--size/--grain/--steps require exactly one --cases benchmark')
    variants = list(dict.fromkeys(args.variants))
    if set(variants)-set(manifest['variants']):
        raise ValueError('requested variants are not available in this bundle')
    thresholds=list(dict.fromkeys(args.cost_thresholds))
    if any(not 0 < value < 2**64-1 for value in thresholds):
        raise ValueError('--cost-thresholds must be positive and below ULLONG_MAX')
    threads = list(dict.fromkeys([1]+args.threads))
    if any(value<1 for value in threads) or args.repeat<1 or args.warmups<0:
        raise ValueError('thread counts and repeat must be positive; warmups must be nonnegative')
    configurations=[('sequential',1,0)]+[(v,t,c) for v in variants
        for c in (thresholds if v=='apac-cost' else [0]) for t in threads]
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
        'environment':{key:value for key,value in env.items() if key.startswith(('OMP_','GOMP_','APAC_'))
                       or key in ('SLURM_JOB_ID','SLURM_CPUS_PER_TASK','SLURM_CPUS_ON_NODE')},
        'variants':variants,'cost_thresholds':thresholds,
        'variant_environment':{v+(f':{c}' if v=='apac-cost' else ''):variant_environment({},v,c)
                               for v,t,c in configurations},
        'runner_sha256':sha256(Path(__file__)),'profile':args.profile,'threads':threads,'warmups':args.warmups,'repeat':args.repeat,
        'timeout_seconds':args.timeout,
        'measurement':'kernel + task synchronization; excludes setup, oracle and OpenMP team creation'}
    if shutil.which('lscpu'):
        metadata['lscpu']=command(['lscpu','--json'])
    cpuinfo = Path('/proc/cpuinfo')
    if cpuinfo.exists():
        metadata['cpu_model'] = next((line.split(':',1)[1].strip() for line in cpuinfo.read_text().splitlines()
                                      if line.startswith('model name')),None)
    report = {'schema':2,'complete':False,'bundle':manifest,'metadata':metadata,'samples':[],'summary':[],'timeouts':[],'all_verified':False}
    workloads=[]
    try:
        with (result_dir/'samples.csv').open('w',newline='') as stream:
            fields=['workload','benchmark','profile','size','grain','steps','seed','pattern','variant','threads',
                    'depth_limit','cost_threshold','repeat','seconds','wall_seconds','verified','scalar','checksum','team_threads','max_abs_error','thread_affinity']
            writer=csv.DictWriter(stream,fieldnames=fields); writer.writeheader()
            for name in names:
                info=manifest['benchmarks'][name]; folder=root/name
                for filename,expected in info['files'].items():
                    if sha256(folder/filename)!=expected:
                        raise RuntimeError(f'prepared source changed: {folder/filename}; prepare a new bundle')
                for variant in ['sequential',*variants]:
                    key = 'source' if variant=='sequential' else 'cost_transformed' if variant=='apac-cost' else 'transformed'
                    source=binaries/(name+'-'+variant+'.cpp')
                    source.write_text((folder/info[key]).read_text()+'\n'+ENTRY.replace('BENCHMARK_CALL',info.get('adapter','return benchmark_kernel(n,grain,steps,keys,aux,x,y,z,tmp);')))
                    harness=binaries/(name+'-'+variant+'-driver.cpp')
                    harness.write_text('#include "support.hpp"\n#include "driver.hpp"\n')
                    command([args.compiler,'-std=c++17','-fopenmp',*flags,'-I',folder,
                             '-DBENCH_KIND='+str(info['kind']),'-DBENCH_PARALLEL='+str(int(variant!='sequential')),
                             source,harness,'-o',binaries/(name+'-'+variant)],
                            timeout=args.timeout,log=binaries/(name+'-'+variant+'-compile.log'))
                    if shutil.which('ldd'):
                        metadata['linked_libraries'][name+'-'+variant]=command(['ldd',binaries/(name+'-'+variant)])
                profile_index=manifest['case_order'].index(name) if 'case_order' in manifest else info['kind']
                size,grain,steps=manifest['profiles'][args.profile][profile_index]
                size=args.size if args.size is not None else size
                grain=args.grain if args.grain is not None else grain
                steps=args.steps if args.steps is not None else steps
                patterns=PATTERNS if name=='quicksort' and args.pattern=='all' else [
                    'random' if args.pattern=='all' else args.pattern]
                for pattern in patterns:
                    workload=name+'/'+pattern; workloads.append(workload)
                    expected_integer=None
                    timed_out=set()
                    def sample(variant,count,threshold,repeat,warmup):
                        nonlocal expected_integer
                        process_env={**variant_environment(env,variant,threshold),'OMP_NUM_THREADS':str(count)}
                        start=time.perf_counter()
                        if (variant,count,threshold) in timed_out:
                            return
                        try:
                            output=command([binaries/(name+'-'+variant),size,grain,steps,args.seed,PATTERNS.index(pattern)],
                                env=process_env,timeout=args.timeout,
                                log=result_dir/f'{name}-{pattern}-{variant}-{threshold}-{count}-{repeat}-{"warmup" if warmup else "sample"}.log')
                        except CommandTimeout as error:
                            timed_out.add((variant,count,threshold))
                            report['timeouts'].append({'workload':workload,'benchmark':name,'profile':args.profile,
                                'size':size,'grain':grain,'steps':steps,'seed':args.seed,'pattern':pattern,
                                'variant':variant,'threads':count,'cost_threshold':threshold,'repeat':repeat,'warmup':warmup,
                                'timeout_seconds':args.timeout,'error':str(error)})
                            save_json(result_dir/'report.json',report)
                            print(f'TIMEOUT {workload} {variant} threshold={threshold} {count} threads: skipping remaining repetitions of this configuration',flush=True)
                            return
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
                                 'depth_limit':1+(count-1).bit_length() if variant=='apac-depth' else None,
                                 'cost_threshold':threshold,
                                 'repeat':repeat,'wall_seconds':wall,**record}
                            report['samples'].append(row)
                            writer.writerow({**row,'thread_affinity':json.dumps(row['thread_affinity'],separators=(',',':'))})
                            stream.flush()
                    for variant,count,threshold in configurations:
                        for warmup in range(args.warmups): sample(variant,count,threshold,warmup,True)
                    for repeat in range(args.repeat):
                        # Rotate order to reduce a fixed bias from temperature/frequency drift.
                        order=configurations[repeat%len(configurations):]+configurations[:repeat%len(configurations)]
                        for variant,count,threshold in order: sample(variant,count,threshold,repeat,False)
                    report['summary']=statistics_for(report['samples'],workloads,configurations,report['timeouts'])
                    save_json(result_dir/'report.json',report)
                    print(('FINISHED WITH TIMEOUTS ' if timed_out else 'VERIFIED ')+workload,flush=True)
            report['complete']=True
            report['all_verified']=not report['timeouts']
    except BaseException as error:
        report['error']=str(error)
        raise
    finally:
        save_json(result_dir/'report.json',report)
        if report['summary']:
            with (result_dir/'summary.csv').open('w',newline='') as summary_stream:
                summary_writer=csv.DictWriter(summary_stream,fieldnames=list(report['summary'][0]))
                summary_writer.writeheader(); summary_writer.writerows(report['summary'])
        print('Benchmark results: '+str(result_dir),flush=True)
    for row in report['summary']:
        label=row['variant']+(f"[{row['cost_threshold']}]" if row['variant']=='apac-cost' else '')
        if row['median_seconds'] is None:
            print(f'{row["workload"]:24s} {label:22s} {row["threads"]:3d} threads TIMEOUT')
            continue
        speedup=f'{row["speedup_vs_sequential"]:.3f}x' if row['speedup_vs_sequential'] is not None else 'n/a'
        print(f'{row["workload"]:24s} {label:22s} {row["threads"]:3d} threads '
              f'{row["median_seconds"]:.6f}s speedup={speedup}'
              +(' [partial: timeout]' if row['status']=='partial' else '')
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
            item.add_argument('--static-cost',action='store_true',help=argparse.SUPPRESS) # Legacy flag: cost source is now always generated.
            item.add_argument('--archive',type=Path,help='create a .tar.gz bundle outside --output-dir')
            item.add_argument('--build-dir',type=Path,default=Path(os.environ.get('APAC_BUILD_DIR',ROOT.parent/'build')))
        if action in ('run','all'):
            item.add_argument('--variants',nargs='+',choices=VARIANTS,default=list(VARIANTS))
            item.add_argument('--cost-thresholds','--cost-threshold',dest='cost_thresholds',nargs='+',type=int,default=DEFAULT_COST_THRESHOLDS,help='cost thresholds to measure (default: 100000 1000000 100000000 1000000000)')
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
