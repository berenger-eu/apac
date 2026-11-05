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
        for name in NAMES:
            for filename in ('kernel.cpp','APACkernel.cpp'):
                assert (portable/name/filename).read_bytes()==(bundle/name/filename).read_bytes()
        print('PASS: archive contains the original and generated sources for all seven benchmarks',flush=True)
        for name,callee in (('matmul','multiply_tile'),('nbody','body_acceleration'),('jacobi','relax_rows')):
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
             env={**os.environ,'APAC_BUILD_DIR':'/nonexistent-build','OMP_WAIT_POLICY':'PASSIVE','APAC_DEPTH_MAX':'8'})
        results=list((portable/'results').iterdir())
        assert len(results)==1
        report=json.loads((results[0]/'report.json').read_text())
        assert report['metadata']['environment']['APAC_DEPTH_MAX']=='8'
        assert report['complete'] is True
        assert len(report['samples'])==40 and len(report['summary'])==40
        assert set(report['bundle']['benchmarks'])==set(NAMES)
        assert 'nqueens' not in report['bundle']['default_cases']
        assert 'dotproduct' not in report['bundle']['benchmarks']
        with (results[0]/'samples.csv').open() as stream:
            assert len(list(csv.DictReader(stream)))==40
        assert all(row['verified'] and row['team_threads']==row['threads'] for row in report['samples'])
        print('PASS: seven portable benchmarks, four sort distributions, 40 verified samples with UBSan',flush=True)
        # Exercise empty/singleton buffers, odd splits, smallest grids and both
        # ping-pong parities. Timings here have no performance assertion.
        inputs={
            'fibonacci':[(n,1,1,17,0) for n in (0,1,2,15,25)],
            'quicksort':[(n,1,1,17,p) for n in (0,1,2,31) for p in range(4)],
            'mergesort':[(n,1,1,17,p) for n in (0,1,2,31) for p in range(4)],
            'nqueens':[(n,1,1,17,0) for n in (0,1,2,4,9)],
            'matmul':[(n,1,1,17,0) for n in (0,1,2,7)],
            'nbody':[(n,1,steps,17,0) for n in (1,2,7) for steps in (1,2,3)],
            'jacobi':[(n,1,steps,17,0) for n in (2,3,7) for steps in (1,2,3)],
        }
        checks=0
        for name,cases in inputs.items():
            for values in cases:
                baseline=None
                for variant,threads in (('sequential',1),('apac',1),('apac',4)):
                    row=json.loads(run([results[0]/'bin'/(name+'-'+variant),*values],env={**os.environ,
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
        sys.path.insert(0,str(portable))
        import run as runner
        adapter=report['bundle']['benchmarks']['quicksort']['adapter']
        adapter=adapter.replace('return 0;','if(n>0) keys[0]=2147483647; return 0;')
        assert 'keys[0]=2147483647;' in adapter
        bad.write_text(source+'\n'+runner.ENTRY.replace('BENCHMARK_CALL',adapter))
        harness=work/'driver.cpp'; harness.write_text('#include "support.hpp"\n#include "driver.hpp"\n')
        run(['g++','-std=c++17','-O1','-fopenmp','-DBENCH_KIND=1','-I',portable/'quicksort',bad,harness,'-o',work/'bad'])
        run([work/'bad','31','4','1','17','0'],expect=2)
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
