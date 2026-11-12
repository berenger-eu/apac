#!/usr/bin/env python3
"""Preserve explicit taskwait through cloning and the complete APAC pipeline."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', type=Path,
                    default=Path(os.environ.get('APAC_BUILD_DIR', ROOT/'build')))
BUILD = parser.parse_args().build_dir.resolve()


def run(argv, folder, threads=1):
    env = {**os.environ, 'OMP_NUM_THREADS': str(threads), 'OMP_DYNAMIC': 'FALSE',
           'OMP_WAIT_POLICY': 'PASSIVE', 'APAC_DEPTH_MAX': '16',
           'APAC_COST_THRESHOLD': '1', 'UBSAN_OPTIONS': 'halt_on_error=1'}
    p = subprocess.run([str(a) for a in argv], cwd=folder, env=env,
                       text=True, capture_output=True, timeout=60)
    assert p.returncode == 0, (argv, p.returncode, p.stdout, p.stderr)
    return p.stdout


SUPPORT = r'''
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdio>
#include <omp.h>
// Hidden effects are intentional: the explicit taskwait supplies the ordering.
std::atomic<int> published_apacGlobal{0};
void publish_apacSeq(int value) {
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  published_apacGlobal.store(value);
}
int observe_apacSeq() { return published_apacGlobal.load(); }
'''
BODY = r'''
// Every pass must use the same OpenMP preprocessing mode, even without a
// user-provided -fopenmp. Otherwise a pass can erase this definition.
#ifndef _OPENMP
#error APAC must parse OpenMP consistently throughout the pipeline
#endif
int scenario() {
  publish_apacSeq(41);
  WAIT
  int first; first=observe_apacSeq();
  WAIT
  publish_apacSeq(42);
  WAIT
  int second; second=observe_apacSeq();
  int result; result=first*100+second;
  return result;
}
'''
DRIVER = r'''
int main() {
  int result=0;
#pragma omp parallel
#pragma omp single
  { result=scenario(); }
  std::printf("%d\n",result);
  return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='apac-taskwait-') as temp:
    root = Path(temp)
    for nested in (False, True):
        folder = root/('nested' if nested else 'plain')
        folder.mkdir()
        (folder/'support.hpp').write_text(SUPPORT)
        wait = '#pragma omp taskwait\n'
        if nested:
            wait = 'if (true) {\n'+wait+'}\n'
        source = '#include "support.hpp"\n'+BODY.replace('WAIT', wait)
        for pipeline in ('taskGraph', 'apac', 'apac-cost'):
            current = folder/'input.cpp'
            current.write_text(source)
            if pipeline == 'taskGraph':
                output = run([BUILD/'duplicateFunctions', current, '--', '-std=c++17'], folder)
                # Both the clone and the transformed body retain all three waits.
                assert len(re.findall(r'#pragma omp taskwait\s*\n', output)) == 6, output
                current = folder/'cloned.cpp'
                current.write_text(output)
                output = run([BUILD/'taskGraph', current, '--', '-std=c++17'], folder)
                checked = folder/'checked.cpp'
                checked.write_text(output+DRIVER)
            else:
                # MainParallel supplies the parallel launch in the full pipeline.
                current.write_text(source+'\nint main() { int r; r=scenario(); std::printf("%d\\n",r); return 0; }\n')
                flags = ['--static-cost'] if pipeline == 'apac-cost' else []
                run([BUILD/'apac', current, *flags, '--', '-std=c++17'], folder)
                checked = folder/'APACinput.cpp'
            for opt in (['-O0'], ['-O2'], ['-O1', '-fsanitize=undefined', '-fno-sanitize-recover=all']):
                binary = folder/'checked'
                run(['g++', '-std=c++17', '-fopenmp', *opt, checked, '-o', binary], folder)
                for threads in (1, 2, 4, 8):
                    for repeat in range(3):
                        actual = run([binary], folder, threads)
                        assert actual == '4142\n', (nested, pipeline, opt, threads, repeat, actual)
            print(f'PASS: explicit taskwait ({folder.name}, {pipeline}, -O0/-O2/UBSan, 1/2/4/8 threads)', flush=True)
