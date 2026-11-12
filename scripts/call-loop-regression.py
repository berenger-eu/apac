#!/usr/bin/env python3
"""Check sibling call tasks, loop-carried dependencies and safe loop fallbacks."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', type=Path, default=Path(os.environ.get('APAC_BUILD_DIR', ROOT/'build')))
BUILD = parser.parse_args().build_dir.resolve()

HELPERS = r'''
void add_apacSeq(int *p, int value) { p[0] = p[0] * 3 + value; }
void copy_apacSeq(const int *p, int *q) { q[0] = p[0]; }
void scale_apacSeq(double *p, double factor) { p[0] += factor; }
void ref_apacSeq(int &p, int value) { p += value; }
int total_apacSeq(const int *p, int n) { int sum=0; for(int i=0;i<n;++i) sum+=p[i]; return sum; }
'''
CASES = {
    'floatingArguments': (r'''
double values[4]={}; double factor=0.25;
for(int i=0;i<4;++i) { scale_apacSeq(&values[i],factor); }
factor=7;
int result; result=4*(values[0]+values[1]+values[2]+values[3]); return result;
''', 1),
    'nestedDependencies': (r'''
int data[6]={};
for(int i=0;i<2;++i) {
  for(int j=0;j<3;++j) {
    for(int k=0;k<4;++k) {
      add_apacSeq(&data[i*3+j], k+1);
    }
  }
}
int result; result=total_apacSeq(data,6); return result;
''', 1),
    'aliasReadersAndWriters': (r'''
int data[8]={}; int out[8]={}; int *p=data; int *q=p;
for(int i=0;i<8;++i) {
  add_apacSeq(&p[i],i+1);
  copy_apacSeq(&q[i],&out[i]);
  add_apacSeq(&p[i],2);
}
int result; result=total_apacSeq(out,8); return result;
''', 3),
    'controlLocalCapture': (r'''
int data[8]={};
for(int i=0;i<8;++i) {
  int value; value=i+1;
  add_apacSeq(&data[i],value);
  value=100;
}
int result; result=total_apacSeq(data,8); return result;
''', 1),
    'breakFallback': (r'''
int data[8]={};
for(int i=0;i<8;++i) { add_apacSeq(&data[i],i); if(i==3) break; }
int result; result=total_apacSeq(data,8); return result;
''', 0),
    'scalarReferenceFallback': (r'''
int data[8]={};
for(int i=0;i<8;++i) { ref_apacSeq(data[i],i); }
int result; result=total_apacSeq(data,8); return result;
''', 0),
    'taskOutputInControlFallback': (r'''
int data[8]={};
for(int i=0;i<8;++i) { add_apacSeq(&data[i],i); int value; value=data[i]; }
int result; result=total_apacSeq(data,8); return result;
''', 0),
    'scalarAliasFallback': (r'''
int limit; limit=8; int *p; p=&limit;
for(int i=0;i<limit;++i) { add_apacSeq(&p[0],-22); }
return limit;
''', 0),
    'emptyLoop': (r'''
int data[8]={};
for(int i=0;i<0;++i) { add_apacSeq(&data[i],i); }
int result; result=total_apacSeq(data,8); return result;
''', 1),
}
WRAPPER = r'''
#include <cstdio>
int main() { int result;
#pragma omp parallel
#pragma omp single
{ result=scenario(); }
std::printf("%d\n",result); }
'''

def run(argv, cwd, threads=1):
    result = subprocess.run([str(arg) for arg in argv], cwd=cwd, text=True,
                            capture_output=True, timeout=90,
                            env={**os.environ, 'OMP_NUM_THREADS': str(threads),
                                 'OMP_DYNAMIC': 'FALSE', 'OMP_WAIT_POLICY': 'PASSIVE'})
    assert result.returncode == 0, (argv, result.stdout[-3000:], result.stderr[-3000:])
    return result.stdout

with tempfile.TemporaryDirectory(prefix='apac-call-loop-') as temporary:
    root=Path(temporary)
    for name,(body,templates) in CASES.items():
        folder=root/name; folder.mkdir()
        source=HELPERS+'\nint scenario() {\n'+body+'\n}\n'
        original=folder/'original.cpp'; original.write_text(source+WRAPPER)
        current=folder/'input.cpp'; current.write_text(source)
        for tool in ('duplicateFunctions','taskGraph'):
            output=run([BUILD/tool,current,'--','-std=c++17'],folder)
            current=folder/(tool+'.cpp'); current.write_text(output)
        for pipeline in ('taskGraph','apac'):
            if pipeline=='apac':
                input_file=folder/'pipeline.cpp'; input_file.write_text(source)
                run([BUILD/'apac',input_file,'--','-std=c++17'],folder)
                output=(folder/'APACpipeline.cpp').read_text()
            assert output.count('__apac_depth_guard __apac_loop_depth')==templates,(name,pipeline,output)
            for capture in re.findall(r'firstprivate\(([^)]*)\)',output):
                assert 'data' not in capture.split(',') and 'out' not in capture.split(','),(name,capture)
            current=folder/(pipeline+'-checked.cpp'); current.write_text(output+WRAPPER)
            for optimization in ('-O0','-O2'):
                for label,path in (('original',original),('transformed',current)):
                    run(['g++','-std=c++17','-fopenmp',optimization,path,'-o',folder/label],folder)
                expected=run([folder/'original'],folder)
                for threads in (1,2,4):
                    for repeat in range(3):
                        assert run([folder/'transformed'],folder,threads)==expected,(name,pipeline,threads)
        print('PASS: call loops '+name+' (TaskGraph/full pipeline, -O0/-O2, 1/2/4 threads)',flush=True)
    # Verify actual overlap, not just the presence of pragmas. The shared
    # instrumentation is protected by atomics and is outside the dependency data.
    source=r'''
#include <atomic>
#include <chrono>
#include <thread>
std::atomic<int> active{0}, peak{0};
void work_apacSeq(int *p,int value) {
  int now=++active; int before=peak.load();
  while(before<now && !peak.compare_exchange_weak(before,now)) {}
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  p[0]=value; --active;
}
int scenario() {
  int data[8]={};
  for(int i=0;i<8;++i) { work_apacSeq(&data[i],i+1); }
  int sum=0; for(int i=0;i<8;++i) sum+=data[i];
  return sum;
}
'''
    folder=root/'overlap'; folder.mkdir()
    current=folder/'input.cpp'; current.write_text(source)
    for tool in ('duplicateFunctions','taskGraph'):
        output=run([BUILD/tool,current,'--','-std=c++17'],folder)
        current=folder/(tool+'.cpp'); current.write_text(output)
    assert output.count('__apac_depth_guard __apac_loop_depth')==1,output
    current.write_text(output+WRAPPER.replace('result);','result*100+peak.load());'))
    run(['g++','-std=c++17','-O1','-fopenmp',current,'-o',folder/'overlap'],folder)
    assert int(run([folder/'overlap'],folder,1))==3601
    for repeat in range(3):
        value=int(run([folder/'overlap'],folder,4))
        assert value//100==36 and value%100>=2,value
    print('PASS: independent loop calls execute concurrently; array writes remain shared',flush=True)
