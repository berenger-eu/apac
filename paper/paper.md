---
title: 'APAC: Automatic Parallelisation of C++ Using OpenMP Task Dependencies'
tags:
  - parallel computing
  - automatic parallelisation
  - OpenMP
  - source-to-source compilation
  - LLVM/Clang
  - C++
  - high-performance computing
authors:
  - name: Julien Gaupp
    affiliation: "1, 2"
    corresponding: true
  - name: Bérenger Bramas
    affiliation: "1, 2"
  - name: Kylian Gerard
    affiliation: "1"
affiliations:
  - index: 1
    name: Université de Strasbourg, Laboratoire ICube, équipe ICPS, France
  - index: 2
    name: Inria - Grand Est, équipe CAMUS, France
date: 21 April 2026
bibliography: paper.bib
---

# Summary

APAC (*Automatic Parallelisation of C++*) is a source-to-source compiler that transforms sequential C++ programs into parallel code using OpenMP task dependencies. Given a C++ source file, APAC analyses the abstract syntax tree (AST) to identify data dependencies between statements and rewrites the source with `#pragma omp task depend(...)` directives.

APAC is built on LLVM/Clang 18 [@Lattner2004] and is organised as a pipeline of transformation passes. These passes normalise the input source, prepare functions for task-based execution, and insert the OpenMP task and dependency clauses. The full pipeline is invoked through the `apac` executable, while individual passes can also be run separately.

# Statement of Need

Modern CPUs expose many cores, but using them efficiently still requires explicit parallelism. Writing correct OpenMP task-based code is difficult: the programmer must identify data dependencies, choose the right `depend(...)` clauses, and prevent recursive functions from creating too many tasks. Incorrect annotations can introduce data races, while correct ones are time-consuming to write and maintain.

APAC automates this process for sequential C++ programs with divide-and-conquer or task-structured behaviour. It is aimed at researchers and engineers working on scientific computing codes that do not fit the affine loop model used by polyhedral compilers, especially programs built around pointer-based structures, recursion, or irregular control flow.

# State of the Field

A major approach to automatic parallelisation is the *polyhedral model* [@Feautrier1992], which targets affine loop nests. Tools such as PLUTO [@Bondhugula2008] and Polly [@Grosser2012] are effective on array-based numerical kernels, but they do not target pointer-based or recursive programs outside the affine domain.

Mainstream compilers such as GCC, Intel oneAPI, and LLVM also support directive-based parallelisation, but in that setting the programmer still provides the OpenMP annotations. Task-parallel runtime libraries such as Intel TBB [@Reinders2007] and OpenCilk [@Schardl2021] likewise require the programmer to express the task structure explicitly.

APAC addresses a different class of programs. It targets pointer-based and recursive C++ codes, works at the source level, produces standard C++ with OpenMP pragmas, and does not require annotations on the input source. Earlier results on this approach were presented at Compas 2020 [@Kusoglu2020], followed by a study focused on OpenMP-specific constraints at COMPAS 2025 [@Gaupp2025].

# Software Design

APAC is implemented with Clang LibTooling and applies a sequence of source-to-source transformations to C++ input programs. Early transformations normalise the code into a form that simplifies later analyses, while later stages insert OpenMP task directives and the structures required for bounded task creation.

The central analysis is performed in `TaskGraph`, which constructs a statement-level dependency graph by traversing the function body and tracking read/write accesses. For pointer parameters, APAC uses an interprocedural read-only parameter analysis (`ParamWriteAnalyzer`) to determine whether a pointer is modified anywhere in the callee's call tree. This makes it possible to emit more precise dependency clauses and avoid unnecessary serialization.

Task creation is bounded by the `ApacDepth` pass. Recursive calls increment a thread-private depth counter and switch to a sequential fallback beyond a runtime threshold derived from the number of logical processors. This design keeps task creation under control while preserving portability: the generated output is standard C++17 source with OpenMP pragmas and requires no runtime beyond a standard OpenMP implementation.

# Research Impact Statement

The APAC approach was first presented at *Compas 2020* [@Kusoglu2020]. A follow-up paper at *COMPAS 2025* [@Gaupp2025] studied the OpenMP-specific trade-offs of the approach, including the choice of code duplication rather than the OpenMP `if` clause for recursion-depth control. The COMPAS 2025 paper is also available as a HAL preprint.

Benchmark experiments included in the repository report speedups of about 2.5× to 3.3× on divide-and-conquer reduction kernels running on 4 threads, relative to the sequential baseline, without manual annotation of the input source.

The current repository history starts in November 2023 and contains more than 700 commits across branches, covering release preparation, repository restructuring, new transformation features, tests, and performance evaluation.

# AI usage disclosure

GitHub Copilot and ChatGPT were used in a limited support role during this project. They were used to help draft parts of the documentation, review portions of test scaffolding and candidate test oracles, proofread this article, improve the French-to-English translation, and generate the repository logo.

No generative AI tools were used to design the software architecture or implement the core transformation logic. The transformation passes, dependency analysis, benchmark programs, and final validated test logic were developed by the human authors. All AI-assisted outputs were reviewed, corrected when necessary, and validated by the authors before inclusion in the repository or manuscript.

# Acknowledgements

This work was carried out at the Université de Strasbourg (Laboratoire ICube, équipe ICPS) and Inria - Grand Est (équipe CAMUS).

# References
