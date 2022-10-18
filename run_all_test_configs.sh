#!/bin/bash
for i in build build_asan build_tsan build_dbg; do pushd $i/tests; ctest $1; popd; done;
