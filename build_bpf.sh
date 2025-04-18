#!/bin/bash
clang -g -O2 -target bpf -c probe.bpf.c -o probe.bpf.o
