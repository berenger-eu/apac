#!/usr/bin/env python3
"""Compare complete outputs through TaskGraph and the full APAC pipeline."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SUPPORT = r'''
#include <cstdio>
#include <cstdlib>
#include <omp.h>
int size_apacSeq() { return std::atoi(std::getenv("APAC_VALIDATION_SIZE")); }
int seed_apacSeq() { return std::atoi(std::getenv("APAC_VALIDATION_SEED")); }
void guard_apacSeq(int *p) { for (int i=0;i<132;++i) p[i]=-777; }
void initialize_apacSeq(int *p,int n,int seed) {
  for(int i=0;i<n;++i) p[i]=(i*17+seed*13)%101-50;
}
void part_apacSeq(int *p,int n,int v) { for(int i=0;i<n;++i) p[i]+=v; }
void negative_apacSeq(int *p,int n,int v) { for(int i=0;i<n;++i) p[i]-=v; }
void leaf_apacSeq(int *p,int n,int v) { for(int i=0;i<n;++i) p[i]=p[i]*2+v; }
void touch_apacSeq(int &v) { v+=7; }
int checksum_apacSeq(const int *p,int n) {
  int result=0; for(int i=0;i<n;++i) result+=(i+1)*p[i]; return result;
}
int boundary_apacSeq(int n,int seed) { return n ? 1+seed%n : 0; }
void rebind_apacSeq(int *&p,int *q) { p=q; }
void merge_apacSeq(int *p,int n,int m) {
  int tmp[128]; int i=0,j=m,k=0;
  while(i<m && j<n) { if(p[i]<=p[j]) tmp[k++]=p[i++]; else tmp[k++]=p[j++]; }
  while(i<m) tmp[k++]=p[i++];
  while(j<n) tmp[k++]=p[j++];
  for(i=0;i<n;++i) p[i]=tmp[i];
}
void double_fill_apacSeq(double *p,int n,int seed) {
  for(int i=0;i<n;++i) p[i]=i+seed;
}
void double_touch_apacSeq(double &v) { v+=0.5; }
int double_sum_apacSeq(const double *p,int n) {
  double sum=0; for(int i=0;i<n;++i) sum+=p[i]; return int(sum*2);
}
void dump_apacSeq(int result,const int *p) {
  std::printf("%d",result);
  for(int i=0;i<132;++i) std::printf(" %d",p[i]);
  std::printf("\n");
}
'''
CASES = {
    'recursiveTransform': r'''
void split(int *p,int n,int seed) {
  if(n>1) {
    int m; m=n/2;
    split(&p[0],m,seed+1);
    split(&p[m],n-m,seed+2);
  } else { leaf_apacSeq(p,n,seed); }
}
int scenario(int *data,int n,int seed) {
  initialize_apacSeq(data,n,seed);
  split(data,n,seed);
  int before; before=checksum_apacSeq(data,n);
  split(data,n,seed+3);
  int result; result=before+checksum_apacSeq(data,n); return result;
}
''',
    'recursiveMergeSort': r'''
void sort(int *p,int n) {
  if(n>1) {
    int m; m=n/2;
    sort(&p[0],m);
    sort(&p[m],n-m);
    merge_apacSeq(p,n,m);
  }
}
int scenario(int *data,int n,int seed) {
  initialize_apacSeq(data,n,seed);
  sort(data,n);
  int result; result=checksum_apacSeq(data,n); return result;
}
''',
    'dynamicPartitions': r'''
int scenario(int *data,int n,int seed) {
  initialize_apacSeq(data,n,seed);
  int before; before=checksum_apacSeq(data,n);
  int m; m=boundary_apacSeq(n,seed);
  part_apacSeq(&data[0],m,3);
  part_apacSeq(&data[m],n-m,11);
#pragma omp taskwait
  int k; k=boundary_apacSeq(n,seed+7);
  part_apacSeq(&data[0],k,17);
  part_apacSeq(&data[k],n-k,23);
  int result; result=before+checksum_apacSeq(data,n); return result;
}
''',
    'shiftedAliases': r'''
int scenario(int *data,int n,int seed) {
  initialize_apacSeq(data,n,seed);
  int *q=data+1;
  int m; m=n>1?n-1:0;
  part_apacSeq(&data[1],m,3);
  part_apacSeq(&q[0],m,11);
  if(n>1) { touch_apacSeq(q[0]); touch_apacSeq(data[1]); }
  int result; result=checksum_apacSeq(data,n); return result;
}
''',
    'conditionalAlias': r'''
int scenario(int *data,int n,int seed) {
  initialize_apacSeq(data,n,seed);
  int scratch[128]={};
  initialize_apacSeq(scratch,n,seed+3);
  int *alias;
  if(seed%2==0) { alias=data; } else { alias=scratch; }
  part_apacSeq(&data[0],n,3);
  part_apacSeq(&scratch[0],n,11);
  part_apacSeq(alias,n,17);
  int result; result=checksum_apacSeq(data,n)+checksum_apacSeq(scratch,n); return result;
}
''',
    'pointerReferenceRebind': r'''
int scenario(int *data,int n,int seed) {
  initialize_apacSeq(data,n,seed);
  int scratch[128]={};
  initialize_apacSeq(scratch,n,seed+3);
  int *alias=data;
  part_apacSeq(&alias[0],n,3);
  rebind_apacSeq(alias,scratch);
  part_apacSeq(&alias[0],n,11);
  int result; result=checksum_apacSeq(data,n)+checksum_apacSeq(scratch,n); return result;
}
''',
    'mixedElementTypes': r'''
int scenario(int *data,int n,int seed) {
  initialize_apacSeq(data,n,seed);
  double other[128]={};
  double_fill_apacSeq(other,n,seed);
  if(n>1) {
    touch_apacSeq(data[0]);
    double_touch_apacSeq(other[0]);
    touch_apacSeq(data[n-1]);
    double_touch_apacSeq(other[n-1]);
  }
  int result; result=checksum_apacSeq(data,n)+double_sum_apacSeq(other,n); return result;
}
''',
    'indirectCalls': r'''
int scenario(int *data,int n,int seed) {
  initialize_apacSeq(data,n,seed);
  void (*operation)(int *,int,int)=part_apacSeq;
  int m; m=boundary_apacSeq(n,seed);
  operation(&data[0],m,3);
  operation(&data[m],n-m,11);
  operation=&negative_apacSeq;
  operation(&data[0],m,1);
  operation(&data[m],n-m,5);
  int result; result=checksum_apacSeq(data,n); return result;
}
''',
    'nestedBufferLifetime': r'''
int scenario(int *data,int n,int seed) {
  initialize_apacSeq(data,n,seed);
  int value; value=0;
  if(seed%2==0) {
    int scratch[128]={};
    initialize_apacSeq(scratch,n,seed+3);
    part_apacSeq(&scratch[0],n,11);
    value=checksum_apacSeq(scratch,n);
  } else {
    int scratch[128]={};
    initialize_apacSeq(scratch,n,seed+7);
    part_apacSeq(&scratch[0],n,17);
    value=checksum_apacSeq(scratch,n);
  }
  part_apacSeq(data,n,23);
  int result; result=value+checksum_apacSeq(data,n); return result;
}
''',
}
INPUTS = ((0,0),(1,1),(2,17),(3,0),(7,1),(16,17),(31,0),(31,17),(64,1))
CONFIGS = {'O0': ['-O0'], 'O2': ['-O2'],
           'ubsan': ['-O1','-fsanitize=undefined','-fno-sanitize-recover=all']}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=Path(os.environ.get('APAC_BUILD_DIR',ROOT/'build')))
    parser.add_argument('--case',choices=tuple(CASES),action='append',dest='selected')
    parser.add_argument('--repeats',type=int,default=3)
    parser.add_argument('--keep-workdir',action='store_true')
    args=parser.parse_args()
    if args.repeats<1: parser.error('--repeats must be positive')
    build=args.build_dir.resolve()
    root=Path(tempfile.mkdtemp(prefix='apac-validation-'))
    executions=0
    checks=0
    report=[]

    def run(argv,folder,*,threads=1,inputs=(31,0)):
        nonlocal executions
        executions+=1
        result=subprocess.run([str(x) for x in argv],cwd=folder,capture_output=True,text=True,timeout=60,
            env={**os.environ,'OMP_NUM_THREADS':str(threads),'OMP_DYNAMIC':'FALSE',
                 'OMP_WAIT_POLICY':'PASSIVE','OMP_THREAD_LIMIT':str(threads),
                 'APAC_VALIDATION_SIZE':str(inputs[0]),'APAC_VALIDATION_SEED':str(inputs[1]),
                 'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'})
        if result.returncode:
            raise AssertionError(f'{argv} (threads={threads}, input={inputs})\n{result.stdout}\n{result.stderr}')
        return result.stdout

    def driver(parallel):
        launch='''#pragma omp parallel
#pragma omp single
{ result=scenario(data,n,seed); }''' if parallel else 'result=scenario(data,n,seed);'
        return '\nint main() {\nint data[132]; guard_apacSeq(data);\nint n; n=size_apacSeq();\nint seed; seed=seed_apacSeq();\nint result;\n'+launch+'\ndump_apacSeq(result,data); return 0;\n}\n'

    try:
        for name in args.selected or CASES:
            folder=root/name; folder.mkdir()
            # Ignored support isolates the operations under test; recursive
            # workers and scenario remain in the source and are transformed.
            (folder/'support.hpp').write_text(SUPPORT)
            source='#include "support.hpp"\n'+CASES[name]
            original=folder/'original.cpp'; original.write_text(source+driver(False))
            current=folder/'input.cpp'; current.write_text(source)
            for tool in ('duplicateFunctions','taskGraph'):
                output=run([build/tool,current,'--','-std=c++17'],folder)
                if name=='dynamicPartitions' and tool=='duplicateFunctions':
                    assert len(re.findall(r'#pragma omp taskwait\s*\n',output))==2, (name,'lost explicit taskwait during cloning')
                current=folder/(tool+'.cpp'); current.write_text(output)
            assert '__apac_array_phase_' in output and '#pragma omp task ' in output,(name,'no tasks generated')
            if name.startswith('recursive'):
                worker='split' if name=='recursiveTransform' else 'sort'
                assert re.search(r'void '+worker+r'\([^)]*\)\s*\{\s*char __apac_array_phase_',output),(name,output)
            current.write_text(output+driver(True))
            pipeline=folder/'pipeline.cpp'; pipeline.write_text(source+driver(False))
            run([build/'apac',pipeline,'--','-std=c++17'],folder)
            assert pipeline.read_text()==source+driver(False),(name,'pipeline changed the input')
            generated=folder/'APACpipeline.cpp'
            assert '__apac_array_phase_' in generated.read_text(),(name,'pipeline missing phase analysis')
            paths={'original':original,'taskgraph':current,'pipeline':generated}
            for config,flags in CONFIGS.items():
                for label,path in paths.items():
                    run(['g++','-std=c++17','-fopenmp',*flags,path,'-o',folder/(label+'-'+config)],folder)
                for inputs in INPUTS:
                    expected=run([folder/('original-'+config)],folder,inputs=inputs)
                    values=[int(value) for value in expected.split()]
                    assert len(values)==133,(name,inputs,expected)
                    assert values[1+inputs[0]:]==[-777]*(132-inputs[0]),(name,'baseline guard corruption',inputs)
                    if name=='recursiveMergeSort':
                        assert values[1:1+inputs[0]]==sorted((i*17+inputs[1]*13)%101-50 for i in range(inputs[0])),(name,'independent sort oracle',inputs)
                    for label in ('taskgraph','pipeline'):
                        for threads in (1,2,4,8):
                            for repeat in range(args.repeats):
                                actual=run([folder/(label+'-'+config)],folder,threads=threads,inputs=inputs)
                                checks+=1
                                if actual!=expected:
                                    (folder/'expected.txt').write_text(expected)
                                    (folder/'actual.txt').write_text(actual)
                                    raise AssertionError((name,label,config,inputs,threads,repeat,expected,actual))
                print(f'PASS: validation {name} ({config}, TaskGraph + full pipeline, 1/2/4/8 threads)',flush=True)
                report.append({'case':name,'configuration':config,'inputs':len(INPUTS),'threads':[1,2,4,8],
                               'paths':['taskgraph','pipeline'],'repeats':args.repeats})
        (root/'report.json').write_text(json.dumps({'checks':checks,'executions':executions,'runs':report},indent=2)+'\n')
    except BaseException:
        print('Validation artifacts retained at '+str(root),flush=True)
        raise
    else:
        print(f'{len(args.selected or CASES)} integration cases passed; {checks} output comparisons (including UBSan)',flush=True)
        if args.keep_workdir: print('Validation artifacts: '+str(root),flush=True)
        else: shutil.rmtree(root)

if __name__=='__main__':
    main()
