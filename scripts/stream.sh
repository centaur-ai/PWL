#!/bin/bash
./pwl_reasoner_dbg "$1" | tee >(python stream.py > "${1%.*}.jsonl")
