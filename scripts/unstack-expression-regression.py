#!/usr/bin/env python3
"""Check statement extraction, C++ value categories and actual sibling task overlap."""
from pathlib import Path
import subprocess,tempfile,os,re,argparse
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir',type=Path,default=Path(os.environ.get('APAC_BUILD_DIR',ROOT/'build')))
BUILD=parser.parse_args().build_dir.resolve()
def run(argv,folder,env=None):
 p=subprocess.run(list(map(str,argv)),cwd=folder,capture_output=True,text=True,timeout=120,env=env)
 assert p.returncode==0,(argv,p.returncode,p.stdout[-3000:],p.stderr[-5000:])
 return p.stdout
cases={
'operations':r'''
int f(int x){return x+1;} int g(int x){return x*2;}
int sum(int n){return f(n)+g(n)+10;}
int nested(int n){return f(g(f(n)));}
int main(){int x=f(1)+g(2);return x!=6 || sum(3)!=20 || nested(3)!=9;}
''',
'control':r'''
int hits=0; int f(int n){++hits;return n;}
int test(int n){if(n) return f(1)+f(2); else return f(3)+f(4);}
int main(){int x=0; if(false) x=f(1)+f(2); else x=f(3)+f(4);
for(int i=0;i<2;++i) x+=f(i)+f(i+1);
while(f(0)) x=f(3)+f(4);
bool a=false&&f(4); bool b=true||f(5); int z=true?f(6):f(7);
int unevaluated=sizeof(f(9)+f(10));
return x!=11 || test(1)!=3 || test(0)!=7 || a || !b || z!=6 || hits!=12;}
''',
'categories':r'''
#include <utility>
int value=5; int &left(){return value;} int &&right(){return static_cast<int&&>(value);}
int val(){return 3;} int pick(int&){return 1;} int pick(int&&){return 2;}
int main(){int x=pick(val())+pick(right())+pick(left());left()=9;
return x!=5 || value!=9;}
''',
'moveOnly':r'''
#include <memory>
#include <utility>
std::unique_ptr<int> make(){return std::make_unique<int>(7);}
int consume(std::unique_ptr<int> v){return *v;}
int main(){int a=consume(make()); auto p=make(); int b=consume(std::move(p));return a!=7||b!=7||p!=nullptr;}
''',
'elision':r'''
struct X {X()=default; X(const X&)=delete; X(X&&)=delete;};
X make(){return X{};} int consume(X){return 9;}
int main(){int n=consume(make());return n!=9;}
''',
'lifetime':r'''
int alive=0; struct X{X(){++alive;} ~X(){--alive;} X(const X&)=delete;};
X make(){return X{};} int consume(const X&){return alive;} int count(){return alive;}
int main(){int n=(consume(make()),count());return n!=1 || alive!=0;}
''',
'arguments':r'''
int state=0; int f(){return ++state;} int g(){return ++state;}
int pair(int a,int b){return a*10+b;} int outer(int x){return x;}
int main(){int n=outer(pair(f()+state,g())); return !(n==12 || n==22 || n==31 || n==41);}
''',
'sequencing':r'''
int a[2]={4,5},b[2]={6,7};int *p=a;int state=0;
int index(){p=b;return 1;}
int leaf(int n){return n+1;}
int left(){state=1;return 1;} int right(){return state;}
int main(){int n=p[index()];int shifted=left()<<right();
if(false) int unused=leaf(1)+leaf(2);
if(true) int unused=leaf(3)+leaf(4);
int x=0;switch(1){case 1:x=leaf(1)+leaf(2);break;default:break;}
return n!=5 || shifted!=2 || x!=5;}
''',
'recordReferences':r'''
struct X{}; X value; X&& right(){return static_cast<X&&>(value);} X& left(){return value;}
int pick(X&&){return 2;} int pick(X&){return 1;}
int main(){int n=pick(right())+pick(left());return n!=3;}
''',
'declarationAndJumps':r'''
int f(int n){return n+1;} int address(int*p){return p!=nullptr;}
int jumped(){goto end;f(f(1));end:return 0;}
int main(){int self=address(&self)+f(1);int x=0;
switch(1){case 1:x+=f(1)+f(2);x+=f(3)+f(4);break;case 2:x=100;break;}
return self!=3 || x!=14 || jumped()!=0;}
''',
'referenceArguments':r'''
int log=0,value=0;
int first(){log=log*10+1;return 0;}
int& reference(int){log=log*10+2;return value;}
int other(){log=log*10+3;return 0;}
void consume(const int&,int){}
int main(){consume(reference(first()),other());return log!=123 && log!=312;}
''',
'names':r'''
int f(int n){return n+1;} int main(){int __tempVar_0=2; int n=f(1)+f(2);return n!=5||__tempVar_0!=2;}
'''
}
with tempfile.TemporaryDirectory(prefix='apac-expression-') as tmp:
 folder=Path(tmp)
 for name,source in cases.items():
  original=folder/(name+'.cpp');original.write_text(source)
  output=run([BUILD/'unstack',original,'--','-std=c++17'],folder)
  changed=folder/(name+'-out.cpp');changed.write_text(output)
  for path in [original,changed]:
   run(['g++','-std=c++17','-O1','-fsanitize=undefined','-fno-sanitize-recover=all',path,'-o',folder/'check'],folder)
   run([folder/'check'],folder)
  if name=='operations':assert output.count('__tempVar_')>=6,output
  print('PASS: expression extraction '+name,flush=True)
 # Parallel overlap, rather than a timing speedup, verifies the generated DAG.
 source=r'''
#include <atomic>
#include <chrono>
std::atomic<int> active{0},peak{0};
int leaf(int n){
 int a=++active; int old=peak.load();while(a>old&&!peak.compare_exchange_weak(old,a)){}
 auto until=std::chrono::steady_clock::now()+std::chrono::milliseconds(200);
 while(peak.load()<2&&std::chrono::steady_clock::now()<until){}
 --active;return n+1;
}
int combine(int n){return leaf(n)+leaf(n+1)+10;}
unsigned long long fib(int n){if(n<2)return n;return fib(n-1)+fib(n-2);}
'''
 original=folder/'pipeline.cpp';original.write_text(source)
 for cost in [False,True]:
  run([BUILD/'apac',original,'--functions','combine,fib','--ignore','leaf',*(['--static-cost'] if cost else []),'--','-std=c++17'],folder)
  output=(folder/'APACpipeline.cpp').read_text()
  # The two calls must be in separate task bodies, never one sum expression.
  assert not re.search(r'leaf\([^;]+\)\s*\+\s*leaf\(',output.split('int combine(')[-1]),output
  output+=r'''
int main(){int result=0;unsigned long long f=0;
#pragma omp parallel
#pragma omp single
{result=combine(3); f=fib(20);}
if(result!=19||f!=6765)return 1;
return omp_get_max_threads()>1 && peak.load()<2;
}
'''
  checked=folder/'checked.cpp';checked.write_text(output)
  run(['g++','-std=c++17','-O1','-fopenmp','-fsanitize=undefined','-fno-sanitize-recover=all',checked,'-o',folder/'parallel'],folder)
  for threads in [1,2,4,8]:
   run([folder/'parallel'],folder,env={**os.environ,'OMP_NUM_THREADS':str(threads),'OMP_DYNAMIC':'FALSE','OMP_WAIT_POLICY':'PASSIVE','APAC_DEPTH_MAX':'32','APAC_COST_THRESHOLD':'100000' if cost else '0'})
  print('PASS: independent sibling tasks overlap, Fibonacci correct, static cost='+str(cost),flush=True)

# These cases must also survive DeclarationSplitter and the later task passes.
with tempfile.TemporaryDirectory(prefix='apac-expression-objects-') as tmp:
 folder=Path(tmp)
 for name,source in {
  'move': '#include <memory>\nstd::unique_ptr<int> make(){return std::make_unique<int>(7);} int consume(std::unique_ptr<int> p){return *p;} int entry(){return consume(make());}',
  'reference': '#include <utility>\nstruct X{};X value;X&& right(){return static_cast<X&&>(value);}int take(X&&){return 7;}int entry(){return take(right());}'
 }.items():
  original=folder/(name+'.cpp');original.write_text(source)
  run([BUILD/'apac',original,'--functions','entry','--','-std=c++17'],folder)
  output=folder/('APAC'+name+'.cpp')
  output.write_text(output.read_text()+'\nint main(){int v=0;\n#pragma omp parallel\n#pragma omp single\n{v=entry();}\nreturn v!=7;}\n')
  run(['g++','-std=c++17','-O1','-fopenmp','-fsanitize=undefined','-fno-sanitize-recover=all',output,'-o',folder/'check'],folder)
  run([folder/'check'],folder,env={**os.environ,'OMP_NUM_THREADS':'4','APAC_DEPTH_MAX':'32','APAC_COST_THRESHOLD':'0'})
  print('PASS: full pipeline '+name+' preserves ownership/value category',flush=True)
