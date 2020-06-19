**D Profiler**
a lightweight, low overhead CPU Memory IO and Lock profiler for Windows x86/x64

**features**
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

**CPU**
1, IPs On CPU  (what code executing on CPU)
2, Function ( IP grouped by function)
3, Module   ( IP grouped by module)
4, Thread   ( IP grouped by thread)
5, CallTree ( IP grouped by calltree)
6, FlameGraph ( IP visualized by flame graph, pinpoint hot path intuitively)
7, History   (The whole CPU execution history, pinpoint CPU high usage intuitively)

**Memory**
1, Outstanding Allocation (Examine memory leak, include heap/handle/GDI/virtual allocation)
2, Heap Allocation by Module ( Examine high heap allocation by module)
3, Heap Allocation by CallTree (Examine high heap allocation by code path)
4, Heap Allocation by FlameGraph (Examine high heap allocation intuitively by graph)

**IO**

1,  General file system IO, socket IO, both synchronous and asynchronous (overlapped) are supported.
2,  IO Counters include max/min latency for IO objects, IO R/W counts, IO size etc
3,  IO stack trace (right click in IO record in UI can pop up stack trace menu)

**Lock**

1, Only critical section, SWR are supported
2, Lock counters include lock acquisition/release count, max/min latency etc
3, Lock stack trace (right click in Lock record in UI can pop up stack trace menu)

**Build**
open dprofiler.sln, rebuild solution. at least VS 2012 with update 4 required.

**Usage**
checkout cpudemo.swf, mmdemo.swf, or gallery/*.png for snapshots.

Example (a heap profile flamegraph):
[![Example](https://github.com/xwlan/dprofiler/blob/master/gallery/MM-Heap%20By%20FlameGraph.PNG)](https://github.com/xwlan/dprofiler/blob/master/gallery/MM-Heap%20By%20FlameGraph.PNG)


