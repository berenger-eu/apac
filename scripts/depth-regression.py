#!/usr/bin/env python3
"""Check depth limits, overrides and restoration across tasks and repeated calls."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir',type=Path,default=Path(os.environ.get('APAC_BUILD_DIR',ROOT/'build')))
BUILD=parser.parse_args().build_dir.resolve()
BASE={k:v for k,v in os.environ.items() if k!='APAC_DEPTH_MAX'}
SOURCE=r'''
int tree(int n) {
  if(n<1) return 1;
  int left; left=tree(n-1);
  int right; right=tree(n-1);
  int result; result=left+right; return result;
}
'''
WRAPPER=r'''
int main(int argc,char **) {
  if(argc>1) { std::printf("%d %d\n",parallel_depth,__apac_depth_max); return 0; }
  int errors=0, total=0;
  __apac_depth=3;
  try {
    __apac_depth_guard outer(5);
    { __apac_depth_guard inner(9); if(__apac_depth!=9) ++errors; }
    if(__apac_depth!=5) ++errors;
    throw 1;
  } catch(int) {}
  if(__apac_depth!=3) ++errors;
  __apac_depth=0;
#pragma omp parallel
  {
#pragma omp single
    {
      for(int step=0;step<12;++step) {
        int result=tree(6);
        if(result!=64 || __apac_depth!=0) ++errors;
        total+=result;
      }
    }
    if(__apac_depth!=0) {
#pragma omp atomic update
      ++errors;
    }
  }
  std::printf("%d %d\n",errors,total);
  return errors ? 1 : 0;
}
'''

def run(argv,cwd,*,threads=1,depth=None,expect=0):
    env={**BASE,'OMP_NUM_THREADS':str(threads),'OMP_DYNAMIC':'FALSE',
         'OMP_WAIT_POLICY':'PASSIVE','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
    if depth is not None: env['APAC_DEPTH_MAX']=str(depth)
    p=subprocess.run([str(x) for x in argv],cwd=cwd,env=env,text=True,capture_output=True,timeout=90)
    assert p.returncode==expect,(argv,p.returncode,p.stdout[-3000:],p.stderr[-3000:])
    return p

with tempfile.TemporaryDirectory(prefix='apac-depth-') as temporary:
    root=Path(temporary)
    for tool in ('taskGraph','apac','apacDepth'):
        folder=root/tool; folder.mkdir()
        original=folder/'input.cpp'; original.write_text(SOURCE)
        if tool=='apac':
            run([BUILD/tool,original,'--','-std=c++17'],folder)
            output=(folder/'APACinput.cpp').read_text()
        else:
            copied=folder/'copied.cpp'
            copied.write_text(run([BUILD/'duplicateFunctions',original,'--','-std=c++17'],folder).stdout)
            output=run([BUILD/tool,copied,'--','-std=c++17'],folder).stdout
        assert 'ffs(' not in output
        source=folder/'checked.cpp'; source.write_text(output+WRAPPER)
        binary=folder/'checked'
        run(['g++','-std=c++17','-fopenmp','-O1','-fsanitize=undefined',
             '-fno-sanitize-recover=all',source,'-o',binary],folder)
        for threads,expected in ((1,1),(2,2),(3,3),(4,3),(8,4),(16,5),
                                 (18,6),(24,6),(32,6),(36,7),(48,7)):
            assert run([binary,'probe'],folder,threads=threads).stdout.strip()==f'{expected} {expected}'
        for depth in (0,1,2,5,8):
            assert run([binary,'probe'],folder,threads=36,depth=depth).stdout.strip()==f'7 {depth}'
        for bad in ('','-1','1x','2147483648','99999999999999999999999999'):
            p=run([binary,'probe'],folder,depth=bad,expect=2)
            assert 'APAC_DEPTH_MAX' in p.stderr
        for threads in (1,2,3,4,6,8):
            for depth in (None,0,1,2,7):
                assert run([binary],folder,threads=threads,depth=depth).stdout.strip()=='0 768'
        print(f'PASS: {tool}: automatic/explicit/invalid depths, repeated calls and restoration on all workers (UBSan)',flush=True)
    # Standalone ApacDepth must guard individual recursive expressions, even
    # when they are nested or occur in a condition, without hiding declarations.
    folder=root/'expressions'; folder.mkdir()
    original=folder/'input.cpp'; original.write_text(r'''
int nested(int n) {
  if(n<1) return 0;
  if(nested(n-1)<0) return -1;
  return nested(nested(n-1));
}
''')
    current=folder/'copied.cpp'
    current.write_text(run([BUILD/'duplicateFunctions',original,'--','-std=c++17'],folder).stdout)
    output=run([BUILD/'apacDepth',current,'--','-std=c++17'],folder).stdout
    current.write_text(output+r'''
int main() {
  for(int i=0;i<5;++i) if(nested(5)!=0 || __apac_depth!=0) return 1;
  return 0;
}
''')
    run(['g++','-std=c++17','-fopenmp','-O1','-fsanitize=undefined',current,'-o',folder/'checked'],folder)
    for depth in (0,1,3,8): run([folder/'checked'],folder,depth=depth)
    print('PASS: standalone depth guards cover nested calls and conditions',flush=True)
