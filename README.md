# kasirga-compiler
This repository includes 'kasirga' compiler and 'alp' object code to instruction specific encrypted hex code obfuscator.

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

You can use to obfuscate any compiled object code to non-encrypted or encrypted hex code.

**Example usages:**

Gives output as non-encrypted out.hex:

```bash
alp -d example.o
```

Gives output as add, sub instructions encrypted out.hex:

```bash
/home/shc/Desktop/kasirga-compiler/build/bin/alp --add --sub -d example.o
```

Gives output as non-encrypted out.hex (bits' length 90):

```bash
/home/shc/Desktop/kasirga-compiler/build/bin/alp --bits 000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 -d example.o
```

Gives output as some instructions encrypted out.hex (most left bit is 0. most right bit is 89. instruction):

```bash
/home/shc/Desktop/kasirga-compiler/build/bin/alp --bits 100000000000000001011001011001000001000100100100010100001010000010001111001000000000000000 -d example.o
```

Gives output as some instructions encrypted out.hex (most left bit is 0. most right bit is 89. instruction):

```bash
/home/shc/Desktop/kasirga-compiler/build/bin/alp --addi --bits 100000000000000001011001011001000001000100100100010100001010000010001111001000000000000000 -d example.o
```


**Encryptable Instruction List (90 instructions):**

**0 -)** beq
**1 -)** bne
**2 -)** blt
**3 -)** bge
**4 -)** bltu
**5 -)** bgeu
**6 -)** jalr
**7 -)** jal
**8 -)** lui
**9 -)** auipc
**10-)** addi
**11-)** slli
**12-)** slti
**13-)** sltiu
**14-)** xori
**15-)** srli
**16-)** srai
**17-)** ori
**18-)** andi
**19-)** add
**20-)** sub
**21-)** sll
**22-)** slt
**23-)** sltu
**24-)** xor
**25-)** srl
**26-)** sra
**27-)** or
**28-)** and
**29-)** addiw
**30-)** slliw
**31-)** srliw
**32-)** sraiw
**33-)** addw
**34-)** subw
**35-)** sllw
**36-)** srlw
**37-)** sraw
**38-)** lb
**39-)** lh
**40-)** lw
**41-)** ld
**42-)** lbu
**43-)** lhu
**44-)** lwu
**45-)** sb
**46-)** sh
**47-)** sw
**48-)** sd
**49-)** fence
**50-)** fence_i
**51-)** mul
**52-)** mulh
**53-)** mulhsu
**54-)** mulhu
**55-)** div
**56-)** divu
**57-)** rem
**58-)** remu
**59-)** mulw
**60-)** divw
**61-)** divuw
**62-)** remw
**63-)** remuw
**64-)** lr_w
**65-)** sc_w
**66-)** lr_d
**67-)** sc_d
**68-)** ecall
**69-)** ebreak
**70-)** uret
**71-)** mret
**72-)** dret
**73-)** sfence_vma
**74-)** wfi
**75-)** csrrw
**76-)** csrrs
**77-)** csrrc
**78-)** csrrwi
**79-)** csrrsi
**80-)** csrrci
**81-)** slli_rv32
**82-)** srli_rv32
**83-)** srai_rv32
**84-)** rdcycle
**85-)** rdtime
**86-)** rdinstret
**87-)** rdcycleh
**88-)** rdtimeh
**89-)** rdinstreth



