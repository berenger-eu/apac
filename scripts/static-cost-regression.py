#!/usr/bin/env python3
"""Validate coarse static work estimates, SCC recursion and sequential cutoffs."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir',type=Path,default=Path(os.environ.get('APAC_BUILD_DIR',ROOT/'build')))
BUILD=parser.parse_args().build_dir.resolve()
BASE={key:value for key,value in os.environ.items() if key not in ('APAC_COST_THRESHOLD','APAC_DEPTH_MAX')}


def run(argv,cwd,*,threshold=None,threads=4,expect=0):
    env={**BASE,'OMP_NUM_THREADS':str(threads),'OMP_DYNAMIC':'FALSE','OMP_WAIT_POLICY':'PASSIVE',
         'APAC_DEPTH_MAX':'32','UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'}
    if threshold is not None: env['APAC_COST_THRESHOLD']=str(threshold)
    p=subprocess.run([str(arg) for arg in argv],cwd=cwd,env=env,text=True,capture_output=True,timeout=90)
    assert p.returncode==expect,(argv,p.returncode,p.stdout[-4000:],p.stderr[-4000:])
    return p.stdout


def transform(folder,source,tool,enabled=True):
    current=folder/'input.cpp'; current.write_text(source)
    flags=['--static-cost'] if enabled else []
    if tool=='apac':
        run([BUILD/tool,current,*flags,'--','-std=c++17'],folder)
        return (folder/'APACinput.cpp').read_text()
    output=run([BUILD/'duplicateFunctions',current,'--','-std=c++17'],folder)
    current=folder/'cloned.cpp'; current.write_text(output)
    return run([BUILD/'taskGraph',current,*flags,'--','-std=c++17'],folder)


def instrument(output):
    lines=[]
    pending=False
    for line in output.splitlines(keepends=True):
        if re.match(r'\s*#pragma omp task(?:\s|$)',line):
            pending=True
        elif pending and '{' in line:
            line=line.replace('{','{ ++cost_test_tasks; ',1)
            pending=False
        line=line.replace('__apac_depth_guard __apac_loop_depth(',
                          '++cost_test_loops; __apac_depth_guard __apac_loop_depth(')
        lines.append(line)
    assert not pending
    return '#include <atomic>\nstd::atomic<int> cost_test_tasks{0},cost_test_loops{0};\n'+''.join(lines)

CASES={
    'fib': (r'''
int fib(int n) {
 if(n<2) return n;
 int left; left=fib(n-1); int right; right=fib(n-2);
 int result; result=left+right; return result;
}
''','fib',144),
    'mutual': (r'''
int f2(int n);
int f1(int n) { if(n<=0) return 0; int r; r=f2(n-1); return r+1; }
int f2(int n) { if(n<=0) return 0; int r; r=f1(n-1); return r+1; }
''','f1',12),
    'threeFunctionCycle': (r'''
int a(int n); int b(int n); int c(int n);
int c(int n) { if(n<=0) return 0; int r; r=a(n-1); return r+1; }
int a(int n) { if(n<=0) return 0; int r; r=b(n-1); return r+1; }
int b(int n) { if(n<=0) return 0; int r; r=c(n-1); return r+1; }
''','a',12),
    'unknownExternal': (r'''
int opaque(int n);
int unknown(int n) { int result; result=opaque(n); return result; }
''','unknown',13),
}

with tempfile.TemporaryDirectory(prefix='apac-static-cost-') as temporary:
    root=Path(temporary)
    for case,(source,entry,expected) in CASES.items():
        for tool in ('taskGraph','apac'):
            folder=root/(case+'-'+tool); folder.mkdir()
            output=transform(folder,source,tool)
            plain=transform(folder,source,tool,False)
            assert '__apac_cost_' not in plain
            models=dict(re.findall(r'APAC cost model: (\w+) => (__apac_cost_\d+);',output))
            model=models[entry]
            if case in ('mutual','threeFunctionCycle'):
                members=('f1','f2') if case=='mutual' else ('a','b','c')
                components=[re.search(r'APAC cost model: '+name+r' => \w+; SCC (\d+); recursive scale n',output).group(1) for name in members]
                assert len(set(components))==1,output
            suffix='int opaque_runs=0;\n'
            if case=='unknownExternal': suffix+='int opaque(int n) { ++opaque_runs; return n+1; }\n'
            suffix+=r'''
int main(int argc,char **argv) {
  int n=argc>2 ? std::atoi(argv[2]) : 12;
  if(argc>1 && argv[1][0]=='p') { std::printf("%llu %d\n",MODEL(__apac_cost_size(n)),opaque_runs); return 0; }
  int result=0;
#pragma omp parallel
#pragma omp single
  {result=ENTRY(n);}
  std::printf("%d %d\n",result,cost_test_tasks.load());
  return result!=(n==28 && FIBCASE ? 317811 : EXPECTED);
}
'''.replace('MODEL',model).replace('ENTRY',entry).replace('EXPECTED',str(expected)).replace('FIBCASE','1' if case=='fib' else '0')
            checked=folder/'checked.cpp'; checked.write_text(instrument(output)+suffix)
            binary=folder/'checked'
            run(['g++','-std=c++17','-O1','-fopenmp','-fsanitize=undefined',
                 '-fno-sanitize-recover=all',checked,'-o',binary],folder)
            cost,original_calls=map(int,run([binary,'probe','12'],folder).split())
            assert original_calls==0 # Estimation must never run the application.
            if case=='unknownExternal':
                assert cost==2**64-1
            else:
                small=int(run([binary,'probe','10'],folder).split()[0])
                large=int(run([binary,'probe','20'],folder).split()[0])
                assert 0<small<2**64-1 and (large>10*small if case=='fib' else large==2*small),(case,small,large)
                assert int(run([binary,'probe','1000000'],folder).split()[0])<2**64-1
            for threads in (1,2,4):
                for threshold in (None,0,1,10**9):
                    value,tasks=map(int,run([binary],folder,threshold=threshold,threads=threads).split())
                    assert value==expected
                    if case!='unknownExternal' and threshold==10**9: assert tasks==0,(case,tasks)
                    else: assert tasks>0,(case,threshold,tasks)
            if case=='fib':
                counts=[]
                for cutoff in (10000,100000,1000000):
                    value,tasks=map(int,run([binary,'run','28'],folder,threshold=cutoff,threads=4).split())
                    assert value==317811 and 0<tasks<1000000,(tool,cutoff,value,tasks)
                    counts.append(tasks)
                assert counts[0]>counts[1]>counts[2],(tool,counts)
                print(f'PASS: Fibonacci(28) retains tasks at all three thresholds: {counts}',flush=True)
            for bad in ('','-1','1x','18446744073709551615','999999999999999999999999'):
                run([binary,'probe'],folder,threshold=bad,expect=2)
            print(f'PASS: static cost {case}/{tool}: finite SCC models, thresholds, real sequential fallback and UBSan',flush=True)
    # Increasing-loop work composes with the n multiplier for recursion.
    folder=root/'quadratic'; folder.mkdir()
    source=r'''
void partition_work(int *data,int n) { for(int i=0;i<n;++i) data[0]+=i; }
void recursive_work(int *data,int n) {
 partition_work(data,n);
 if(n>1) { recursive_work(data,n-1); }
}
'''
    output=transform(folder,source,'apac')
    model=re.search(r'APAC cost model: recursive_work => (\w+);',output).group(1)
    checked=folder/'checked.cpp'; checked.write_text(output+f'''
int main() {{ auto a={model}(10),b={model}(20); std::printf("%llu %llu\\n",a,b); }}
''')
    run(['g++','-std=c++17','-O1','-fopenmp',checked,'-o',folder/'checked'],folder)
    a,b=map(int,run([folder/'checked'],folder).split())
    assert 3*a<b<5*a and b<2**64-1,(a,b)
    print('PASS: linear partition work times recursive size gives quadratic estimates',flush=True)
    # Real sort base cases must retain quadratic insertion-sort work, rather
    # than multiplying it by a recursion factor when no recursive edge is taken.
    for kernel,entry in (('quicksort','quick_sort'),('mergesort','merge_sort')):
        folder=root/('terminal-'+kernel); folder.mkdir()
        output=transform(folder,(ROOT/'benchmarks/kernels'/(kernel+'.cpp')).read_text(),'apac')
        model=re.search(r'APAC cost model: '+entry+r' => (\w+);',output).group(1)
        checked=folder/'checked.cpp'
        checked.write_text(output+f'''
int main() {{ std::printf("%llu %llu %llu %llu\\n",{model}(8,32),{model}(16,32),{model}(32,32),{model}(33,32)); }}
''')
        run(['g++','-std=c++17','-O1','-fopenmp','-fsanitize=undefined',checked,'-o',folder/'checked'],folder)
        a,b,terminal,recursive=map(int,run([folder/'checked'],folder).split())
        assert 3*a<b<5*a and terminal<recursive<2**64-1,(kernel,a,b,terminal,recursive)
        print(f'PASS: {kernel} terminal insertion branch has quadratic cost and no recursive multiplier',flush=True)
    folder=root/'terminal-paths'; folder.mkdir()
    source=r'''
int opaque(int n);
int b(int n);
int a(int n) { int r; if(n<=32) {r=1;} else {r=b(n-1);} return r; }
int b(int n) { int value; value=opaque(n); int r; r=a(n-1); r+=value; return r; }
int ternary_rec(int n) { int r; r=n<=32 ? 1 : ternary_rec(n-1); return r; }
int loop_rec(int n) { for(int i=32;i<n;++i) loop_rec(n-1); return 1; }
int logical_rec(int n) { int r; r=n>32 && logical_rec(n-1); return r; }
'''
    for tool in ('taskGraph','apac'):
        output=transform(folder,source,tool)
        models=dict(re.findall(r'APAC cost model: (\w+) => (__apac_cost_\d+);',output))
        checks=''
        for name in ('a','ternary_rec','loop_rec','logical_rec'):
            model=models[name]
            checks+=f'if({model}(8)!={model}(16) || {model}(16)==__apac_cost_unknown) return 1;\n'
        checks+=f'if({models["a"]}(64)!=__apac_cost_unknown) return 2;\n'
        checked=folder/'checked.cpp'
        checked.write_text(output+'\nint opaque(int n) {return n;}\nint main(){'+checks+'return 0;}\n')
        run(['g++','-std=c++17','-O1','-fopenmp','-fsanitize=undefined',checked,'-o',folder/'checked'],folder)
        run([folder/'checked'],folder)
    print('PASS: terminal mutual recursion, ternary, short-circuit and zero-trip loop paths stay unscaled',flush=True)
    # A wide acyclic call DAG must not make estimation itself exponential.
    folder=root/'budget'; folder.mkdir()
    source='int chain70(int n) { return n; }\n'
    for i in reversed(range(70)):
        source+=f'int chain{i}(int n) {{ return chain{i+1}(n)+chain{i+1}(n); }}\n'
    output=transform(folder,source,'apac')
    model=re.search(r'APAC cost model: chain0 => (\w+);',output).group(1)
    checked=folder/'checked.cpp'; checked.write_text(output+f'''
int main() {{
 if({model}(100)!=__apac_cost_unknown) return 1;
 if(__apac_cost_mul(__apac_cost_limit,2)!=__apac_cost_limit) return 2;
 if(__apac_cost_add(__apac_cost_limit,2)!=__apac_cost_limit) return 3;
 if(__apac_cost_div(1,0)!=__apac_cost_unknown) return 4;
 return 0;
}}
''')
    run(['g++','-std=c++17','-O0','-fopenmp','-fsanitize=undefined',checked,'-o',folder/'checked'],folder)
    run([folder/'checked'],folder)
    print('PASS: bounded model evaluation and saturating arithmetic',flush=True)
    # Low-cost loop calls must retain their real ordering even when undeferred.
    folder=root/'loopDependencies'; folder.mkdir()
    source=r'''
void add_apacSeq(int *p,int value) { p[0]=p[0]*3+value; }
int scenario(int n) {
 int data[8]={};
 for(int i=0;i<n;++i) { add_apacSeq(&data[i%8],i+1); }
 int result=0; for(int i=0;i<8;++i) result+=data[i]; return result;
}
'''
    output=transform(folder,source,'apac')
    assert 'if(__apac_cost_threshold == 0 ||' in output and '} else {' in output,output
    assert re.search(r'#pragma omp task[^\n]*depend\(inout:',output),output
    assert not re.search(r'#pragma omp task[^\n]*\bif\(',output),output
    assert re.search(r'else \{\s*#pragma omp taskwait\s*__apac_depth_guard __apac_sequential_depth',output),output
    checked=folder/'checked.cpp'; checked.write_text(output+r'''
int main() {int result=0;
#pragma omp parallel
#pragma omp single
 {result=scenario(24);}
 std::printf("%d\n",result); return 0;}
''')
    run(['g++','-std=c++17','-O1','-fopenmp','-fsanitize=undefined',checked,'-o',folder/'checked'],folder)
    expected=run([folder/'checked'],folder,threshold=0,threads=1)
    for threads in (1,2,4):
        for threshold in (1,30,10**9): assert run([folder/'checked'],folder,threshold=threshold,threads=threads)==expected
    print('PASS: direct sequential loop calls avoid task descriptors and retain dependency order',flush=True)

    # Interleave large deferred calls and small direct calls on overlapping
    # representatives. The direct path must wait before writing, then remain
    # visible to later sibling tasks without creating any small task body.
    folder=root/'mixedLoop'; folder.mkdir()
    source=r'''
void update_apacSeq(int *p,int repetitions,int value) {
 for(int j=0;j<repetitions;++j) p[0]=(p[0]+value)%1009;
}
int scenario(int n) {
 int data[3]={};
 for(int i=0;i<n;++i) update_apacSeq(&data[i%3],i%2==0?1000:1,i+1);
 int result=0; for(int i=0;i<3;++i) result+=data[i]; return result;
}
'''
    output=transform(folder,source,'apac')
    checked=folder/'checked.cpp';checked.write_text(instrument(output)+r'''
int main(){int result=0;
#pragma omp parallel
#pragma omp single
 {result=scenario(24);}
 std::printf("%d %d\n",result,cost_test_loops.load());}
''')
    run(['g++','-std=c++17','-O1','-fopenmp','-fsanitize=undefined',checked,'-o',folder/'checked'],folder)
    data=[0,0,0]
    for i in range(24): data[i%3]=(data[i%3]+(1000 if i%2==0 else 1)*(i+1))%1009
    for threads in (1,2,4,8):
        value,created=map(int,run([folder/'checked'],folder,threshold=1000,threads=threads).split())
        assert value==sum(data) and created==12,(value,created,data)
    print('PASS: mixed deferred/direct loop calls preserve overlapping dependencies; only 12 large call tasks created',flush=True)
