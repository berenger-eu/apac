#!/usr/bin/env python3
"""Verify the portable benchmark bundle, numerical oracles and boundary inputs."""
import argparse
import csv
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile

ROOT=Path(__file__).resolve().parents[1]
NAMES=('fibonacci','quicksort','mergesort','nqueens','matmul','nbody','jacobi')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=Path(os.environ.get('APAC_BUILD_DIR',ROOT/'build')))
    args=parser.parse_args()
    work=Path(tempfile.mkdtemp(prefix='apac-benchmark-regression-'))
    def run(argv, *, env=None, expect=0):
        p=subprocess.run([str(x) for x in argv],cwd=work,capture_output=True,text=True,timeout=300,env=env)
        if p.returncode!=expect:
            raise AssertionError((argv,p.returncode,p.stdout[-5000:],p.stderr[-5000:]))
        return p.stdout
    try:
        bundle=work/'prepared'
        archive=work/'apac-benchmarks.tar.gz'
        run([sys.executable,ROOT/'benchmarks/benchmark.py','prepare','--output-dir',bundle,
             '--build-dir',args.build_dir.resolve(),'--archive',archive])
        # Use only the copied script and relative manifest paths. Running this
        # step does not need APAC or this checkout's sources on the target host.
        with tarfile.open(archive, 'r:gz') as packaged:
            packaged.extractall(work/'transported', filter='data')
        portable=work/'transported'/bundle.name
        for batch in ('apac-bora.sbatch','apac-suroit.sbatch'):
            assert (portable/batch).read_bytes()==(ROOT/batch).read_bytes()
        for name in NAMES:
            for filename in ('kernel.cpp','APACkernel.cpp','APACcost.cpp'):
                assert (portable/name/filename).read_bytes()==(bundle/name/filename).read_bytes()
        for name in NAMES:
            assert '__apac_cost_' not in (portable/name/'APACkernel.cpp').read_text()
            assert '__apac_cost_threshold' in (portable/name/'APACcost.cpp').read_text()
        print('PASS: archive contains the original and generated sources for all seven benchmarks',flush=True)
        for name,callee in (('matmul','multiply_tile'),('nbody','accelerate_bodies'),('jacobi','relax_rows')):
            source=(portable/name/'kernel.cpp').read_text()
            transformed=(portable/name/'APACkernel.cpp').read_text()
            assert 'for (' in source and callee in source,(name,source)
            assert 'benchmark_kernel' not in source and '_apacSeq' not in source,(name,source)
            assert '__apac_depth_guard __apac_loop_depth' in transformed,(name,transformed)
            assert re.search(r'__apac_loop_depth[^}]*'+callee,transformed),(name,transformed)
        print('PASS: readable standalone kernels generate dependent call tasks in loops',flush=True)
        run([sys.executable,portable/'run.py','run','--output-dir',portable,'--cases',*NAMES,'--profile','smoke',
             '--threads','1','2','4','--repeat','1','--warmups','0',
             '--cxxflags=-O1 -fsanitize=undefined -fno-sanitize-recover=all'],
             env={**os.environ,'APAC_BUILD_DIR':'/nonexistent-build','OMP_WAIT_POLICY':'PASSIVE','APAC_DEPTH_MAX':'0','APAC_COST_THRESHOLD':'invalid-inherited-value'})
        results=list((portable/'results').iterdir())
        assert len(results)==1
        report=json.loads((results[0]/'report.json').read_text())
        assert report['metadata']['environment']['APAC_DEPTH_MAX']=='0'
        variants=('apac-unlimited','apac-depth','apac-cost')
        assert report['metadata']['variants']==list(variants)
        policies=report['metadata']['variant_environment']
        assert 'APAC_DEPTH_MAX' not in policies['apac-depth']
        assert policies['apac-depth']['APAC_COST_THRESHOLD']=='0'
        assert policies['apac-unlimited']=={'APAC_DEPTH_MAX':str(2**31-1),'APAC_COST_THRESHOLD':'0'}
        assert report['metadata']['cost_thresholds']==[100000,1000000,100000000,1000000000]
        for threshold in (100000,1000000,100000000,1000000000):
            assert policies[f'apac-cost:{threshold}']=={'APAC_DEPTH_MAX':str(2**31-1),'APAC_COST_THRESHOLD':str(threshold)}
        for row in report['samples']:
            assert row['depth_limit']==(1+(row['threads']-1).bit_length() if row['variant']=='apac-depth' else None)
            assert row['cost_threshold'] in ((100000,1000000,100000000,1000000000) if row['variant']=='apac-cost' else (0,))
        assert all(row['speedup_vs_variant_1']==1 for row in report['summary'] if row['threads']==1 and row['variant']!='sequential')
        assert report['complete'] is True and report['all_verified'] is True and report['timeouts']==[]
        assert len(report['samples'])==190 and len(report['summary'])==190
        assert set(report['bundle']['benchmarks'])==set(NAMES)
        assert 'nqueens' not in report['bundle']['default_cases']
        assert 'dotproduct' not in report['bundle']['benchmarks']
        with (results[0]/'samples.csv').open() as stream:
            assert len(list(csv.DictReader(stream)))==190
        assert all(row['verified'] and row['team_threads']==row['threads'] for row in report['samples'])
        for row in report['samples']:
            affinity=row['thread_affinity']
            assert len(affinity)==row['threads']
            assert [a['thread'] for a in affinity]==list(range(row['threads']))
            if sys.platform.startswith('linux'):
                assert all(a['cpu'] in a['cpus'] for a in affinity)
                assert all(a['place']>=0 for a in affinity if row['variant']!='sequential')

        print('PASS: seven portable benchmarks, four sort distributions, 190 verified samples across three limiter policies with UBSan',flush=True)
        # Exercise empty/singleton buffers, odd splits, smallest grids and both
        # ping-pong parities. Timings here have no performance assertion.
        inputs={
            'fibonacci':[(n,1,1,17,0) for n in (0,1,2,15,25)],
            'quicksort':[(n,1,1,17,p) for n in (0,1,2,31) for p in range(4)],
            'mergesort':[(n,1,1,17,p) for n in (0,1,2,31) for p in range(4)],
            'nqueens':[(n,1,1,17,0) for n in (0,1,2,4,9)],
            'matmul':[(n,1,1,17,0) for n in (0,1,2,7)],
            'nbody':[(n,grain,steps,17,0) for n in (1,2,7) for grain in (1,2,4,16) for steps in (1,2,3)],
            'jacobi':[(n,1,steps,17,0) for n in (2,3,7) for steps in (1,2,3)],
        }
        sys.path.insert(0,str(portable))
        import run as runner
        checks=0
        for name,cases in inputs.items():
            for values in cases:
                baseline=None
                for variant,threads,threshold in [('sequential',1,0)]+[(v,t,c) for v in variants for c in ((100000,1000000,100000000,1000000000) if v=='apac-cost' else (0,)) for t in (1,4)]:
                    row=json.loads(run([results[0]/'bin'/(name+'-'+variant),*values],env={**runner.variant_environment(os.environ,variant,threshold),
                        'OMP_NUM_THREADS':str(threads),'OMP_DYNAMIC':'FALSE','OMP_WAIT_POLICY':'PASSIVE',
                        'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}))
                    assert row['verified'] is True,(name,values,row)
                    if variant=='sequential': baseline=(row['scalar'],row['checksum'])
                    if name in NAMES[:4]: assert baseline==(row['scalar'],row['checksum']),(name,values,row)
                    checks+=1
            print('PASS: benchmark boundaries '+name,flush=True)
        # Invalid arguments and failed verification must be fatal, rather than
        # entering the timing statistics. Corrupt a computed output in a small
        # original kernel and require the independent sort oracle to reject it.
        run([results[0]/'bin'/'fibonacci-sequential','-1','1','1','17','0'],expect=2)
        bad=work/'bad.cpp'
        source=(portable/'quicksort'/'kernel.cpp').read_text()
        adapter=report['bundle']['benchmarks']['quicksort']['adapter']
        adapter=adapter.replace('return 0;','if(n>0) keys[0]=2147483647; return 0;')
        assert 'keys[0]=2147483647;' in adapter
        bad.write_text(source+'\n'+runner.ENTRY.replace('BENCHMARK_CALL',adapter))
        harness=work/'driver.cpp'; harness.write_text('#include "support.hpp"\n#include "driver.hpp"\n')
        run(['g++','-std=c++17','-O1','-fopenmp','-DBENCH_KIND=1','-I',portable/'quicksort',bad,harness,'-o',work/'bad'])
        run([work/'bad','31','4','1','17','0'],expect=2)
        run([sys.executable,portable/'run.py','run','--output-dir',portable,'--cases','fibonacci',
             '--variants','apac-cost','--cost-threshold','5000','--size','12',
             '--threads','1','2','--repeat','1','--warmups','0'])
        extra=next(path for path in (portable/'results').iterdir() if path!=results[0])
        subset=json.loads((extra/'report.json').read_text())
        assert subset['complete'] and len(subset['samples'])==3
        assert {row['variant'] for row in subset['samples']}=={'sequential','apac-cost'}
        assert all(row['cost_threshold']==5000 for row in subset['samples'] if row['variant']=='apac-cost')
        assert report['bundle']['profiles']['standard'][0]==[43,1,1]
        assert report['bundle']['profiles']['coarse'][0]==[43,1,1]
        assert report['bundle']['profiles']['standard'][5]==[8192,128,8]
        assert report['bundle']['profiles']['coarse'][4]==[1024,128,1]
        assert report['bundle']['profiles']['coarse'][6]==[8192,128,20]
        run([sys.executable,portable/'run.py','run','--output-dir',portable,'--cost-threshold','0'],expect=1)
        print('PASS: variant selection, configurable cost threshold and isolated runtime environments',flush=True)
        # A timeout is a missing measurement, not a failed numerical oracle.
        # Exercise both a warmup timeout (no samples) and a timeout after one
        # measured sample, with all other variants/thread counts continuing.
        try:
            runner.command([sys.executable,'-c','import time; time.sleep(2)'],timeout=0.05,log=work/'timeout.log')
        except runner.CommandTimeout:
            assert 'TIMEOUT' in (work/'timeout.log').read_text()
        else:
            raise AssertionError('subprocess timeout was not classified')
        injection=work/'inject_timeout.py'
        injection.write_text(r'''import sys
from pathlib import Path
sys.path.insert(0,sys.argv[1])
import run as runner
original=runner.command
trigger=int(sys.argv[2]); calls=0

def injected(argv,**kwargs):
    global calls
    if Path(str(argv[0])).name=='fibonacci-apac-cost' and kwargs.get('env',{}).get('OMP_NUM_THREADS')=='1' and kwargs.get('env',{}).get('APAC_COST_THRESHOLD')=='100000':
        calls+=1
        if calls==trigger:
            raise runner.CommandTimeout('injected measurement timeout')
    return original(argv,**kwargs)
runner.command=injected
sys.argv=['run.py',*sys.argv[3:]]
sys.exit(runner.main())
''')
        for trigger in (1,3):
            before=set((portable/'results').iterdir())
            run([sys.executable,injection,portable,trigger,'run','--output-dir',portable,
                 '--cases','fibonacci','--size','12','--threads','1','2','--repeat','2','--warmups','1'])
            timeout_dir=(set((portable/'results').iterdir())-before).pop()
            timed=json.loads((timeout_dir/'report.json').read_text())
            assert timed['complete'] is True and timed['all_verified'] is False
            assert len(timed['timeouts'])==1 and timed['timeouts'][0]['warmup']==(trigger==1)
            missing=next(row for row in timed['summary'] if row['variant']=='apac-cost' and row['cost_threshold']==100000 and row['threads']==1)
            assert missing['status']==('timeout' if trigger==1 else 'partial')
            assert missing['samples']==(0 if trigger==1 else 1)
            assert all(row['status']=='ok' and row['samples']==2 for row in timed['summary'] if row is not missing)
            scaling=next(row for row in timed['summary'] if row['variant']=='apac-cost' and row['cost_threshold']==100000 and row['threads']==2)
            if trigger==1: assert scaling['speedup_vs_variant_1'] is None
            with (timeout_dir/'summary.csv').open() as stream:
                rows=list(csv.DictReader(stream))
            assert len(rows)==13 and any(row['status']==missing['status'] for row in rows)
        print('PASS: real subprocess timeout, warmup/measured timeouts remain explicit and other configurations continue',flush=True)
        manifest_path=portable/'manifest.json'
        manifest=json.loads(manifest_path.read_text())
        manifest['complete']=False; manifest_path.write_text(json.dumps(manifest))
        run([sys.executable,portable/'run.py','run','--output-dir',portable],expect=1)
        manifest['complete']=True; manifest_path.write_text(json.dumps(manifest))
        # Source hashes protect against accidentally mixing a modified source
        # with timings claimed to belong to the prepared revision.
        with (portable/'fibonacci'/'kernel.cpp').open('a') as stream: stream.write('\n// modified\n')
        run([sys.executable,portable/'run.py','run','--output-dir',portable,'--cases','fibonacci',
             '--profile','smoke','--threads','1','--repeat','1','--warmups','0'],expect=1)
        print(f'PASS: {checks} boundary executions, oracle rejects corruption, changed/incomplete bundles rejected',flush=True)
    except BaseException:
        print('Benchmark validation artifacts retained at '+str(work),flush=True)
        raise
    else:
        shutil.rmtree(work)

if __name__=='__main__':
    main()
