 mkdir -p build
 pushd build
 cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
 make -j 16
 popd
 
 mkdir -p build_dbg
 pushd build_dbg
 cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
 make -j 16
 popd
 ln -s build_dbg/compile_commands.json compile_commands.json
 
 mkdir -p build_asan
 pushd build_asan
 cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" -DCMAKE_C_FLAGS="-fsanitize=address -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" -DCMAKE_MODULE_LINKER_FLAGS="-fsanitize=address" ..
 make -j 16
 popd
 
 # Due to a gcc bug, a false positive is reported on thread sanitizer, when using condition_variable.wait_for/wait_unitl.
 # The sanitizer build is exclusively performed using clang, hence:
 # - The script explicitly requrires clang to be compiling this build type.
 # - Some features like CTAD are avoided to prevent warnings from being reported as errors.
 # Sources
 # - https://gcc.gnu.org/bugzilla//show_bug.cgi?id=101978
 # - https://stackoverflow.com/q/71188546/4224575
 mkdir -p build_tsan
 pushd build_tsan
 cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" -DCMAKE_C_FLAGS="-fsanitize=thread -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" -DCMAKE_MODULE_LINKER_FLAGS="-fsanitize=thread" -DCMAKE_C_COMPILER=clang  -DCMAKE_CXX_COMPILER=clang++ ..
 make -j 16
 popd
 
 mkdir -p build_ubsan
 pushd build_ubsan
 cmake -DCMAKE_CXX_FLAGS="-fsanitize=undefined -g" -DCMAKE_C_FLAGS="-fsanitize=undefined -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=undefined" -DCMAKE_MODULE_LINKER_FLAGS="-fsanitize=undefined" ..
 make -j 16
 popd

