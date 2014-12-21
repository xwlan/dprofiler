dprofiler
=========
A open source fast, lightweight win32/64 CPU and memory profiler


goal
=====
1, very low overhead and impact to target application

2, portable, easy to deploy and easy to use, intuitive

3, both flat and graphic performance data

4, collect data on machine A, analyze data on machine on B

5, both CPU/memory profiling supported

6, easy to pinpoint CPU and memory hot path.

7, both 32/64 bits are supported.

8, the report can be re-analyzed with more accurate symbols anytime.

9, dynamic attach to and detach from target without code re-compiling,
   only pdb are required to parse report.


features
=======
CPU profiling support:

1, IPs On CPU  (what code executing on CPU)

2, Function ( IP grouped by function)

3, Module   ( IP grouped by module)

4, Thread   ( IP grouped by thread)

5, CallTree ( IP grouped by calltree)

6, FlameGraph ( IP visualized by flame graph, pinpoint hot path intuitively)

7, History   (The whole CPU execution history, pinpoint CPU high usage intuitively)


Memory profiling support:

1, Outstanding Allocation (Examine memory leak, include heap/handle/GDI/virtual allocation)

2, Heap Allocation by Module ( Examine high heap allocation by module)

3, Heap Allocation by CallTree (Examine high heap allocation by code path)

4, Heap Allocation by FlameGraph (Examine high heap allocation intuitively by graph)


build
====== 
open dprofiler.sln, rebuild solution. VS 2012 with update 4

usage
======
checkout gallery/*.png for snapshots and see cpudemo.swf, mmdemo.swf.
