#!/usr/bin/env python3
"""Exercise array phase ordering and representative dependencies with OpenMP."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', type=Path, default=Path(os.environ.get('APAC_BUILD_DIR', ROOT / 'build')))
BUILD = parser.parse_args().build_dir.resolve()
def run(argv, folder, threads=1):
    p = subprocess.run([str(x) for x in argv], cwd=folder, text=True, capture_output=True,
                       env={**os.environ, 'OMP_NUM_THREADS':str(threads), 'OMP_DYNAMIC':'FALSE'}, timeout=45)
    assert p.returncode == 0, (argv, p.stdout, p.stderr)
    return p.stdout

helpers = '''
int index_apacGlobal=5;
struct Worker { void apply_apacSeq(int *p) { p[5]+=10; } };
int read_apacSeq(const int *p) { int sum=0; for(int i=0;i<16;++i) sum+=p[i]; return sum; }
void fill_apacSeq(int *p) { for(int i=0;i<16;++i) p[i]=1; }
void touch_apacSeq(int &v) { v+=10; }
void part_apacSeq(int *p, int n, int v) { for(int i=0;i<n;++i) p[i]+=v; }
int middle_apacSeq() { return 8; }
void rebind_apacSeq(int *&p,int *q) { p=q; }
'''
wrapper='''
#include <cstdio>
int main() { int result;
#pragma omp parallel
#pragma omp single
{ result=scenario(); }
std::printf("%d\\n",result); }
'''
cases = {
'pointerArgumentSideEffect': """int a[17]={}; int *p=a;
fill_apacSeq(p++); touch_apacSeq(p[0]);
int result; result=p[0]+a[0]*100+read_apacSeq(a); return result;""",
'dereference': """int a[16]={}; int *p=a; fill_apacSeq(p);
*p=10; touch_apacSeq(*p); ++*p;
int result; result=*p+read_apacSeq(p); return result;""",
'memberBuffer': """int a[16]={}; int *p=a; Worker worker; fill_apacSeq(p);
worker.apply_apacSeq(p); touch_apacSeq(p[6]);
int result; result=read_apacSeq(p); return result;""",
'globalIndex': """int a[16]={}; int *p=a; fill_apacSeq(p);
touch_apacSeq(p[index_apacGlobal]); index_apacGlobal=6;
int result; result=read_apacSeq(p)+index_apacGlobal; return result;""",
'partitionSizeMutation': """int a[16]={}; int *p=a; int m=8;
part_apacSeq(&p[0],m,100); part_apacSeq(&p[8],m,200); m=4;
int result; result=read_apacSeq(p)+m; return result;""",
'indexArrayMutation': """int a[16]={}; int ix[1]={5}; int *p=a;
fill_apacSeq(p); touch_apacSeq(p[ix[0]]); ix[0]=6;
int result; result=read_apacSeq(p)+ix[0]; return result;""",
'wrappedIndex': """int a[16]={}; int index=5; std::reference_wrapper<int> i(index); int *p=a;
fill_apacSeq(p); touch_apacSeq(p[i]); index=6;
int result; result=read_apacSeq(p)+index; return result;""",
'deleteBuffer': """int *p=new int[16]; fill_apacSeq(p);
touch_apacSeq(p[5]); touch_apacSeq(p[6]);
int result; result=read_apacSeq(p); delete[] p; return result;""",
'partitionEpoch': """int a[16]={}; int *p=a;
part_apacSeq(&p[0],8,100); part_apacSeq(&p[8],8,200);
#pragma omp taskwait
part_apacSeq(&p[0],4,300); part_apacSeq(&p[4],12,400);
int result; result=read_apacSeq(p); return result;""",
'indexResult': """int a[16]={}; int *p=a; fill_apacSeq(p);
int i; i=5; i=p[i]; touch_apacSeq(p[i]);
int result; result=i+read_apacSeq(p); return result;""",
'indexSideEffect': """int a[16]={}; int *p=a; fill_apacSeq(p);
int i; i=5; touch_apacSeq(p[i++]); touch_apacSeq(p[i]);
int result; result=i+read_apacSeq(p); return result;""",
'cycle': '''int data[16]={}; int *p=data;
int before; before=read_apacSeq(p); fill_apacSeq(p);
touch_apacSeq(p[5]); touch_apacSeq(p[6]);
int m; m=middle_apacSeq();
part_apacSeq(&p[0],8,100); part_apacSeq(&p[m],8,200);
int result; result=before+read_apacSeq(p); return result;''',
'readersToElements': '''int data[16]={}; int *p=data; fill_apacSeq(p);
int a; a=read_apacSeq(p); int b; b=read_apacSeq(p);
touch_apacSeq(p[5]); touch_apacSeq(p[6]);
int result; result=a*1000+b*100+read_apacSeq(p); return result;''',
'elementAlias': '''int data[16]={}; int *p=data; int *q=p;
fill_apacSeq(p); touch_apacSeq(p[5]); touch_apacSeq(q[5]);
int result; result=read_apacSeq(q); return result;''',
'partitionAlias': '''int data[16]={}; int *p=data; int *q=p;
fill_apacSeq(p); part_apacSeq(&p[0],8,100); part_apacSeq(&q[0],8,200);
int result; result=read_apacSeq(p); return result;''',
'rebinding': '''int a[16]={}; int b[16]={}; int *p=a;
part_apacSeq(&p[0],8,100); p=b; part_apacSeq(&p[0],8,200);
int result; result=read_apacSeq(a)+read_apacSeq(b); return result;''',
'rebindingCall': '''int a[16]={}; int b[16]={}; int *p=a;
part_apacSeq(&p[0],8,100); rebind_apacSeq(p,b); part_apacSeq(&p[0],8,200);
int result; result=read_apacSeq(a)+read_apacSeq(b); return result;''',
'branch': '''int a[16]={}; int *p=a; fill_apacSeq(p);
int flag; flag=middle_apacSeq();
if(flag>0) {touch_apacSeq(p[5]); touch_apacSeq(p[6]);}
int result; result=read_apacSeq(p); return result;''',
'directElements': '''int a[16]={}; int *p=a; fill_apacSeq(p);
p[5]+=10; p[6]+=20; p[5]+=30;
int result; result=read_apacSeq(p); return result;''',
'wrappedPointer': '''int data[16]={}; int *raw=data; std::reference_wrapper<int*> p(raw);
fill_apacSeq(raw); touch_apacSeq(p[5]); touch_apacSeq(p[6]);
int result; result=read_apacSeq(raw); return result;''',
'computedIndex': '''int a[16]={}; int ix[2]={5,6}; int *p=a; fill_apacSeq(p);
touch_apacSeq(p[ix[0]]); touch_apacSeq(p[ix[1]]);
int result; result=read_apacSeq(p); return result;''',
}
with tempfile.TemporaryDirectory(prefix='apac-array-phases-') as temp:
    root=Path(temp)
    for name, body in cases.items():
        folder=root/name; folder.mkdir()
        source='#include <functional>\n'+helpers+'\nint scenario() {\n'+body+'\n}\n'
        original=folder/'original.cpp'; original.write_text(source+wrapper)
        current=folder/'input.cpp'; current.write_text(source)
        for tool in ('duplicateFunctions','taskGraph'):
            output=run([BUILD/tool,current,'--','-std=c++17'],folder)
            current=folder/(tool+'.cpp'); current.write_text(output)
        scenario=output[output.rindex('int scenario()'):]
        token=re.search(r'char (__apac_array_phase_\d+);',scenario)
        assert token, (name, scenario)
        gate=token.group(1)
        if name=='cycle':
            assert len(re.findall(r'#pragma omp task depend\(inout: '+gate+r'\)\s*\{\}',scenario))==3, scenario
            for rep in ('p[5]','p[6]','p[0]','p[m]'):
                assert 'depend(inout: '+rep+')' in scenario, scenario
            assert re.search(r'taskwait depend\(in: [^)]*m',scenario), scenario
        if name in ('cycle','readersToElements'):
            # Distinct fine representatives must remain sibling tasks with only
            # shared read dependencies on the gate and pointer storage.
            left=scenario.index('touch_apacSeq(p[5]);')
            right=scenario.index('touch_apacSeq(p[6]);')
            between=scenario[left:right]
            assert '#pragma omp taskwait\n' not in between,scenario
            assert 'depend(inout: '+gate+')' not in between,scenario
        if name=='readersToElements':
            reads=[line for line in scenario.splitlines() if '#pragma omp task ' in line and re.search(r'depend \(inout: [^)]*\ba\b',line)]
            assert reads and 'depend(in: '+gate+')' in reads[0],scenario
        current.write_text(output+wrapper)
        for opt in ('-O0','-O2'):
            for label,path in (('original',original),('transformed',current)):
                run(['g++','-std=c++17','-fopenmp',opt,path,'-o',folder/label],folder)
            expected=run([folder/'original'],folder)
            for threads in (1,2,4):
                for repeat in range(5):
                    actual=run([folder/'transformed'],folder,threads)
                    assert actual==expected,(name,opt,threads,repeat,expected,actual,scenario)
        if name in ('cycle','readersToElements','partitionEpoch'):
            pipeline=folder/'pipeline.cpp'
            pipeline.write_text(source+'\n#include <cstdio>\nint main(){std::printf("%d\\n",scenario());return 0;}\n')
            run([BUILD/'apac',pipeline,'--','-std=c++17'],folder)
            run(['g++','-std=c++17','-fopenmp','-O2',folder/'APACpipeline.cpp','-o',folder/'pipeline'],folder)
            for threads in (1,2,4):
                assert run([folder/'pipeline'],folder,threads)==expected,(name,'full pipeline')
        print('PASS: array phases '+name,flush=True)
print(str(len(cases))+' array phase cases passed')
