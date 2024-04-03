# cppkaleidoscope
Simple language frontend for llvm.

## Usage
1. Build LLVM >= 18.1.2

Here's my build options (for Apple Silicon Mac):

```bash
git clone https://github.com/llvm/llvm-project.git --depth=1

cd llvm-project

cmake -S llvm -B build -G Ninja -DCMAKE_INSTALL_PREFIX=/opt/llvm -DCMAKE_BUILD_TYPE=RelWithDebInfo \
-DLLVM_PARALLEL_COMPILE_JOBS=16 -DLLVM_PARALLEL_LINK_JOBS=16 \
-DLLVM_ENABLE_PROJECTS="clang;lld;clang-tools-extra;mlir;lldb" \
-DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang \
-DDEFAULT_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk \
-DLLVM_ENABLE_RUNTIMES="all" -DCMAKE_OSX_ARCHITECTURES='arm64' \
-DLLVM_TARGETS_TO_BUILD='AArch64;RISCV' \
-DLIBCXX_INSTALL_MODULES=ON

cmake --build build

sudo ninja -C install
```
2. configure rpath and path

3. build the project
The following command will bring you a interactive repl to play with.
```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
cmake --build build
./build/cpplox-repl
```

