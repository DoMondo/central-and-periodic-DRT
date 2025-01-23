#!/usr/bin/env bash


make host-nosched N=64
make host-nosched N=128
make host-nosched N=256
make host-nosched N=512
make host-nosched N=1024
make host-nosched N=2048
make host-nosched N=4096

#make arm64-run-sched N=4
#make arm64-run-sched N=8
#make arm64-run-sched N=16
#make arm64-run-sched N=32
#make arm64-run-sched N=64
#make arm64-run-sched N=128
#make arm64-run-sched N=256
#make arm64-run-sched N=512
