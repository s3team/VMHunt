# VMHunt: Extraction and Simplification of Virtualized Binary Code

VMHunt is a set of tools for analyzing virtualized binary code. Now we only support 32 bit traces.

## Prerequisites
1. PIN tools from Intel. I tested version 2.13 and 3.2, but other versions probably work as well.
2. g++ compiler (6.0 version or above).

## How to compile and install
1. Compile the tracer: run `make PIN_ROOT=PinDirectory TARGET=ia32 $*` in the `tracer` directory.
2. Compile VMHunt: run `make` in the project root directory.

## How to use
1. Use the tracer to record an execution trace.
   `pin -t tracer/obj-ia32/instracelog.so -- yourprogram`
2. Extract virtualized snippet in the trace.
   `./vmextract tracefile`
3. Backward slice the trace.
   `./slicer tracefile`
4. Run MG symbolic execution
   `./mgse tracefile`
