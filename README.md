# kasirga-compiler
This repository includes 'kasirga' compiler and 'alp' object code to encrypted hex code obfuscator.

I assume that gnu compiler toolchain (gcc, g++) installed on your host pc.

# CMake and Ninja Installation
```bash
sudo snap install cmake --classic

sudo apt install ninja-build
```

# LLVM Installation
```bash
git clone https://github.com/llvm/llvm-project.git && cd llvm-project && mkdir build && cd build && cmake -G Ninja -DLLVM_ENABLE_PROJECTS=clang -DLLVM_TARGETS_TO_BUILD=all -DLLVM_ENABLE_LIBCXX=ON -DCMAKE_BUILD_TYPE=Release -DLLVM_INSTALL_UTILS=ON -DBUILD_SHARED_LIBS=True -DLLVM_USE_SPLIT_DWARF=True -DLLVM_OPTIMIZED_TABLEGEN=True -DLLVM_BUILD_TESTS=True -DLLVM_PARALLEL_LINK_JOBS=1 ../llvm && cmake --build .
```

Just you need to copy and paste above commands to terminal, it can take too much time (about 2-4 hours).

# Cloning Repository
```bash
git clone https://github.com/seyyidhikmetcelik/kasirga-compiler
```
# Building Repository

**1-)** Create a build directory and change directory:
```bash
mkdir build
cd build
```

**2-)** Export your LLVM directories:

```bash
export LLVM_PROJECT_DIR={your-llvm-project-directory}
export LLVM_BUILD_DIR={your-llvm-install-or-build-directory}
```

Example:
```bash
export LLVM_PROJECT_DIR=~/llvm/llvm-project
export LLVM_BUILD_DIR=~/llvm/llvm-project/build
```

**3-)** Configure with cmake:

```bash
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_BUILD_DIR -DCMAKE_MODULE_PATH=$LLVM_PROJECT_DIR/clang/cmake/modules ..
```

**4-)** Build with cmake or make:

```bash
cmake --build .
```

Alternative build with make:

```bash
make
```

If you can't build because of a compiler error, install a new compiler if does not exist, change your compiler as for example:

```bash
export CC=clang-11
export CXX=clang++-11
```

then delete build directory and start with the first step again.

You can look here for changing compiler that I answered: https://stackoverflow.com/questions/68349442/how-to-fix-undefined-reference-llvm-error-while-linking-cxx-executable/68568867#68568867

**Now you can find your executables in /kasirga-compiler/build/bin folder as alp and kasirga variants.**

# kasirga

You can use clang-like compiler but if you compile a .c code as object code it will also run alp.

**Example usages:**

Host pc executable:

```bash
kasirga example.c -o example
```

Host pc assembly code:

```bash
/home/shc/Desktop/kasirga-compiler/build/bin/kasirga -S example.c -o example.s 
```

Host pc llvm ir code:

```bash
/home/shc/Desktop/kasirga-compiler/build/bin/kasirga -S -emit-llvm example.c -o example.ll 
```

Host pc object code:

```bash
/home/shc/Desktop/kasirga-compiler/build/bin/kasirga -c example.c -o example.o
```

Riscv32 object code (also runs alp):

```bash
/home/shc/Desktop/kasirga-compiler/build/bin/kasirga -c -target riscv32-unknown-elf --sysroot=/home/shc/riscv-new/_install/riscv64-unknown-elf --gcc-toolchain=/home/shc/riscv-new/_install/ example.c -o example.o
```

I am using --sysroot and --gcc-toolchain flags to compile for riscv. You need to have riscv-gnu-toolchain pre installed.
For --sysroot and --gcc-toolchain flags you can look here that I answered: https://stackoverflow.com/questions/68580399/using-clang-to-compile-for-risc-v

# alp

You can use to obfuscate any compiled object code to 

Example usage:

