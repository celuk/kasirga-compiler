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

# kasirga

# alp
