//===-- alp.cpp - Object file dumping utility for llvm -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This program is a utility that works like binutils "objdump", that is, it
// dumps out a plethora of information about an object file depending on the
// flags.
//
// The flags and output of this program should be near identical to those of
// binutils objdump.
//
//===----------------------------------------------------------------------===//

// her seyin basi     Name = unwrapOrError(Symbol.getName(), FileName, ArchiveName, ArchitectureName); 
//#include "MCTargetDesc/RISCVBaseInfo.h"
//#include "MCTargetDesc/RISCVFixupKinds.h"
//#include "MCTargetDesc/RISCVMCExpr.h"
//#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "MCTargetDesc/RISCVInstPrinter.h"
#include "RISCV.h"
//#include "RISCVTargetMachine.h"
#include "llvm/IR/Instruction.h"
#include <stdlib.h> // ekledim
#include "alp.h"
#include "COFFDump.h"
#include "ELFDump.h"
#include "MachODump.h"
#include "WasmDump.h"
#include "XCOFFDump.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetOperations.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/Triple.h"
#include "llvm/CodeGen/FaultMaps.h"
#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/DebugInfo/Symbolize/Symbolize.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCDisassembler/MCRelocationInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Object/Archive.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/COFFImportFile.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/MachO.h"
#include "llvm/Object/MachOUniversal.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/Wasm.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <fstream>
#include <string>

//////////////////////
llvm::StringRef opcodeList[90] = {
"beq"               
,"bne"               
,"blt"               
,"bge"               
,"bltu"              
,"bgeu"              
,"jalr"              
,"jal"               
,"lui"               
,"auipc"             
,"addi"              
,"slli"              
,"slti"              
,"sltiu"             
,"xori"              
,"srli"              
,"srai"              
,"ori"               
,"andi"              
,"add"               
,"sub"               
,"sll"               
,"slt"               
,"sltu"              
,"xor"               
,"srl"               
,"sra"               
,"or"                
,"and"               
,"addiw"             
,"slliw"             
,"srliw"             
,"sraiw"             
,"addw"              
,"subw"              
,"sllw"              
,"srlw"              
,"sraw"              
,"lb"                
,"lh"                
,"lw"                
,"ld"                
,"lbu"               
,"lhu"               
,"lwu"               
,"sb"                
,"sh"                
,"sw"                
,"sd"                
,"fence"             
,"fence_i"           
,"mul"               
,"mulh"              
,"mulhsu"            
,"mulhu"             
,"div"               
,"divu"              
,"rem"               
,"remu"              
,"mulw"              
,"divw"              
,"divuw"             
,"remw"              
,"remuw"             
,"lr_w"              
,"sc_w"              
,"lr_d"              
,"sc_d"              
,"ecall"             
,"ebreak"            
,"uret"              
,"mret"              
,"dret"              
,"sfence_vma"        
,"wfi"               
,"csrrw"             
,"csrrs"             
,"csrrc"             
,"csrrwi"            
,"csrrsi"            
,"csrrci"            
,"slli_rv32"         
,"srli_rv32"         
,"srai_rv32"         
,"rdcycle"           
,"rdtime"            
,"rdinstret"         
,"rdcycleh"          
,"rdtimeh"           
,"rdinstreth"
};

//////////ekleme
int g_argc;
char **g_argv;

using namespace llvm;
using namespace llvm::object;
using namespace llvm::objdump;

#define DEBUG_TYPE "objdump"

static cl::OptionCategory ObjdumpCat("llvm-objdump Options");

////////////////ekleme
cl::opt<std::string> objdump::dosya(
    "dosya",
    cl::desc(
        "if given file option"),
    cl::cat(ObjdumpCat));

cl::opt<std::string> objdump::bits(
    "bits",
    cl::desc(
        "if given bits option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::beq(
    "beq",
    cl::desc("if given beq option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::bne(
    "bne",
    cl::desc("if given bne option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::blt(
    "blt",
    cl::desc("if given blt option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::bge(
    "bge",
    cl::desc("if given bge option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::bltu(
    "bltu",
    cl::desc("if given bltu option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::bgeu(
    "bgeu",
    cl::desc("if given bgeu option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::jalr(
    "jalr",
    cl::desc("if given jalr option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::jal(
    "jal",
    cl::desc("if given jal option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::lui(
    "lui",
    cl::desc("if given lui option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::auipc(
    "auipc",
    cl::desc("if given auipc option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::addi(
    "addi",
    cl::desc("if given addi option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::slli(
    "slli",
    cl::desc("if given slli option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::slti(
    "slti",
    cl::desc("if given slti option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sltiu(
    "sltiu",
    cl::desc("if given sltiu option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::xori(
    "xori",
    cl::desc("if given xori option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::srli(
    "srli",
    cl::desc("if given srli option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::srai(
    "srai",
    cl::desc("if given srai option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::ori(
    "ori",
    cl::desc("if given ori option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::andi(
    "andi",
    cl::desc("if given andi option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::add(
    "add",
    cl::desc("if given add option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sub(
    "sub",
    cl::desc("if given sub option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sll(
    "sll",
    cl::desc("if given sll option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::slt(
    "slt",
    cl::desc("if given slt option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sltu(
    "sltu",
    cl::desc("if given sltu option"),
    cl::cat(ObjdumpCat));

// xor_
cl::opt<bool> objdump::xor_(
    "xor",
    cl::desc("if given xor_ option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::srl(
    "srl",
    cl::desc("if given srl option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sra(
    "sra",
    cl::desc("if given sra option"),
    cl::cat(ObjdumpCat));

// or_
cl::opt<bool> objdump::or_(
    "or",
    cl::desc("if given or_ option"),
    cl::cat(ObjdumpCat));

// and_
cl::opt<bool> objdump::and_(
    "and",
    cl::desc("if given and_ option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::addiw(
    "addiw",
    cl::desc("if given addiw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::slliw(
    "slliw",
    cl::desc("if given slliw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::srliw(
    "srliw",
    cl::desc("if given srliw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sraiw(
    "sraiw",
    cl::desc("if given sraiw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::addw(
    "addw",
    cl::desc("if given addw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::subw(
    "subw",
    cl::desc("if given subw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sllw(
    "sllw",
    cl::desc("if given sllw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::srlw(
    "srlw",
    cl::desc("if given srlw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sraw(
    "sraw",
    cl::desc("if given sraw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::lb(
    "lb",
    cl::desc("if given lb option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::lh(
    "lh",
    cl::desc("if given lh option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::lw(
    "lw",
    cl::desc("if given lw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::ld(
    "ld",
    cl::desc("if given ld option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::lbu(
    "lbu",
    cl::desc("if given lbu option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::lhu(
    "lhu",
    cl::desc("if given lhu option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::lwu(
    "lwu",
    cl::desc("if given lwu option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sb(
    "sb",
    cl::desc("if given sb option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sh(
    "sh",
    cl::desc("if given sh option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sw(
    "sw",
    cl::desc("if given sw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sd(
    "sd",
    cl::desc("if given sd option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::fence(
    "fence",
    cl::desc("if given fence option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::fence_i(
    "fence_i",
    cl::desc("if given fence_i option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::mul(
    "mul",
    cl::desc("if given mul option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::mulh(
    "mulh",
    cl::desc("if given mulh option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::mulhsu(
    "mulhsu",
    cl::desc("if given mulhsu option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::mulhu(
    "mulhu",
    cl::desc("if given mulhu option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::div(
    "div",
    cl::desc("if given div option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::divu(
    "divu",
    cl::desc("if given divu option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::rem(
    "rem",
    cl::desc("if given rem option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::remu(
    "remu",
    cl::desc("if given remu option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::mulw(
    "mulw",
    cl::desc("if given mulw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::divw(
    "divw",
    cl::desc("if given divw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::divuw(
    "divuw",
    cl::desc("if given divuw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::remw(
    "remw",
    cl::desc("if given remw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::remuw(
    "remuw",
    cl::desc("if given remuw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::lr_w(
    "lr_w",
    cl::desc("if given lr_w option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sc_w(
    "sc_w",
    cl::desc("if given sc_w option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::lr_d(
    "lr_d",
    cl::desc("if given lr_d option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sc_d(
    "sc_d",
    cl::desc("if given sc_d option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::ecall(
    "ecall",
    cl::desc("if given ecall option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::ebreak(
    "ebreak",
    cl::desc("if given ebreak option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::uret(
    "uret",
    cl::desc("if given uret option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::mret(
    "mret",
    cl::desc("if given mret option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::dret(
    "dret",
    cl::desc("if given dret option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::sfence_vma(
    "sfence_vma",
    cl::desc("if given sfence_vma option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::wfi(
    "wfi",
    cl::desc("if given wfi option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::csrrw(
    "csrrw",
    cl::desc("if given csrrw option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::csrrs(
    "csrrs",
    cl::desc("if given csrrs option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::csrrc(
    "csrrc",
    cl::desc("if given csrrc option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::csrrwi(
    "csrrwi",
    cl::desc("if given csrrwi option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::csrrsi(
    "csrrsi",
    cl::desc("if given csrrsi option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::csrrci(
    "csrrci",
    cl::desc("if given csrrci option"),
    cl::cat(ObjdumpCat));

// customs
// custom0
//cl::opt<bool> objdump::custom0(
//    "custom0",
//    cl::desc("if given custom0 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom0_rs1(
//    "custom0_rs1",
//    cl::desc("if given custom0_rs1 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom0_rs1_rs2(
//    "custom0_rs1_rs2",
//    cl::desc("if given custom0_rs1_rs2 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom0_rd(
//    "custom0_rd",
//    cl::desc("if given custom0_rd option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom0_rd_rs1(
//    "custom0_rd_rs1",
//    cl::desc("if given custom0_rd_rs1 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom0_rd_rs1_rs2(
//    "custom0_rd_rs1_rs2",
//    cl::desc("if given custom0_rd_rs1_rs2 option"),
//    cl::cat(ObjdumpCat));
//
//// custom1
//cl::opt<bool> objdump::custom1(
//    "custom1",
//    cl::desc("if given custom1 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom1_rs1(
//    "custom1_rs1",
//    cl::desc("if given custom1_rs1 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom1_rs1_rs2(
//    "custom1_rs1_rs2",
//    cl::desc("if given custom1_rs1_rs2 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom1_rd(
//    "custom1_rd",
//    cl::desc("if given custom1_rd option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom1_rd_rs1(
//    "custom1_rd_rs1",
//    cl::desc("if given custom1_rd_rs1 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom1_rd_rs1_rs2(
//    "custom1_rd_rs1_rs2",
//    cl::desc("if given custom1_rd_rs1_rs2 option"),
//    cl::cat(ObjdumpCat));
//
//// custom2
//cl::opt<bool> objdump::custom2(
//    "custom2",
//    cl::desc("if given custom2 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom2_rs1(
//    "custom2_rs1",
//    cl::desc("if given custom2_rs1 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom2_rs1_rs2(
//    "custom2_rs1_rs2",
//    cl::desc("if given custom2_rs1_rs2 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom2_rd(
//    "custom2_rd",
//    cl::desc("if given custom2_rd option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom2_rd_rs1(
//    "custom2_rd_rs1",
//    cl::desc("if given custom2_rd_rs1 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom2_rd_rs1_rs2(
//    "custom2_rd_rs1_rs2",
//    cl::desc("if given custom2_rd_rs1_rs2 option"),
//    cl::cat(ObjdumpCat));
//
//// custom3
//cl::opt<bool> objdump::custom3(
//    "custom3",
//    cl::desc("if given custom3 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom3_rs1(
//    "custom3_rs1",
//    cl::desc("if given custom3_rs1 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom3_rs1_rs2(
//    "custom3_rs1_rs2",
//    cl::desc("if given custom3_rs1_rs2 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom3_rd(
//    "custom3_rd",
//    cl::desc("if given custom3_rd option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom3_rd_rs1(
//    "custom3_rd_rs1",
//    cl::desc("if given custom3_rd_rs1 option"),
//    cl::cat(ObjdumpCat));
//
//cl::opt<bool> objdump::custom3_rd_rs1_rs2(
//    "custom3_rd_rs1_rs2",
//    cl::desc("if given custom3_rd_rs1_rs2 option"),
//    cl::cat(ObjdumpCat));
//

cl::opt<bool> objdump::slli_rv32(
    "slli_rv32",
    cl::desc("if given slli_rv32 option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::srli_rv32(
    "srli_rv32",
    cl::desc("if given srli_rv32 option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::srai_rv32(
    "srai_rv32",
    cl::desc("if given srai_rv32 option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::rdcycle(
    "rdcycle",
    cl::desc("if given rdcycle option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::rdtime(
    "rdtime",
    cl::desc("if given rdtime option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::rdinstret(
    "rdinstret",
    cl::desc("if given rdinstret option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::rdcycleh(
    "rdcycleh",
    cl::desc("if given rdcycleh option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::rdtimeh(
    "rdtimeh",
    cl::desc("if given rdtimeh option"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::rdinstreth(
    "rdinstreth",
    cl::desc("if given rdinstreth option"),
    cl::cat(ObjdumpCat));


//cl::opt<bool> objdump::add(
//    "add",
//    cl::desc("if given add option"),
//    cl::cat(ObjdumpCat));
//// --add olarak kullanılıyor
//
//cl::opt<bool> objdump::sub(
//    "sub",
//    cl::desc("if given sub option"),
//    cl::cat(ObjdumpCat));






static cl::opt<uint64_t> AdjustVMA(
    "adjust-vma",
    cl::desc("Increase the displayed address by the specified offset"),
    cl::value_desc("offset"), cl::init(0), cl::cat(ObjdumpCat));

static cl::opt<bool>
    AllHeaders("all-headers",
               cl::desc("Display all available header information"),
               cl::cat(ObjdumpCat));
static cl::alias AllHeadersShort("x", cl::desc("Alias for --all-headers"),
                                 cl::NotHidden, cl::Grouping,
                                 cl::aliasopt(AllHeaders));

static cl::opt<std::string>
    ArchName("arch-name",
             cl::desc("Target arch to disassemble for, "
                      "see -version for available targets"),
             cl::cat(ObjdumpCat));

cl::opt<bool>
    objdump::ArchiveHeaders("archive-headers",
                            cl::desc("Display archive header information"),
                            cl::cat(ObjdumpCat));
static cl::alias ArchiveHeadersShort("a",
                                     cl::desc("Alias for --archive-headers"),
                                     cl::NotHidden, cl::Grouping,
                                     cl::aliasopt(ArchiveHeaders));

cl::opt<bool> objdump::Demangle("demangle", cl::desc("Demangle symbols names"),
                                cl::init(false), cl::cat(ObjdumpCat));
static cl::alias DemangleShort("C", cl::desc("Alias for --demangle"),
                               cl::NotHidden, cl::Grouping,
                               cl::aliasopt(Demangle));

cl::opt<bool> objdump::Disassemble(
    "disassemble",
    cl::desc("Display assembler mnemonics for the machine instructions"),
    cl::cat(ObjdumpCat));
static cl::alias DisassembleShort("d", cl::desc("Alias for --disassemble"),
                                  cl::NotHidden, cl::Grouping,
                                  cl::aliasopt(Disassemble));

cl::opt<bool> objdump::DisassembleAll(
    "disassemble-all",
    cl::desc("Display assembler mnemonics for the machine instructions"),
    cl::cat(ObjdumpCat));
static cl::alias DisassembleAllShort("D",
                                     cl::desc("Alias for --disassemble-all"),
                                     cl::NotHidden, cl::Grouping,
                                     cl::aliasopt(DisassembleAll));

cl::opt<bool> objdump::SymbolDescription(
    "symbol-description",
    cl::desc("Add symbol description for disassembly. This "
             "option is for XCOFF files only"),
    cl::init(false), cl::cat(ObjdumpCat));

static cl::list<std::string>
    DisassembleSymbols("disassemble-symbols", cl::CommaSeparated,
                       cl::desc("List of symbols to disassemble. "
                                "Accept demangled names when --demangle is "
                                "specified, otherwise accept mangled names"),
                       cl::cat(ObjdumpCat));

static cl::opt<bool> DisassembleZeroes(
    "disassemble-zeroes",
    cl::desc("Do not skip blocks of zeroes when disassembling"),
    cl::cat(ObjdumpCat));
static cl::alias
    DisassembleZeroesShort("z", cl::desc("Alias for --disassemble-zeroes"),
                           cl::NotHidden, cl::Grouping,
                           cl::aliasopt(DisassembleZeroes));

static cl::list<std::string>
    DisassemblerOptions("disassembler-options",
                        cl::desc("Pass target specific disassembler options"),
                        cl::value_desc("options"), cl::CommaSeparated,
                        cl::cat(ObjdumpCat));
static cl::alias
    DisassemblerOptionsShort("M", cl::desc("Alias for --disassembler-options"),
                             cl::NotHidden, cl::Grouping, cl::Prefix,
                             cl::CommaSeparated,
                             cl::aliasopt(DisassemblerOptions));

cl::opt<DIDumpType> objdump::DwarfDumpType(
    "dwarf", cl::init(DIDT_Null), cl::desc("Dump of dwarf debug sections:"),
    cl::values(clEnumValN(DIDT_DebugFrame, "frames", ".debug_frame")),
    cl::cat(ObjdumpCat));

static cl::opt<bool> DynamicRelocations(
    "dynamic-reloc",
    cl::desc("Display the dynamic relocation entries in the file"),
    cl::cat(ObjdumpCat));
static cl::alias DynamicRelocationShort("R",
                                        cl::desc("Alias for --dynamic-reloc"),
                                        cl::NotHidden, cl::Grouping,
                                        cl::aliasopt(DynamicRelocations));

static cl::opt<bool>
    FaultMapSection("fault-map-section",
                    cl::desc("Display contents of faultmap section"),
                    cl::cat(ObjdumpCat));

static cl::opt<bool>
    FileHeaders("file-headers",
                cl::desc("Display the contents of the overall file header"),
                cl::cat(ObjdumpCat));
static cl::alias FileHeadersShort("f", cl::desc("Alias for --file-headers"),
                                  cl::NotHidden, cl::Grouping,
                                  cl::aliasopt(FileHeaders));

cl::opt<bool>
    objdump::SectionContents("full-contents",
                             cl::desc("Display the content of each section"),
                             cl::cat(ObjdumpCat));
static cl::alias SectionContentsShort("s",
                                      cl::desc("Alias for --full-contents"),
                                      cl::NotHidden, cl::Grouping,
                                      cl::aliasopt(SectionContents));

static cl::list<std::string> InputFilenames(cl::Positional,
                                            cl::desc("<input object files>"),
                                            cl::ZeroOrMore,
                                            cl::cat(ObjdumpCat));

static cl::opt<bool>
    PrintLines("line-numbers",
               cl::desc("Display source line numbers with "
                        "disassembly. Implies disassemble object"),
               cl::cat(ObjdumpCat));
static cl::alias PrintLinesShort("l", cl::desc("Alias for --line-numbers"),
                                 cl::NotHidden, cl::Grouping,
                                 cl::aliasopt(PrintLines));

static cl::opt<bool> MachOOpt("macho",
                              cl::desc("Use MachO specific object file parser"),
                              cl::cat(ObjdumpCat));
static cl::alias MachOm("m", cl::desc("Alias for --macho"), cl::NotHidden,
                        cl::Grouping, cl::aliasopt(MachOOpt));

cl::opt<std::string> objdump::MCPU(
    "mcpu", cl::desc("Target a specific cpu type (-mcpu=help for details)"),
    cl::value_desc("cpu-name"), cl::init(""), cl::cat(ObjdumpCat));

cl::list<std::string> objdump::MAttrs("mattr", cl::CommaSeparated,
                                      cl::desc("Target specific attributes"),
                                      cl::value_desc("a1,+a2,-a3,..."),
                                      cl::cat(ObjdumpCat));

cl::opt<bool> objdump::NoShowRawInsn(
    "no-show-raw-insn",
    cl::desc(
        "When disassembling instructions, do not print the instruction bytes."),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::NoLeadingAddr("no-leading-addr",
                                     cl::desc("Print no leading address"),
                                     cl::cat(ObjdumpCat));

static cl::opt<bool> RawClangAST(
    "raw-clang-ast",
    cl::desc("Dump the raw binary contents of the clang AST section"),
    cl::cat(ObjdumpCat));

cl::opt<bool>
    objdump::Relocations("reloc",
                         cl::desc("Display the relocation entries in the file"),
                         cl::cat(ObjdumpCat));
static cl::alias RelocationsShort("r", cl::desc("Alias for --reloc"),
                                  cl::NotHidden, cl::Grouping,
                                  cl::aliasopt(Relocations));

cl::opt<bool>
    objdump::PrintImmHex("print-imm-hex",
                         cl::desc("Use hex format for immediate values"),
                         cl::cat(ObjdumpCat));

cl::opt<bool>
    objdump::PrivateHeaders("private-headers",
                            cl::desc("Display format specific file headers"),
                            cl::cat(ObjdumpCat));
static cl::alias PrivateHeadersShort("p",
                                     cl::desc("Alias for --private-headers"),
                                     cl::NotHidden, cl::Grouping,
                                     cl::aliasopt(PrivateHeaders));

cl::list<std::string>
    objdump::FilterSections("section",
                            cl::desc("Operate on the specified sections only. "
                                     "With -macho dump segment,section"),
                            cl::cat(ObjdumpCat));
static cl::alias FilterSectionsj("j", cl::desc("Alias for --section"),
                                 cl::NotHidden, cl::Grouping, cl::Prefix,
                                 cl::aliasopt(FilterSections));

cl::opt<bool> objdump::SectionHeaders(
    "section-headers",
    cl::desc("Display summaries of the headers for each section."),
    cl::cat(ObjdumpCat));
static cl::alias SectionHeadersShort("headers",
                                     cl::desc("Alias for --section-headers"),
                                     cl::NotHidden,
                                     cl::aliasopt(SectionHeaders));
static cl::alias SectionHeadersShorter("h",
                                       cl::desc("Alias for --section-headers"),
                                       cl::NotHidden, cl::Grouping,
                                       cl::aliasopt(SectionHeaders));

static cl::opt<bool>
    ShowLMA("show-lma",
            cl::desc("Display LMA column when dumping ELF section headers"),
            cl::cat(ObjdumpCat));

static cl::opt<bool> PrintSource(
    "source",
    cl::desc(
        "Display source inlined with disassembly. Implies disassemble object"),
    cl::cat(ObjdumpCat));
static cl::alias PrintSourceShort("S", cl::desc("Alias for -source"),
                                  cl::NotHidden, cl::Grouping,
                                  cl::aliasopt(PrintSource));

static cl::opt<uint64_t>
    StartAddress("start-address", cl::desc("Disassemble beginning at address"),
                 cl::value_desc("address"), cl::init(0), cl::cat(ObjdumpCat));
static cl::opt<uint64_t> StopAddress("stop-address",
                                     cl::desc("Stop disassembly at address"),
                                     cl::value_desc("address"),
                                     cl::init(UINT64_MAX), cl::cat(ObjdumpCat));

cl::opt<bool> objdump::SymbolTable("syms", cl::desc("Display the symbol table"),
                                   cl::cat(ObjdumpCat));
static cl::alias SymbolTableShort("t", cl::desc("Alias for --syms"),
                                  cl::NotHidden, cl::Grouping,
                                  cl::aliasopt(SymbolTable));

static cl::opt<bool> DynamicSymbolTable(
    "dynamic-syms",
    cl::desc("Display the contents of the dynamic symbol table"),
    cl::cat(ObjdumpCat));
static cl::alias DynamicSymbolTableShort("T",
                                         cl::desc("Alias for --dynamic-syms"),
                                         cl::NotHidden, cl::Grouping,
                                         cl::aliasopt(DynamicSymbolTable));

cl::opt<std::string> objdump::TripleName(
    "triple",
    cl::desc(
        "Target triple to disassemble for, see -version for available targets"),
    cl::cat(ObjdumpCat));

cl::opt<bool> objdump::UnwindInfo("unwind-info",
                                  cl::desc("Display unwind information"),
                                  cl::cat(ObjdumpCat));
static cl::alias UnwindInfoShort("u", cl::desc("Alias for --unwind-info"),
                                 cl::NotHidden, cl::Grouping,
                                 cl::aliasopt(UnwindInfo));

static cl::opt<bool>
    Wide("wide", cl::desc("Ignored for compatibility with GNU objdump"),
         cl::cat(ObjdumpCat));
static cl::alias WideShort("w", cl::Grouping, cl::aliasopt(Wide));

enum DebugVarsFormat {
  DVDisabled,
  DVUnicode,
  DVASCII,
};

static cl::opt<DebugVarsFormat> DbgVariables(
    "debug-vars", cl::init(DVDisabled),
    cl::desc("Print the locations (in registers or memory) of "
             "source-level variables alongside disassembly"),
    cl::ValueOptional,
    cl::values(clEnumValN(DVUnicode, "", "unicode"),
               clEnumValN(DVUnicode, "unicode", "unicode"),
               clEnumValN(DVASCII, "ascii", "unicode")),
    cl::cat(ObjdumpCat));

static cl::opt<int>
    DbgIndent("debug-vars-indent", cl::init(40),
              cl::desc("Distance to indent the source-level variable display, "
                       "relative to the start of the disassembly"),
              cl::cat(ObjdumpCat));

static cl::extrahelp
    HelpResponse("\nPass @FILE as argument to read options from FILE.\n");

static StringSet<> DisasmSymbolSet;
StringSet<> objdump::FoundSectionSet;
static StringRef ToolName;

//const MCRegisterInfo &MRI;

///////////////////////////////ekledim
namespace portedprint{

static bool RISCVInstPrinterValidateMCOperand(const MCOperand &MCOp,
                  const MCSubtargetInfo &STI,
                  unsigned PredicateIndex) {
  switch (PredicateIndex) {
  default:
    llvm_unreachable("Unknown MCOperandPredicate kind");
    break;
  case 1: {

    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isShiftedInt<12, 1>(Imm);
    return MCOp.isBareSymbolRef();
  
    }
  case 2: {

    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isShiftedInt<20, 1>(Imm);
    return MCOp.isBareSymbolRef();
  
    }
  case 3: {

    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isInt<12>(Imm);
    return MCOp.isBareSymbolRef();
  
    }
  }
}


////////////////////////////////////////////////
bool printAliasInstr(const MCInst *MI, uint64_t Address, const MCSubtargetInfo &STI, raw_ostream &OS) {
  static const PatternsForOpcode OpToPatterns[] = {
    {RISCV::ADDI, 0, 2 },
    {RISCV::ADDIW, 2, 1 },
    {RISCV::ANDI, 3, 2 },
    {RISCV::BEQ, 5, 1 },
    {RISCV::BGE, 6, 2 },
    {RISCV::BLT, 8, 2 },
    {RISCV::BNE, 10, 1 },
    {RISCV::CSRRC, 11, 1 },
    {RISCV::CSRRCI, 12, 1 },
    {RISCV::CSRRS, 13, 11 },
    {RISCV::CSRRSI, 24, 1 },
    {RISCV::CSRRW, 25, 7 },
    {RISCV::CSRRWI, 32, 5 },
    {RISCV::FADD_D, 37, 1 },
    {RISCV::FADD_S, 38, 1 },
    {RISCV::FCVT_D_L, 39, 1 },
    {RISCV::FCVT_D_LU, 40, 1 },
    {RISCV::FCVT_LU_D, 41, 1 },
    {RISCV::FCVT_LU_S, 42, 1 },
    {RISCV::FCVT_L_D, 43, 1 },
    {RISCV::FCVT_L_S, 44, 1 },
    {RISCV::FCVT_S_D, 45, 1 },
    {RISCV::FCVT_S_L, 46, 1 },
    {RISCV::FCVT_S_LU, 47, 1 },
    {RISCV::FCVT_S_W, 48, 1 },
    {RISCV::FCVT_S_WU, 49, 1 },
    {RISCV::FCVT_WU_D, 50, 1 },
    {RISCV::FCVT_WU_S, 51, 1 },
    {RISCV::FCVT_W_D, 52, 1 },
    {RISCV::FCVT_W_S, 53, 1 },
    {RISCV::FDIV_D, 54, 1 },
    {RISCV::FDIV_S, 55, 1 },
    {RISCV::FENCE, 56, 1 },
    {RISCV::FMADD_D, 57, 1 },
    {RISCV::FMADD_S, 58, 1 },
    {RISCV::FMSUB_D, 59, 1 },
    {RISCV::FMSUB_S, 60, 1 },
    {RISCV::FMUL_D, 61, 1 },
    {RISCV::FMUL_S, 62, 1 },
    {RISCV::FNMADD_D, 63, 1 },
    {RISCV::FNMADD_S, 64, 1 },
    {RISCV::FNMSUB_D, 65, 1 },
    {RISCV::FNMSUB_S, 66, 1 },
    {RISCV::FSGNJN_D, 67, 1 },
    {RISCV::FSGNJN_S, 68, 1 },
    {RISCV::FSGNJX_D, 69, 1 },
    {RISCV::FSGNJX_S, 70, 1 },
    {RISCV::FSGNJ_D, 71, 1 },
    {RISCV::FSGNJ_S, 72, 1 },
    {RISCV::FSQRT_D, 73, 1 },
    {RISCV::FSQRT_S, 74, 1 },
    {RISCV::FSUB_D, 75, 1 },
    {RISCV::FSUB_S, 76, 1 },
    {RISCV::GORCI, 77, 26 },
    {RISCV::GREVI, 103, 26 },
    {RISCV::JAL, 129, 2 },
    {RISCV::JALR, 131, 6 },
    {RISCV::PACK, 137, 2 },
    {RISCV::PACKW, 139, 1 },
    {RISCV::SFENCE_VMA, 140, 2 },
    {RISCV::SHFLI, 142, 19 },
    {RISCV::SLT, 161, 2 },
    {RISCV::SLTIU, 163, 1 },
    {RISCV::SLTU, 164, 1 },
    {RISCV::SUB, 165, 1 },
    {RISCV::SUBW, 166, 1 },
    {RISCV::UNSHFLI, 167, 19 },
    {RISCV::VMAND_MM, 186, 1 },
    {RISCV::VMNAND_MM, 187, 1 },
    {RISCV::VMXNOR_MM, 188, 1 },
    {RISCV::VMXOR_MM, 189, 1 },
    {RISCV::VWADDU_VX, 190, 1 },
    {RISCV::VWADD_VX, 191, 1 },
    {RISCV::VXOR_VI, 192, 1 },
    {RISCV::XORI, 193, 1 },
  };

  static const AliasPattern Patterns[] = {
    // RISCV::ADDI - 0
    {0, 0, 3, 3 },
    {4, 3, 3, 3 },
    // RISCV::ADDIW - 2
    {14, 6, 3, 4 },
    // RISCV::ANDI - 3
    {28, 10, 3, 5 },
    {28, 15, 3, 5 },
    // RISCV::BEQ - 5
    {42, 20, 3, 3 },
    // RISCV::BGE - 6
    {54, 23, 3, 3 },
    {66, 26, 3, 3 },
    // RISCV::BLT - 8
    {78, 29, 3, 3 },
    {90, 32, 3, 3 },
    // RISCV::BNE - 10
    {102, 35, 3, 3 },
    // RISCV::CSRRC - 11
    {114, 38, 3, 3 },
    // RISCV::CSRRCI - 12
    {128, 41, 3, 2 },
    // RISCV::CSRRS - 13
    {143, 43, 3, 4 },
    {152, 47, 3, 4 },
    {160, 51, 3, 4 },
    {171, 55, 3, 3 },
    {184, 58, 3, 3 },
    {195, 61, 3, 3 },
    {205, 64, 3, 4 },
    {219, 68, 3, 4 },
    {231, 72, 3, 4 },
    {242, 76, 3, 3 },
    {256, 79, 3, 3 },
    // RISCV::CSRRSI - 24
    {270, 82, 3, 2 },
    // RISCV::CSRRW - 25
    {285, 84, 3, 4 },
    {294, 88, 3, 4 },
    {302, 92, 3, 4 },
    {313, 96, 3, 3 },
    {327, 99, 3, 4 },
    {340, 103, 3, 4 },
    {352, 107, 3, 4 },
    // RISCV::CSRRWI - 32
    {367, 111, 3, 3 },
    {376, 114, 3, 3 },
    {388, 117, 3, 2 },
    {403, 119, 3, 3 },
    {416, 122, 3, 3 },
    // RISCV::FADD_D - 37
    {432, 125, 4, 5 },
    // RISCV::FADD_S - 38
    {450, 130, 4, 5 },
    // RISCV::FCVT_D_L - 39
    {468, 135, 3, 5 },
    // RISCV::FCVT_D_LU - 40
    {484, 140, 3, 5 },
    // RISCV::FCVT_LU_D - 41
    {501, 145, 3, 5 },
    // RISCV::FCVT_LU_S - 42
    {518, 150, 3, 5 },
    // RISCV::FCVT_L_D - 43
    {535, 155, 3, 5 },
    // RISCV::FCVT_L_S - 44
    {551, 160, 3, 5 },
    // RISCV::FCVT_S_D - 45
    {567, 165, 3, 4 },
    // RISCV::FCVT_S_L - 46
    {583, 169, 3, 5 },
    // RISCV::FCVT_S_LU - 47
    {599, 174, 3, 5 },
    // RISCV::FCVT_S_W - 48
    {616, 179, 3, 4 },
    // RISCV::FCVT_S_WU - 49
    {632, 183, 3, 4 },
    // RISCV::FCVT_WU_D - 50
    {649, 187, 3, 4 },
    // RISCV::FCVT_WU_S - 51
    {666, 191, 3, 4 },
    // RISCV::FCVT_W_D - 52
    {683, 195, 3, 4 },
    // RISCV::FCVT_W_S - 53
    {699, 199, 3, 4 },
    // RISCV::FDIV_D - 54
    {715, 203, 4, 5 },
    // RISCV::FDIV_S - 55
    {733, 208, 4, 5 },
    // RISCV::FENCE - 56
    {751, 213, 2, 2 },
    // RISCV::FMADD_D - 57
    {757, 215, 5, 6 },
    // RISCV::FMADD_S - 58
    {780, 221, 5, 6 },
    // RISCV::FMSUB_D - 59
    {803, 227, 5, 6 },
    // RISCV::FMSUB_S - 60
    {826, 233, 5, 6 },
    // RISCV::FMUL_D - 61
    {849, 239, 4, 5 },
    // RISCV::FMUL_S - 62
    {867, 244, 4, 5 },
    // RISCV::FNMADD_D - 63
    {885, 249, 5, 6 },
    // RISCV::FNMADD_S - 64
    {909, 255, 5, 6 },
    // RISCV::FNMSUB_D - 65
    {933, 261, 5, 6 },
    // RISCV::FNMSUB_S - 66
    {957, 267, 5, 6 },
    // RISCV::FSGNJN_D - 67
    {981, 273, 3, 4 },
    // RISCV::FSGNJN_S - 68
    {995, 277, 3, 4 },
    // RISCV::FSGNJX_D - 69
    {1009, 281, 3, 4 },
    // RISCV::FSGNJX_S - 70
    {1023, 285, 3, 4 },
    // RISCV::FSGNJ_D - 71
    {1037, 289, 3, 4 },
    // RISCV::FSGNJ_S - 72
    {1050, 293, 3, 4 },
    // RISCV::FSQRT_D - 73
    {1063, 297, 3, 4 },
    // RISCV::FSQRT_S - 74
    {1078, 301, 3, 4 },
    // RISCV::FSUB_D - 75
    {1093, 305, 4, 5 },
    // RISCV::FSUB_S - 76
    {1111, 310, 4, 5 },
    // RISCV::GORCI - 77
    {1129, 315, 3, 6 },
    {1142, 321, 3, 6 },
    {1156, 327, 3, 6 },
    {1169, 333, 3, 6 },
    {1183, 339, 3, 6 },
    {1197, 345, 3, 6 },
    {1210, 351, 3, 6 },
    {1224, 357, 3, 6 },
    {1238, 363, 3, 6 },
    {1252, 369, 3, 6 },
    {1265, 375, 3, 7 },
    {1278, 382, 3, 7 },
    {1290, 389, 3, 7 },
    {1302, 396, 3, 7 },
    {1314, 403, 3, 7 },
    {1325, 410, 3, 7 },
    {1340, 417, 3, 7 },
    {1354, 424, 3, 7 },
    {1368, 431, 3, 7 },
    {1382, 438, 3, 7 },
    {1395, 445, 3, 7 },
    {1265, 452, 3, 7 },
    {1278, 459, 3, 7 },
    {1290, 466, 3, 7 },
    {1302, 473, 3, 7 },
    {1314, 480, 3, 7 },
    // RISCV::GREVI - 103
    {1408, 487, 3, 6 },
    {1421, 493, 3, 6 },
    {1435, 499, 3, 6 },
    {1448, 505, 3, 6 },
    {1462, 511, 3, 6 },
    {1476, 517, 3, 6 },
    {1489, 523, 3, 6 },
    {1503, 529, 3, 6 },
    {1517, 535, 3, 6 },
    {1531, 541, 3, 6 },
    {1544, 547, 3, 7 },
    {1557, 554, 3, 7 },
    {1569, 561, 3, 7 },
    {1581, 568, 3, 7 },
    {1593, 575, 3, 7 },
    {1604, 582, 3, 7 },
    {1619, 589, 3, 7 },
    {1633, 596, 3, 7 },
    {1647, 603, 3, 7 },
    {1661, 610, 3, 7 },
    {1674, 617, 3, 7 },
    {1544, 624, 3, 7 },
    {1557, 631, 3, 7 },
    {1569, 638, 3, 7 },
    {1581, 645, 3, 7 },
    {1593, 652, 3, 7 },
    // RISCV::JAL - 129
    {1687, 659, 2, 2 },
    {1692, 661, 2, 2 },
    // RISCV::JALR - 131
    {1699, 663, 3, 3 },
    {1703, 666, 3, 3 },
    {1709, 669, 3, 3 },
    {1717, 672, 3, 3 },
    {1729, 675, 3, 3 },
    {1739, 678, 3, 3 },
    // RISCV::PACK - 137
    {1751, 681, 3, 5 },
    {1765, 686, 3, 5 },
    // RISCV::PACKW - 139
    {1751, 691, 3, 5 },
    // RISCV::SFENCE_VMA - 140
    {1779, 696, 2, 2 },
    {1790, 698, 2, 2 },
    // RISCV::SHFLI - 142
    {1804, 700, 3, 6 },
    {1817, 706, 3, 6 },
    {1831, 712, 3, 6 },
    {1844, 718, 3, 6 },
    {1858, 724, 3, 6 },
    {1872, 730, 3, 6 },
    {1885, 736, 3, 7 },
    {1897, 743, 3, 7 },
    {1909, 750, 3, 7 },
    {1921, 757, 3, 7 },
    {1932, 764, 3, 7 },
    {1946, 771, 3, 7 },
    {1960, 778, 3, 7 },
    {1974, 785, 3, 7 },
    {1987, 792, 3, 7 },
    {1885, 799, 3, 7 },
    {1897, 806, 3, 7 },
    {1909, 813, 3, 7 },
    {1921, 820, 3, 7 },
    // RISCV::SLT - 161
    {2000, 827, 3, 3 },
    {2012, 830, 3, 3 },
    // RISCV::SLTIU - 163
    {2024, 833, 3, 3 },
    // RISCV::SLTU - 164
    {2036, 836, 3, 3 },
    // RISCV::SUB - 165
    {2048, 839, 3, 3 },
    // RISCV::SUBW - 166
    {2059, 842, 3, 4 },
    // RISCV::UNSHFLI - 167
    {2071, 846, 3, 6 },
    {2086, 852, 3, 6 },
    {2102, 858, 3, 6 },
    {2117, 864, 3, 6 },
    {2133, 870, 3, 6 },
    {2149, 876, 3, 6 },
    {2164, 882, 3, 7 },
    {2178, 889, 3, 7 },
    {2192, 896, 3, 7 },
    {2206, 903, 3, 7 },
    {2219, 910, 3, 7 },
    {2235, 917, 3, 7 },
    {2251, 924, 3, 7 },
    {2267, 931, 3, 7 },
    {2282, 938, 3, 7 },
    {2164, 945, 3, 7 },
    {2178, 952, 3, 7 },
    {2192, 959, 3, 7 },
    {2206, 966, 3, 7 },
    // RISCV::VMAND_MM - 186
    {2297, 973, 3, 4 },
    // RISCV::VMNAND_MM - 187
    {2312, 977, 3, 4 },
    // RISCV::VMXNOR_MM - 188
    {2327, 981, 3, 4 },
    // RISCV::VMXOR_MM - 189
    {2338, 985, 3, 4 },
    // RISCV::VWADDU_VX - 190
    {2349, 989, 4, 5 },
    // RISCV::VWADD_VX - 191
    {2373, 994, 4, 5 },
    // RISCV::VXOR_VI - 192
    {2396, 999, 4, 5 },
    // RISCV::XORI - 193
    {2414, 1004, 3, 3 },
  };

  static const AliasPatternCond Conds[] = {
    // (ADDI X0, X0, 0) - 0
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Imm, uint32_t(0)},
    // (ADDI GPR:$rd, GPR:$rs, 0) - 3
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(0)},
    // (ADDIW GPR:$rd, GPR:$rs, 0) - 6
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(0)},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (ANDI GPR:$rd, GPR:$rs, 255) - 10
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(255)},
    {AliasPatternCond::K_Feature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (ANDI GPR:$rd, GPR:$rs, 255) - 15
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(255)},
    {AliasPatternCond::K_Feature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (BEQ GPR:$rs, X0, simm13_lsb0:$offset) - 20
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Custom, 1},
    // (BGE X0, GPR:$rs, simm13_lsb0:$offset) - 23
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Custom, 1},
    // (BGE GPR:$rs, X0, simm13_lsb0:$offset) - 26
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Custom, 1},
    // (BLT GPR:$rs, X0, simm13_lsb0:$offset) - 29
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Custom, 1},
    // (BLT X0, GPR:$rs, simm13_lsb0:$offset) - 32
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Custom, 1},
    // (BNE GPR:$rs, X0, simm13_lsb0:$offset) - 35
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Custom, 1},
    // (CSRRC X0, csr_sysreg:$csr, GPR:$rs) - 38
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Ignore, 0},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    // (CSRRCI X0, csr_sysreg:$csr, uimm5:$imm) - 41
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Ignore, 0},
    // (CSRRS GPR:$rd, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 }, X0) - 43
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3)},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRS GPR:$rd, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, X0) - 47
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(2)},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRS GPR:$rd, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 }, X0) - 51
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(1)},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRS GPR:$rd, { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, X0) - 55
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3074)},
    {AliasPatternCond::K_Reg, RISCV::X0},
    // (CSRRS GPR:$rd, { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, X0) - 58
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3072)},
    {AliasPatternCond::K_Reg, RISCV::X0},
    // (CSRRS GPR:$rd, { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 }, X0) - 61
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3073)},
    {AliasPatternCond::K_Reg, RISCV::X0},
    // (CSRRS GPR:$rd, { 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0 }, X0) - 64
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3202)},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (CSRRS GPR:$rd, { 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 }, X0) - 68
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3200)},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (CSRRS GPR:$rd, { 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 }, X0) - 72
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3201)},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (CSRRS GPR:$rd, csr_sysreg:$csr, X0) - 76
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Ignore, 0},
    {AliasPatternCond::K_Reg, RISCV::X0},
    // (CSRRS X0, csr_sysreg:$csr, GPR:$rs) - 79
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Ignore, 0},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    // (CSRRSI X0, csr_sysreg:$csr, uimm5:$imm) - 82
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Ignore, 0},
    // (CSRRW X0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 }, GPR:$rs) - 84
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Imm, uint32_t(3)},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRW X0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, GPR:$rs) - 88
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Imm, uint32_t(2)},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRW X0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 }, GPR:$rs) - 92
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Imm, uint32_t(1)},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRW X0, csr_sysreg:$csr, GPR:$rs) - 96
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Ignore, 0},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    // (CSRRW GPR:$rd, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 }, GPR:$rs) - 99
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3)},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRW GPR:$rd, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, GPR:$rs) - 103
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(2)},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRW GPR:$rd, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 }, GPR:$rs) - 107
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(1)},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRWI X0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, uimm5:$imm) - 111
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Imm, uint32_t(2)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRWI X0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 }, uimm5:$imm) - 114
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Imm, uint32_t(1)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRWI X0, csr_sysreg:$csr, uimm5:$imm) - 117
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Ignore, 0},
    // (CSRRWI GPR:$rd, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, uimm5:$imm) - 119
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(2)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (CSRRWI GPR:$rd, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 }, uimm5:$imm) - 122
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(1)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FADD_D FPR64:$rd, FPR64:$rs1, FPR64:$rs2, { 1, 1, 1 }) - 125
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FADD_S FPR32:$rd, FPR32:$rs1, FPR32:$rs2, { 1, 1, 1 }) - 130
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FCVT_D_L FPR64:$rd, GPR:$rs1, { 1, 1, 1 }) - 135
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (FCVT_D_LU FPR64:$rd, GPR:$rs1, { 1, 1, 1 }) - 140
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (FCVT_LU_D GPR:$rd, FPR64:$rs1, { 1, 1, 1 }) - 145
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (FCVT_LU_S GPR:$rd, FPR32:$rs1, { 1, 1, 1 }) - 150
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (FCVT_L_D GPR:$rd, FPR64:$rs1, { 1, 1, 1 }) - 155
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (FCVT_L_S GPR:$rd, FPR32:$rs1, { 1, 1, 1 }) - 160
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (FCVT_S_D FPR32:$rd, FPR64:$rs1, { 1, 1, 1 }) - 165
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FCVT_S_L FPR32:$rd, GPR:$rs1, { 1, 1, 1 }) - 169
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (FCVT_S_LU FPR32:$rd, GPR:$rs1, { 1, 1, 1 }) - 174
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (FCVT_S_W FPR32:$rd, GPR:$rs1, { 1, 1, 1 }) - 179
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FCVT_S_WU FPR32:$rd, GPR:$rs1, { 1, 1, 1 }) - 183
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FCVT_WU_D GPR:$rd, FPR64:$rs1, { 1, 1, 1 }) - 187
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FCVT_WU_S GPR:$rd, FPR32:$rs1, { 1, 1, 1 }) - 191
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FCVT_W_D GPR:$rd, FPR64:$rs1, { 1, 1, 1 }) - 195
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FCVT_W_S GPR:$rd, FPR32:$rs1, { 1, 1, 1 }) - 199
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FDIV_D FPR64:$rd, FPR64:$rs1, FPR64:$rs2, { 1, 1, 1 }) - 203
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FDIV_S FPR32:$rd, FPR32:$rs1, FPR32:$rs2, { 1, 1, 1 }) - 208
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FENCE 15, 15) - 213
    {AliasPatternCond::K_Imm, uint32_t(15)},
    {AliasPatternCond::K_Imm, uint32_t(15)},
    // (FMADD_D FPR64:$rd, FPR64:$rs1, FPR64:$rs2, FPR64:$rs3, { 1, 1, 1 }) - 215
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FMADD_S FPR32:$rd, FPR32:$rs1, FPR32:$rs2, FPR32:$rs3, { 1, 1, 1 }) - 221
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FMSUB_D FPR64:$rd, FPR64:$rs1, FPR64:$rs2, FPR64:$rs3, { 1, 1, 1 }) - 227
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FMSUB_S FPR32:$rd, FPR32:$rs1, FPR32:$rs2, FPR32:$rs3, { 1, 1, 1 }) - 233
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FMUL_D FPR64:$rd, FPR64:$rs1, FPR64:$rs2, { 1, 1, 1 }) - 239
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FMUL_S FPR32:$rd, FPR32:$rs1, FPR32:$rs2, { 1, 1, 1 }) - 244
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FNMADD_D FPR64:$rd, FPR64:$rs1, FPR64:$rs2, FPR64:$rs3, { 1, 1, 1 }) - 249
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FNMADD_S FPR32:$rd, FPR32:$rs1, FPR32:$rs2, FPR32:$rs3, { 1, 1, 1 }) - 255
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FNMSUB_D FPR64:$rd, FPR64:$rs1, FPR64:$rs2, FPR64:$rs3, { 1, 1, 1 }) - 261
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FNMSUB_S FPR32:$rd, FPR32:$rs1, FPR32:$rs2, FPR32:$rs3, { 1, 1, 1 }) - 267
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FSGNJN_D FPR64:$rd, FPR64:$rs, FPR64:$rs) - 273
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_TiedReg, 1},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FSGNJN_S FPR32:$rd, FPR32:$rs, FPR32:$rs) - 277
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_TiedReg, 1},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FSGNJX_D FPR64:$rd, FPR64:$rs, FPR64:$rs) - 281
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_TiedReg, 1},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FSGNJX_S FPR32:$rd, FPR32:$rs, FPR32:$rs) - 285
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_TiedReg, 1},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FSGNJ_D FPR64:$rd, FPR64:$rs, FPR64:$rs) - 289
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_TiedReg, 1},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FSGNJ_S FPR32:$rd, FPR32:$rs, FPR32:$rs) - 293
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_TiedReg, 1},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FSQRT_D FPR64:$rd, FPR64:$rs1, { 1, 1, 1 }) - 297
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FSQRT_S FPR32:$rd, FPR32:$rs1, { 1, 1, 1 }) - 301
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (FSUB_D FPR64:$rd, FPR64:$rs1, FPR64:$rs2, { 1, 1, 1 }) - 305
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR64RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtD},
    // (FSUB_S FPR32:$rd, FPR32:$rs1, FPR32:$rs2, { 1, 1, 1 }) - 310
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_RegClass, RISCV::FPR32RegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtF},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 0, 0, 0, 1 }) - 315
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(1)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 0, 0, 1, 0 }) - 321
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(2)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 0, 0, 1, 1 }) - 327
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 0, 1, 0, 0 }) - 333
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(4)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 0, 1, 1, 0 }) - 339
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(6)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 0, 1, 1, 1 }) - 345
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 1, 0, 0, 0 }) - 351
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(8)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 1, 1, 0, 0 }) - 357
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(12)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 0 }) - 363
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(14)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 1 }) - 369
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(15)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GORCI GPR:$rd, GPR:$rs, { 1, 0, 0, 0, 0 }) - 375
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(16)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 1, 1, 0, 0, 0 }) - 382
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(24)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 1, 1, 1, 0, 0 }) - 389
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(28)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 0 }) - 396
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(30)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 1 }) - 403
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(31)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 1, 0, 0, 0, 0 }) - 410
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(16)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 1, 1, 0, 0, 0 }) - 417
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(24)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 0, 0 }) - 424
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(28)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 1, 0 }) - 431
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(30)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 1, 1 }) - 438
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(31)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 1, 0, 0, 0, 0, 0 }) - 445
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(32)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 1, 1, 0, 0, 0, 0 }) - 452
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(48)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 1, 1, 1, 0, 0, 0 }) - 459
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(56)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 0, 0 }) - 466
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(60)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 1, 0 }) - 473
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(62)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GORCI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 1, 1 }) - 480
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(63)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 0, 0, 0, 1 }) - 487
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(1)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 0, 0, 1, 0 }) - 493
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(2)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 0, 0, 1, 1 }) - 499
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 0, 1, 0, 0 }) - 505
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(4)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 0, 1, 1, 0 }) - 511
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(6)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 0, 1, 1, 1 }) - 517
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 1, 0, 0, 0 }) - 523
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(8)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 1, 1, 0, 0 }) - 529
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(12)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 0 }) - 535
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(14)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 1 }) - 541
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(15)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (GREVI GPR:$rd, GPR:$rs, { 1, 0, 0, 0, 0 }) - 547
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(16)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 1, 1, 0, 0, 0 }) - 554
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(24)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 1, 1, 1, 0, 0 }) - 561
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(28)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 0 }) - 568
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(30)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 1 }) - 575
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(31)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 1, 0, 0, 0, 0 }) - 582
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(16)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 1, 1, 0, 0, 0 }) - 589
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(24)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 0, 0 }) - 596
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(28)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 1, 0 }) - 603
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(30)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 1, 1 }) - 610
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(31)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 1, 0, 0, 0, 0, 0 }) - 617
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(32)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 1, 1, 0, 0, 0, 0 }) - 624
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(48)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 1, 1, 1, 0, 0, 0 }) - 631
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(56)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 0, 0 }) - 638
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(60)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 1, 0 }) - 645
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(62)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (GREVI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 1, 1 }) - 652
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(63)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (JAL X0, simm21_lsb0_jal:$offset) - 659
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Custom, 2},
    // (JAL X1, simm21_lsb0_jal:$offset) - 661
    {AliasPatternCond::K_Reg, RISCV::X1},
    {AliasPatternCond::K_Custom, 2},
    // (JALR X0, X1, 0) - 663
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Reg, RISCV::X1},
    {AliasPatternCond::K_Imm, uint32_t(0)},
    // (JALR X0, GPR:$rs, 0) - 666
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(0)},
    // (JALR X1, GPR:$rs, 0) - 669
    {AliasPatternCond::K_Reg, RISCV::X1},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(0)},
    // (JALR GPR:$rd, GPR:$rs, 0) - 672
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(0)},
    // (JALR X0, GPR:$rs, simm12:$offset) - 675
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Custom, 3},
    // (JALR X1, GPR:$rs, simm12:$offset) - 678
    {AliasPatternCond::K_Reg, RISCV::X1},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Custom, 3},
    // (PACK GPR:$rd, GPR:$rs, X0) - 681
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Feature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (PACK GPR:$rd, GPR:$rs, X0) - 686
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Feature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (PACKW GPR:$rd, GPR:$rs, X0) - 691
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Feature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (SFENCE_VMA X0, X0) - 696
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_Reg, RISCV::X0},
    // (SFENCE_VMA GPR:$rs, X0) - 698
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    // (SHFLI GPR:$rd, GPR:$rs, { 0, 0, 0, 1 }) - 700
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(1)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (SHFLI GPR:$rd, GPR:$rs, { 0, 0, 1, 0 }) - 706
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(2)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (SHFLI GPR:$rd, GPR:$rs, { 0, 0, 1, 1 }) - 712
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (SHFLI GPR:$rd, GPR:$rs, { 0, 1, 0, 0 }) - 718
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(4)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (SHFLI GPR:$rd, GPR:$rs, { 0, 1, 1, 0 }) - 724
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(6)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (SHFLI GPR:$rd, GPR:$rs, { 0, 1, 1, 1 }) - 730
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (SHFLI GPR:$rd, GPR:$rs, { 1, 0, 0, 0 }) - 736
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(8)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 1, 1, 0, 0 }) - 743
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(12)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 1, 1, 1, 0 }) - 750
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(14)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 1, 1, 1, 1 }) - 757
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(15)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 0, 1, 0, 0, 0 }) - 764
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(8)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 0, 1, 1, 0, 0 }) - 771
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(12)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 0 }) - 778
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(14)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 1 }) - 785
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(15)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 1, 0, 0, 0, 0 }) - 792
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(16)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 1, 1, 0, 0, 0 }) - 799
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(24)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 1, 1, 1, 0, 0 }) - 806
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(28)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 0 }) - 813
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(30)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (SHFLI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 1 }) - 820
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(31)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (SLT GPR:$rd, GPR:$rs, X0) - 827
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    // (SLT GPR:$rd, X0, GPR:$rs) - 830
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    // (SLTIU GPR:$rd, GPR:$rs, 1) - 833
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(1)},
    // (SLTU GPR:$rd, X0, GPR:$rs) - 836
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    // (SUB GPR:$rd, X0, GPR:$rs) - 839
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    // (SUBW GPR:$rd, X0, GPR:$rs) - 842
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 0, 0, 0, 1 }) - 846
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(1)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 0, 0, 1, 0 }) - 852
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(2)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 0, 0, 1, 1 }) - 858
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(3)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 0, 1, 0, 0 }) - 864
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(4)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 0, 1, 1, 0 }) - 870
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(6)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 0, 1, 1, 1 }) - 876
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(7)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 1, 0, 0, 0 }) - 882
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(8)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 1, 1, 0, 0 }) - 889
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(12)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 1, 1, 1, 0 }) - 896
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(14)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 1, 1, 1, 1 }) - 903
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(15)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_NegFeature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 0, 1, 0, 0, 0 }) - 910
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(8)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 0, 1, 1, 0, 0 }) - 917
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(12)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 0 }) - 924
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(14)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 0, 1, 1, 1, 1 }) - 931
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(15)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 1, 0, 0, 0, 0 }) - 938
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(16)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 1, 1, 0, 0, 0 }) - 945
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(24)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 1, 1, 1, 0, 0 }) - 952
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(28)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 0 }) - 959
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(30)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (UNSHFLI GPR:$rd, GPR:$rs, { 1, 1, 1, 1, 1 }) - 966
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(31)},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbb},
    {AliasPatternCond::K_OrFeature, RISCV::FeatureExtZbp},
    {AliasPatternCond::K_EndOrFeatures, 0},
    {AliasPatternCond::K_Feature, RISCV::Feature64Bit},
    // (VMAND_MM VRegOp:$vd, VRegOp:$vs, VRegOp:$vs) - 973
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_TiedReg, 1},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtV},
    // (VMNAND_MM VRegOp:$vd, VRegOp:$vs, VRegOp:$vs) - 977
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_TiedReg, 1},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtV},
    // (VMXNOR_MM VRegOp:$vd, VRegOp:$vd, VRegOp:$vd) - 981
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_TiedReg, 0},
    {AliasPatternCond::K_TiedReg, 0},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtV},
    // (VMXOR_MM VRegOp:$vd, VRegOp:$vd, VRegOp:$vd) - 985
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_TiedReg, 0},
    {AliasPatternCond::K_TiedReg, 0},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtV},
    // (VWADDU_VX VRegOp:$vd, VRegOp:$vs, X0, VMaskOp:$vm) - 989
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_RegClass, RISCV::VMV0RegClassID},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtV},
    // (VWADD_VX VRegOp:$vd, VRegOp:$vs, X0, VMaskOp:$vm) - 994
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_Reg, RISCV::X0},
    {AliasPatternCond::K_RegClass, RISCV::VMV0RegClassID},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtV},
    // (VXOR_VI VRegOp:$vd, VRegOp:$vs, -1, VMaskOp:$vm) - 999
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::VRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(-1)},
    {AliasPatternCond::K_RegClass, RISCV::VMV0RegClassID},
    {AliasPatternCond::K_Feature, RISCV::FeatureStdExtV},
    // (XORI GPR:$rd, GPR:$rs, -1) - 1004
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_RegClass, RISCV::GPRRegClassID},
    {AliasPatternCond::K_Imm, uint32_t(-1)},
  };

  static const char AsmStrings[] =
    /* 0 */ "nop\0"
    /* 4 */ "mv $\x01, $\x02\0"
    /* 14 */ "sext.w $\x01, $\x02\0"
    /* 28 */ "zext.b $\x01, $\x02\0"
    /* 42 */ "beqz $\x01, $\x03\0"
    /* 54 */ "blez $\x02, $\x03\0"
    /* 66 */ "bgez $\x01, $\x03\0"
    /* 78 */ "bltz $\x01, $\x03\0"
    /* 90 */ "bgtz $\x02, $\x03\0"
    /* 102 */ "bnez $\x01, $\x03\0"
    /* 114 */ "csrc $\xFF\x02\x01, $\x03\0"
    /* 128 */ "csrci $\xFF\x02\x01, $\x03\0"
    /* 143 */ "frcsr $\x01\0"
    /* 152 */ "frrm $\x01\0"
    /* 160 */ "frflags $\x01\0"
    /* 171 */ "rdinstret $\x01\0"
    /* 184 */ "rdcycle $\x01\0"
    /* 195 */ "rdtime $\x01\0"
    /* 205 */ "rdinstreth $\x01\0"
    /* 219 */ "rdcycleh $\x01\0"
    /* 231 */ "rdtimeh $\x01\0"
    /* 242 */ "csrr $\x01, $\xFF\x02\x01\0"
    /* 256 */ "csrs $\xFF\x02\x01, $\x03\0"
    /* 270 */ "csrsi $\xFF\x02\x01, $\x03\0"
    /* 285 */ "fscsr $\x03\0"
    /* 294 */ "fsrm $\x03\0"
    /* 302 */ "fsflags $\x03\0"
    /* 313 */ "csrw $\xFF\x02\x01, $\x03\0"
    /* 327 */ "fscsr $\x01, $\x03\0"
    /* 340 */ "fsrm $\x01, $\x03\0"
    /* 352 */ "fsflags $\x01, $\x03\0"
    /* 367 */ "fsrmi $\x03\0"
    /* 376 */ "fsflagsi $\x03\0"
    /* 388 */ "csrwi $\xFF\x02\x01, $\x03\0"
    /* 403 */ "fsrmi $\x01, $\x03\0"
    /* 416 */ "fsflagsi $\x01, $\x03\0"
    /* 432 */ "fadd.d $\x01, $\x02, $\x03\0"
    /* 450 */ "fadd.s $\x01, $\x02, $\x03\0"
    /* 468 */ "fcvt.d.l $\x01, $\x02\0"
    /* 484 */ "fcvt.d.lu $\x01, $\x02\0"
    /* 501 */ "fcvt.lu.d $\x01, $\x02\0"
    /* 518 */ "fcvt.lu.s $\x01, $\x02\0"
    /* 535 */ "fcvt.l.d $\x01, $\x02\0"
    /* 551 */ "fcvt.l.s $\x01, $\x02\0"
    /* 567 */ "fcvt.s.d $\x01, $\x02\0"
    /* 583 */ "fcvt.s.l $\x01, $\x02\0"
    /* 599 */ "fcvt.s.lu $\x01, $\x02\0"
    /* 616 */ "fcvt.s.w $\x01, $\x02\0"
    /* 632 */ "fcvt.s.wu $\x01, $\x02\0"
    /* 649 */ "fcvt.wu.d $\x01, $\x02\0"
    /* 666 */ "fcvt.wu.s $\x01, $\x02\0"
    /* 683 */ "fcvt.w.d $\x01, $\x02\0"
    /* 699 */ "fcvt.w.s $\x01, $\x02\0"
    /* 715 */ "fdiv.d $\x01, $\x02, $\x03\0"
    /* 733 */ "fdiv.s $\x01, $\x02, $\x03\0"
    /* 751 */ "fence\0"
    /* 757 */ "fmadd.d $\x01, $\x02, $\x03, $\x04\0"
    /* 780 */ "fmadd.s $\x01, $\x02, $\x03, $\x04\0"
    /* 803 */ "fmsub.d $\x01, $\x02, $\x03, $\x04\0"
    /* 826 */ "fmsub.s $\x01, $\x02, $\x03, $\x04\0"
    /* 849 */ "fmul.d $\x01, $\x02, $\x03\0"
    /* 867 */ "fmul.s $\x01, $\x02, $\x03\0"
    /* 885 */ "fnmadd.d $\x01, $\x02, $\x03, $\x04\0"
    /* 909 */ "fnmadd.s $\x01, $\x02, $\x03, $\x04\0"
    /* 933 */ "fnmsub.d $\x01, $\x02, $\x03, $\x04\0"
    /* 957 */ "fnmsub.s $\x01, $\x02, $\x03, $\x04\0"
    /* 981 */ "fneg.d $\x01, $\x02\0"
    /* 995 */ "fneg.s $\x01, $\x02\0"
    /* 1009 */ "fabs.d $\x01, $\x02\0"
    /* 1023 */ "fabs.s $\x01, $\x02\0"
    /* 1037 */ "fmv.d $\x01, $\x02\0"
    /* 1050 */ "fmv.s $\x01, $\x02\0"
    /* 1063 */ "fsqrt.d $\x01, $\x02\0"
    /* 1078 */ "fsqrt.s $\x01, $\x02\0"
    /* 1093 */ "fsub.d $\x01, $\x02, $\x03\0"
    /* 1111 */ "fsub.s $\x01, $\x02, $\x03\0"
    /* 1129 */ "orc.p $\x01, $\x02\0"
    /* 1142 */ "orc2.n $\x01, $\x02\0"
    /* 1156 */ "orc.n $\x01, $\x02\0"
    /* 1169 */ "orc4.b $\x01, $\x02\0"
    /* 1183 */ "orc2.b $\x01, $\x02\0"
    /* 1197 */ "orc.b $\x01, $\x02\0"
    /* 1210 */ "orc8.h $\x01, $\x02\0"
    /* 1224 */ "orc4.h $\x01, $\x02\0"
    /* 1238 */ "orc2.h $\x01, $\x02\0"
    /* 1252 */ "orc.h $\x01, $\x02\0"
    /* 1265 */ "orc16 $\x01, $\x02\0"
    /* 1278 */ "orc8 $\x01, $\x02\0"
    /* 1290 */ "orc4 $\x01, $\x02\0"
    /* 1302 */ "orc2 $\x01, $\x02\0"
    /* 1314 */ "orc $\x01, $\x02\0"
    /* 1325 */ "orc16.w $\x01, $\x02\0"
    /* 1340 */ "orc8.w $\x01, $\x02\0"
    /* 1354 */ "orc4.w $\x01, $\x02\0"
    /* 1368 */ "orc2.w $\x01, $\x02\0"
    /* 1382 */ "orc.w $\x01, $\x02\0"
    /* 1395 */ "orc32 $\x01, $\x02\0"
    /* 1408 */ "rev.p $\x01, $\x02\0"
    /* 1421 */ "rev2.n $\x01, $\x02\0"
    /* 1435 */ "rev.n $\x01, $\x02\0"
    /* 1448 */ "rev4.b $\x01, $\x02\0"
    /* 1462 */ "rev2.b $\x01, $\x02\0"
    /* 1476 */ "rev.b $\x01, $\x02\0"
    /* 1489 */ "rev8.h $\x01, $\x02\0"
    /* 1503 */ "rev4.h $\x01, $\x02\0"
    /* 1517 */ "rev2.h $\x01, $\x02\0"
    /* 1531 */ "rev.h $\x01, $\x02\0"
    /* 1544 */ "rev16 $\x01, $\x02\0"
    /* 1557 */ "rev8 $\x01, $\x02\0"
    /* 1569 */ "rev4 $\x01, $\x02\0"
    /* 1581 */ "rev2 $\x01, $\x02\0"
    /* 1593 */ "rev $\x01, $\x02\0"
    /* 1604 */ "rev16.w $\x01, $\x02\0"
    /* 1619 */ "rev8.w $\x01, $\x02\0"
    /* 1633 */ "rev4.w $\x01, $\x02\0"
    /* 1647 */ "rev2.w $\x01, $\x02\0"
    /* 1661 */ "rev.w $\x01, $\x02\0"
    /* 1674 */ "rev32 $\x01, $\x02\0"
    /* 1687 */ "j $\x02\0"
    /* 1692 */ "jal $\x02\0"
    /* 1699 */ "ret\0"
    /* 1703 */ "jr $\x02\0"
    /* 1709 */ "jalr $\x02\0"
    /* 1717 */ "jalr $\x01, $\x02\0"
    /* 1729 */ "jr $\x03($\x02)\0"
    /* 1739 */ "jalr $\x03($\x02)\0"
    /* 1751 */ "zext.h $\x01, $\x02\0"
    /* 1765 */ "zext.w $\x01, $\x02\0"
    /* 1779 */ "sfence.vma\0"
    /* 1790 */ "sfence.vma $\x01\0"
    /* 1804 */ "zip.n $\x01, $\x02\0"
    /* 1817 */ "zip2.b $\x01, $\x02\0"
    /* 1831 */ "zip.b $\x01, $\x02\0"
    /* 1844 */ "zip4.h $\x01, $\x02\0"
    /* 1858 */ "zip2.h $\x01, $\x02\0"
    /* 1872 */ "zip.h $\x01, $\x02\0"
    /* 1885 */ "zip8 $\x01, $\x02\0"
    /* 1897 */ "zip4 $\x01, $\x02\0"
    /* 1909 */ "zip2 $\x01, $\x02\0"
    /* 1921 */ "zip $\x01, $\x02\0"
    /* 1932 */ "zip8.w $\x01, $\x02\0"
    /* 1946 */ "zip4.w $\x01, $\x02\0"
    /* 1960 */ "zip2.w $\x01, $\x02\0"
    /* 1974 */ "zip.w $\x01, $\x02\0"
    /* 1987 */ "zip16 $\x01, $\x02\0"
    /* 2000 */ "sltz $\x01, $\x02\0"
    /* 2012 */ "sgtz $\x01, $\x03\0"
    /* 2024 */ "seqz $\x01, $\x02\0"
    /* 2036 */ "snez $\x01, $\x03\0"
    /* 2048 */ "neg $\x01, $\x03\0"
    /* 2059 */ "negw $\x01, $\x03\0"
    /* 2071 */ "unzip.n $\x01, $\x02\0"
    /* 2086 */ "unzip2.b $\x01, $\x02\0"
    /* 2102 */ "unzip.b $\x01, $\x02\0"
    /* 2117 */ "unzip4.h $\x01, $\x02\0"
    /* 2133 */ "unzip2.h $\x01, $\x02\0"
    /* 2149 */ "unzip.h $\x01, $\x02\0"
    /* 2164 */ "unzip8 $\x01, $\x02\0"
    /* 2178 */ "unzip4 $\x01, $\x02\0"
    /* 2192 */ "unzip2 $\x01, $\x02\0"
    /* 2206 */ "unzip $\x01, $\x02\0"
    /* 2219 */ "unzip8.w $\x01, $\x02\0"
    /* 2235 */ "unzip4.w $\x01, $\x02\0"
    /* 2251 */ "unzip2.w $\x01, $\x02\0"
    /* 2267 */ "unzip.w $\x01, $\x02\0"
    /* 2282 */ "unzip16 $\x01, $\x02\0"
    /* 2297 */ "vmcpy.m $\x01, $\x02\0"
    /* 2312 */ "vmnot.m $\x01, $\x02\0"
    /* 2327 */ "vmset.m $\x01\0"
    /* 2338 */ "vmclr.m $\x01\0"
    /* 2349 */ "vwcvtu.x.x.v $\x01, $\x02$\xFF\x04\x02\0"
    /* 2373 */ "vwcvt.x.x.v $\x01, $\x02$\xFF\x04\x02\0"
    /* 2396 */ "vnot.v $\x01, $\x02$\xFF\x04\x02\0"
    /* 2414 */ "not $\x01, $\x02\0"
  ;

#ifndef NDEBUG
  static struct SortCheck {
    SortCheck(ArrayRef<PatternsForOpcode> OpToPatterns) {
      assert(std::is_sorted(
                 OpToPatterns.begin(), OpToPatterns.end(),
                 [](const PatternsForOpcode &L, const PatternsForOpcode &R) {
                   return L.Opcode < R.Opcode;
                 }) &&
             "tablegen failed to sort opcode patterns");
    }
  } sortCheckVar(OpToPatterns);
#endif

  AliasMatchingData M {
    makeArrayRef(OpToPatterns),
    makeArrayRef(Patterns),
    makeArrayRef(Conds),
    StringRef(AsmStrings, array_lengthof(AsmStrings)),
    &RISCVInstPrinterValidateMCOperand,
  };
  //const char *AsmString = llvm::MCInstPrinter::matchAliasPatterns(MI, &STI, M);
  //if (!AsmString) return false;
  //
  //unsigned I = 0;
  //while (AsmString[I] != ' ' && AsmString[I] != '\t' &&
  //       AsmString[I] != '$' && AsmString[I] != '\0')
  //  ++I;
  //OS << '\t' << StringRef(AsmString, I);
  //if (AsmString[I] != '\0') {
  //  if (AsmString[I] == ' ' || AsmString[I] == '\t') {
  //    OS << '\t';
  //    ++I;
  //  }
  //  do {
  //    if (AsmString[I] == '$') {
  //      ++I;
  //      if (AsmString[I] == (char)0xff) {
  //        ++I;
  //        int OpIdx = AsmString[I++] - 1;
  //        int PrintMethodIdx = AsmString[I++] - 1;
  //        //printCustomAliasOperand(MI, Address, OpIdx, PrintMethodIdx, STI, OS);
  //      } //else
  //        //printOperand(MI, unsigned(AsmString[I++]) - 1, STI, OS);
  //    } else {
  //      OS << AsmString[I++];
  //    }
  //  } while (AsmString[I] != '\0');
  //}

  return true;
}


//////////////////////////////////////
static bool RISCVValidateMCOperand(const MCOperand &MCOp,
                  const MCSubtargetInfo &STI,
                  unsigned PredicateIndex) {
  switch (PredicateIndex) {
  default:
    llvm_unreachable("Unknown MCOperandPredicate kind");
    break;
  case 1: {
  // uimm10_lsb00nonzero
  
    int64_t Imm;
    if (!MCOp.evaluateAsConstantImm(Imm))
      return false;
    return isShiftedUInt<8, 2>(Imm) && (Imm != 0);
  
  }
  case 2: {
  // simm6nonzero
  
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return (Imm != 0) && isInt<6>(Imm);
    return MCOp.isBareSymbolRef();
  
  }
  case 3: {
  // simm6
  
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isInt<6>(Imm);
    return MCOp.isBareSymbolRef();
  
  }
  case 4: {
  // simm10_lsb0000nonzero
  
    int64_t Imm;
    if (!MCOp.evaluateAsConstantImm(Imm))
      return false;
    return isShiftedInt<6, 4>(Imm) && (Imm != 0);
  
  }
  case 5: {
  // simm9_lsb0
  
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isShiftedInt<8, 1>(Imm);
    return MCOp.isBareSymbolRef();

  
  }
  case 6: {
  // uimm8_lsb000
  
    int64_t Imm;
    if (!MCOp.evaluateAsConstantImm(Imm))
      return false;
    return isShiftedUInt<5, 3>(Imm);
  
  }
  case 7: {
  // uimm9_lsb000
  
    int64_t Imm;
    if (!MCOp.evaluateAsConstantImm(Imm))
      return false;
    return isShiftedUInt<6, 3>(Imm);
  
  }
  case 8: {
  // uimm7_lsb00
  
    int64_t Imm;
    if (!MCOp.evaluateAsConstantImm(Imm))
      return false;
    return isShiftedUInt<5, 2>(Imm);
  
  }
  case 9: {
  // uimm8_lsb00
  
    int64_t Imm;
    if (!MCOp.evaluateAsConstantImm(Imm))
      return false;
    return isShiftedUInt<6, 2>(Imm);
  
  }
  case 10: {
  // simm12_lsb0
  
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isShiftedInt<11, 1>(Imm);
    return MCOp.isBareSymbolRef();
  
  }
  case 11: {
  // c_lui_imm
  
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return (Imm != 0) && (isUInt<5>(Imm) ||
             (Imm >= 0xfffe0 && Imm <= 0xfffff));
    return MCOp.isBareSymbolRef();
  
  }
  case 12: {
  // uimmlog2xlennonzero
  
    int64_t Imm;
    if (!MCOp.evaluateAsConstantImm(Imm))
      return false;
    if (STI.getTargetTriple().isArch64Bit())
      return  isUInt<6>(Imm) && (Imm != 0);
    return isUInt<5>(Imm) && (Imm != 0);
  
  }
  }
}

///////////////////////////////////////
static bool uncompressInst(MCInst& OutInst,
                           const MCInst &MI,
                           const MCRegisterInfo &MRI,
                           const MCSubtargetInfo &STI) {
  switch (MI.getOpcode()) {
    default: return false;
    case RISCV::C_ADD: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg()))) {
      // add	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::ADD);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg()))) {
      // add	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::ADD);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(2));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_ADD
    case RISCV::C_ADDI: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // addi	$rd, $rs1, $imm12
      OutInst.setOpcode(RISCV::ADDI);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_ADDI
    case RISCV::C_ADDI16SP: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MI.getOperand(0).getReg() == RISCV::X2) &&
      (MI.getOperand(1).getReg() == RISCV::X2) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // addi	$rd, $rs1, $imm12
      OutInst.setOpcode(RISCV::ADDI);
      // Operand: rd
      OutInst.addOperand(MCOperand::createReg(RISCV::X2));
      // Operand: rs1
      OutInst.addOperand(MCOperand::createReg(RISCV::X2));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_ADDI16SP
    case RISCV::C_ADDI4SPN: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // addi	$rd, $rs1, $imm12
      OutInst.setOpcode(RISCV::ADDI);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_ADDI4SPN
    case RISCV::C_ADDIW: {
    if (STI.getFeatureBits()[RISCV::Feature64Bit] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // addiw	$rd, $rs1, $imm12
      OutInst.setOpcode(RISCV::ADDIW);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_ADDIW
    case RISCV::C_ADDW: {
    if (STI.getFeatureBits()[RISCV::Feature64Bit] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg()))) {
      // addw	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::ADDW);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
    if (STI.getFeatureBits()[RISCV::Feature64Bit] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg()))) {
      // addw	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::ADDW);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(2));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_ADDW
    case RISCV::C_AND: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg()))) {
      // and	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::AND);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg()))) {
      // and	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::AND);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(2));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_AND
    case RISCV::C_ANDI: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // andi	$rd, $rs1, $imm12
      OutInst.setOpcode(RISCV::ANDI);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_ANDI
    case RISCV::C_BEQZ: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(1), STI, 2)) {
      // beq	$rs1, $rs2, $imm12
      OutInst.setOpcode(RISCV::BEQ);
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs2
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(1));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_BEQZ
    case RISCV::C_BNEZ: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(1), STI, 2)) {
      // bne	$rs1, $rs2, $imm12
      OutInst.setOpcode(RISCV::BNE);
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs2
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(1));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_BNEZ
    case RISCV::C_EBREAK: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC]) {
      // ebreak	
      OutInst.setOpcode(RISCV::EBREAK);
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_EBREAK
    case RISCV::C_FLD: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtD] &&
      (MRI.getRegClass(RISCV::FPR64RegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // fld	$rd, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::FLD);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_FLD
    case RISCV::C_FLDSP: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtD] &&
      (MRI.getRegClass(RISCV::FPR64RegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // fld	$rd, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::FLD);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_FLDSP
    case RISCV::C_FLW: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtF] &&
      !STI.getFeatureBits()[RISCV::Feature64Bit] &&
      (MRI.getRegClass(RISCV::FPR32RegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // flw	$rd, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::FLW);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_FLW
    case RISCV::C_FLWSP: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtF] &&
      !STI.getFeatureBits()[RISCV::Feature64Bit] &&
      (MRI.getRegClass(RISCV::FPR32RegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // flw	$rd, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::FLW);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_FLWSP
    case RISCV::C_FSD: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtD] &&
      (MRI.getRegClass(RISCV::FPR64RegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // fsd	$rs2, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::FSD);
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_FSD
    case RISCV::C_FSDSP: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtD] &&
      (MRI.getRegClass(RISCV::FPR64RegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // fsd	$rs2, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::FSD);
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_FSDSP
    case RISCV::C_FSW: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtF] &&
      !STI.getFeatureBits()[RISCV::Feature64Bit] &&
      (MRI.getRegClass(RISCV::FPR32RegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // fsw	$rs2, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::FSW);
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_FSW
    case RISCV::C_FSWSP: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtF] &&
      !STI.getFeatureBits()[RISCV::Feature64Bit] &&
      (MRI.getRegClass(RISCV::FPR32RegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // fsw	$rs2, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::FSW);
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_FSWSP
    case RISCV::C_J: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      RISCVValidateMCOperand(MI.getOperand(0), STI, 3)) {
      // jal	$rd, $imm20
      OutInst.setOpcode(RISCV::JAL);
      // Operand: rd
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      // Operand: imm20
      OutInst.addOperand(MI.getOperand(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_J
    case RISCV::C_JAL: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      !STI.getFeatureBits()[RISCV::Feature64Bit] &&
      RISCVValidateMCOperand(MI.getOperand(0), STI, 3)) {
      // jal	$rd, $imm20
      OutInst.setOpcode(RISCV::JAL);
      // Operand: rd
      OutInst.addOperand(MCOperand::createReg(RISCV::X1));
      // Operand: imm20
      OutInst.addOperand(MI.getOperand(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_JAL
    case RISCV::C_JALR: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MCOperand::createImm(0), STI, 1)) {
      // jalr	$rd, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::JALR);
      // Operand: rd
      OutInst.addOperand(MCOperand::createReg(RISCV::X1));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: imm12
      OutInst.addOperand(MCOperand::createImm(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_JALR
    case RISCV::C_JR: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MCOperand::createImm(0), STI, 1)) {
      // jalr	$rd, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::JALR);
      // Operand: rd
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: imm12
      OutInst.addOperand(MCOperand::createImm(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_JR
    case RISCV::C_LD: {
    if (STI.getFeatureBits()[RISCV::Feature64Bit] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // ld	$rd, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::LD);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_LD
    case RISCV::C_LDSP: {
    if (STI.getFeatureBits()[RISCV::Feature64Bit] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // ld	$rd, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::LD);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_LDSP
    case RISCV::C_LI: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(1), STI, 1)) {
      // addi	$rd, $rs1, $imm12
      OutInst.setOpcode(RISCV::ADDI);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(1));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
    if (STI.getFeatureBits()[RISCV::Feature64Bit] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(1), STI, 1)) {
      // addiw	$rd, $rs1, $imm12
      OutInst.setOpcode(RISCV::ADDIW);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(1));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_LI
    case RISCV::C_LUI: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(1), STI, 4)) {
      // lui	$rd, $imm20
      OutInst.setOpcode(RISCV::LUI);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: imm20
      OutInst.addOperand(MI.getOperand(1));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_LUI
    case RISCV::C_LW: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // lw	$rd, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::LW);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_LW
    case RISCV::C_LWSP: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // lw	$rd, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::LW);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_LWSP
    case RISCV::C_MV: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg()))) {
      // add	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::ADD);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(1));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg()))) {
      // add	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::ADD);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: rs2
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MCOperand::createImm(0), STI, 1)) {
      // addi	$rd, $rs1, $imm12
      OutInst.setOpcode(RISCV::ADDI);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MCOperand::createImm(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_MV
    case RISCV::C_NEG: {
    if (STI.getFeatureBits()[RISCV::FeatureExtZbproposedc] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg()))) {
      // sub	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::SUB);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_NEG
    case RISCV::C_NOP: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      RISCVValidateMCOperand(MCOperand::createImm(0), STI, 1)) {
      // addi	$rd, $rs1, $imm12
      OutInst.setOpcode(RISCV::ADDI);
      // Operand: rd
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      // Operand: rs1
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      // Operand: imm12
      OutInst.addOperand(MCOperand::createImm(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_NOP
    case RISCV::C_NOT: {
    if (STI.getFeatureBits()[RISCV::FeatureExtZbproposedc] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MCOperand::createImm(-1), STI, 1)) {
      // xori	$rd, $rs1, $imm12
      OutInst.setOpcode(RISCV::XORI);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: imm12
      OutInst.addOperand(MCOperand::createImm(-1));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_NOT
    case RISCV::C_OR: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg()))) {
      // or	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::OR);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg()))) {
      // or	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::OR);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(2));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_OR
    case RISCV::C_SD: {
    if (STI.getFeatureBits()[RISCV::Feature64Bit] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // sd	$rs2, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::SD);
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_SD
    case RISCV::C_SDSP: {
    if (STI.getFeatureBits()[RISCV::Feature64Bit] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // sd	$rs2, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::SD);
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_SDSP
    case RISCV::C_SLLI: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 5)) {
      // slli	$rd, $rs1, $shamt
      OutInst.setOpcode(RISCV::SLLI);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: shamt
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_SLLI
    case RISCV::C_SRAI: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 5)) {
      // srai	$rd, $rs1, $shamt
      OutInst.setOpcode(RISCV::SRAI);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: shamt
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_SRAI
    case RISCV::C_SRLI: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 5)) {
      // srli	$rd, $rs1, $shamt
      OutInst.setOpcode(RISCV::SRLI);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: shamt
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_SRLI
    case RISCV::C_SUB: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg()))) {
      // sub	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::SUB);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_SUB
    case RISCV::C_SUBW: {
    if (STI.getFeatureBits()[RISCV::Feature64Bit] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg()))) {
      // subw	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::SUBW);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_SUBW
    case RISCV::C_SW: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // sw	$rs2, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::SW);
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_SW
    case RISCV::C_SWSP: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(1).getReg())) &&
      RISCVValidateMCOperand(MI.getOperand(2), STI, 1)) {
      // sw	$rs2, ${imm12}(${rs1})
      OutInst.setOpcode(RISCV::SW);
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(1));
      // Operand: imm12
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_SWSP
    case RISCV::C_UNIMP: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC]) {
      // unimp	
      OutInst.setOpcode(RISCV::UNIMP);
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_UNIMP
    case RISCV::C_XOR: {
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg()))) {
      // xor	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::XOR);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(2));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
    if (STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(2).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg()))) {
      // xor	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::XOR);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(2));
      // Operand: rs2
      OutInst.addOperand(MI.getOperand(0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if
      break;
    } // case C_XOR
    case RISCV::C_ZEXTW: {
    if (STI.getFeatureBits()[RISCV::Feature64Bit] &&
      STI.getFeatureBits()[RISCV::FeatureExtZbproposedc] &&
      STI.getFeatureBits()[RISCV::FeatureStdExtC] &&
      (STI.getFeatureBits()[RISCV::FeatureExtZbb] || STI.getFeatureBits()[RISCV::FeatureExtZbp]) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg())) &&
      (MRI.getRegClass(RISCV::GPRRegClassID).contains(MI.getOperand(0).getReg()))) {
      // pack	$rd, $rs1, $rs2
      OutInst.setOpcode(RISCV::PACK);
      // Operand: rd
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs1
      OutInst.addOperand(MI.getOperand(0));
      // Operand: rs2
      OutInst.addOperand(MCOperand::createReg(RISCV::X0));
      OutInst.setLoc(MI.getLoc());
      return true;
    } // if

    } // case C_ZEXTW
  } // switch
  return false;
}



//static bool RISCVValidateMachineOperand(const MachineOperand &MO,
//                  const RISCVSubtarget *Subtarget,
//                  unsigned PredicateIndex) {
//  int64_t Imm = MO.getImm(); 
//  switch (PredicateIndex) {
//  default:
//    llvm_unreachable("Unknown ImmLeaf Predicate kind");
//    break;
//  case 1: {
//  // simm6nonzero
//  return (Imm != 0) && isInt<6>(Imm);
//  }
//  case 2: {
//  // simm10_lsb0000nonzero
//  return (Imm != 0) && isShiftedInt<6, 4>(Imm);
//  }
//  case 3: {
//  // uimm10_lsb00nonzero
//  return isShiftedUInt<8, 2>(Imm) && (Imm != 0);
//  }
//  case 4: {
//  // simm6
//  return isInt<6>(Imm);
//  }
//  case 5: {
//  // simm9_lsb0
//  return isShiftedInt<8, 1>(Imm);
//  }
//  case 6: {
//  // uimm8_lsb000
//  return isShiftedUInt<5, 3>(Imm);
//  }
//  case 7: {
//  // uimm9_lsb000
//  return isShiftedUInt<6, 3>(Imm);
//  }
//  case 8: {
//  // uimm7_lsb00
//  return isShiftedUInt<5, 2>(Imm);
//  }
//  case 9: {
//  // uimm8_lsb00
//  return isShiftedUInt<6, 2>(Imm);
//  }
//  case 10: {
//  // simm12_lsb0
//  return isShiftedInt<11, 1>(Imm);
//  }
//  case 11: {
//  // c_lui_imm
//  return (Imm != 0) &&
//                                 (isUInt<5>(Imm) ||
//                                  (Imm >= 0xfffe0 && Imm <= 0xfffff));
//  }
//  case 12: {
//  // uimmlog2xlennonzero
//  
//  if (Subtarget->is64Bit())
//    return isUInt<6>(Imm) && (Imm != 0);
//  return isUInt<5>(Imm) && (Imm != 0);
//
//  }
//  }
//}

/////////////////////////////////
//void RISCVInstPrinter::printInstruction(const MCInst *MI, uint64_t Address, const MCSubtargetInfo &STI, raw_ostream &O) {
StringRef printInstruction(const MCInst *MI, uint64_t Address, const MCSubtargetInfo &STI, raw_ostream &O) {

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverlength-strings"
#endif
  static const char AsmStrs[] = {
  /* 0 */ "c.srai64\t\0"
  /* 10 */ "c.slli64\t\0"
  /* 20 */ "c.srli64\t\0"
  /* 30 */ "lla\t\0"
  /* 35 */ "sfence.vma\t\0"
  /* 47 */ "sra\t\0"
  /* 52 */ "crc32.b\t\0"
  /* 61 */ "crc32c.b\t\0"
  /* 71 */ "sext.b\t\0"
  /* 79 */ "lb\t\0"
  /* 83 */ "sb\t\0"
  /* 87 */ "c.sub\t\0"
  /* 94 */ "auipc\t\0"
  /* 101 */ "gorc\t\0"
  /* 107 */ "csrrc\t\0"
  /* 114 */ "crc32.d\t\0"
  /* 123 */ "fsub.d\t\0"
  /* 131 */ "fmsub.d\t\0"
  /* 140 */ "fnmsub.d\t\0"
  /* 150 */ "crc32c.d\t\0"
  /* 160 */ "sc.d\t\0"
  /* 166 */ "fadd.d\t\0"
  /* 174 */ "fmadd.d\t\0"
  /* 183 */ "fnmadd.d\t\0"
  /* 193 */ "amoadd.d\t\0"
  /* 203 */ "amoand.d\t\0"
  /* 213 */ "fle.d\t\0"
  /* 220 */ "fsgnj.d\t\0"
  /* 229 */ "fcvt.l.d\t\0"
  /* 239 */ "fmul.d\t\0"
  /* 247 */ "fmin.d\t\0"
  /* 255 */ "amomin.d\t\0"
  /* 265 */ "fsgnjn.d\t\0"
  /* 275 */ "amoswap.d\t\0"
  /* 286 */ "feq.d\t\0"
  /* 293 */ "lr.d\t\0"
  /* 299 */ "amoor.d\t\0"
  /* 308 */ "amoxor.d\t\0"
  /* 318 */ "fcvt.s.d\t\0"
  /* 328 */ "fclass.d\t\0"
  /* 338 */ "flt.d\t\0"
  /* 345 */ "fsqrt.d\t\0"
  /* 354 */ "fcvt.lu.d\t\0"
  /* 365 */ "amominu.d\t\0"
  /* 376 */ "fcvt.wu.d\t\0"
  /* 387 */ "amomaxu.d\t\0"
  /* 398 */ "fdiv.d\t\0"
  /* 406 */ "fcvt.w.d\t\0"
  /* 416 */ "fmv.x.d\t\0"
  /* 425 */ "fmax.d\t\0"
  /* 433 */ "amomax.d\t\0"
  /* 443 */ "fsgnjx.d\t\0"
  /* 453 */ "c.add\t\0"
  /* 460 */ "la.tls.gd\t\0"
  /* 471 */ "c.ld\t\0"
  /* 477 */ "c.fld\t\0"
  /* 484 */ "c.and\t\0"
  /* 491 */ "c.sd\t\0"
  /* 497 */ "c.fsd\t\0"
  /* 504 */ "fence\t\0"
  /* 511 */ "bge\t\0"
  /* 516 */ "la.tls.ie\t\0"
  /* 527 */ "bne\t\0"
  /* 532 */ "vfmv.s.f\t\0"
  /* 542 */ "vfmv.v.f\t\0"
  /* 552 */ "vfsub.vf\t\0"
  /* 562 */ "vfmsub.vf\t\0"
  /* 573 */ "vfnmsub.vf\t\0"
  /* 585 */ "vfrsub.vf\t\0"
  /* 596 */ "vfwsub.vf\t\0"
  /* 607 */ "vfmsac.vf\t\0"
  /* 618 */ "vfnmsac.vf\t\0"
  /* 630 */ "vfwnmsac.vf\t\0"
  /* 643 */ "vfwmsac.vf\t\0"
  /* 655 */ "vfmacc.vf\t\0"
  /* 666 */ "vfnmacc.vf\t\0"
  /* 678 */ "vfwnmacc.vf\t\0"
  /* 691 */ "vfwmacc.vf\t\0"
  /* 703 */ "vfadd.vf\t\0"
  /* 713 */ "vfmadd.vf\t\0"
  /* 724 */ "vfnmadd.vf\t\0"
  /* 736 */ "vfwadd.vf\t\0"
  /* 747 */ "vmfge.vf\t\0"
  /* 757 */ "vmfle.vf\t\0"
  /* 767 */ "vmfne.vf\t\0"
  /* 777 */ "vfsgnj.vf\t\0"
  /* 788 */ "vfmul.vf\t\0"
  /* 798 */ "vfwmul.vf\t\0"
  /* 809 */ "vfmin.vf\t\0"
  /* 819 */ "vfsgnjn.vf\t\0"
  /* 831 */ "vmfeq.vf\t\0"
  /* 841 */ "vmfgt.vf\t\0"
  /* 851 */ "vmflt.vf\t\0"
  /* 861 */ "vfdiv.vf\t\0"
  /* 871 */ "vfrdiv.vf\t\0"
  /* 882 */ "vfmax.vf\t\0"
  /* 892 */ "vfsgnjx.vf\t\0"
  /* 904 */ "vfwsub.wf\t\0"
  /* 915 */ "vfwadd.wf\t\0"
  /* 926 */ "c.neg\t\0"
  /* 933 */ "crc32.h\t\0"
  /* 942 */ "crc32c.h\t\0"
  /* 952 */ "sext.h\t\0"
  /* 960 */ "packh\t\0"
  /* 967 */ "clmulh\t\0"
  /* 975 */ "sh\t\0"
  /* 979 */ "fence.i\t\0"
  /* 988 */ "vmv.v.i\t\0"
  /* 997 */ "c.srai\t\0"
  /* 1005 */ "gorci\t\0"
  /* 1012 */ "csrrci\t\0"
  /* 1020 */ "c.addi\t\0"
  /* 1028 */ "c.andi\t\0"
  /* 1036 */ "wfi\t\0"
  /* 1041 */ "c.li\t\0"
  /* 1047 */ "unshfli\t\0"
  /* 1056 */ "c.slli\t\0"
  /* 1064 */ "c.srli\t\0"
  /* 1072 */ "vsetvli\t\0"
  /* 1081 */ "sloi\t\0"
  /* 1087 */ "sroi\t\0"
  /* 1093 */ "sbclri\t\0"
  /* 1101 */ "rori\t\0"
  /* 1107 */ "xori\t\0"
  /* 1113 */ "fsri\t\0"
  /* 1119 */ "csrrsi\t\0"
  /* 1127 */ "sbseti\t\0"
  /* 1135 */ "slti\t\0"
  /* 1141 */ "sbexti\t\0"
  /* 1149 */ "c.lui\t\0"
  /* 1156 */ "vssra.vi\t\0"
  /* 1166 */ "vsra.vi\t\0"
  /* 1175 */ "vrsub.vi\t\0"
  /* 1185 */ "vmadc.vi\t\0"
  /* 1195 */ "vsadd.vi\t\0"
  /* 1205 */ "vadd.vi\t\0"
  /* 1214 */ "vand.vi\t\0"
  /* 1223 */ "vmsle.vi\t\0"
  /* 1233 */ "vmsne.vi\t\0"
  /* 1243 */ "vsll.vi\t\0"
  /* 1252 */ "vssrl.vi\t\0"
  /* 1262 */ "vsrl.vi\t\0"
  /* 1271 */ "vslidedown.vi\t\0"
  /* 1286 */ "vslideup.vi\t\0"
  /* 1299 */ "vmseq.vi\t\0"
  /* 1309 */ "vrgather.vi\t\0"
  /* 1322 */ "vor.vi\t\0"
  /* 1330 */ "vxor.vi\t\0"
  /* 1339 */ "vmsgt.vi\t\0"
  /* 1349 */ "vsaddu.vi\t\0"
  /* 1360 */ "vmsleu.vi\t\0"
  /* 1371 */ "vmsgtu.vi\t\0"
  /* 1382 */ "grevi\t\0"
  /* 1389 */ "sbinvi\t\0"
  /* 1397 */ "vnsra.wi\t\0"
  /* 1407 */ "vnsrl.wi\t\0"
  /* 1417 */ "vnclip.wi\t\0"
  /* 1428 */ "vnclipu.wi\t\0"
  /* 1440 */ "csrrwi\t\0"
  /* 1448 */ "c.j\t\0"
  /* 1453 */ "c.ebreak\t\0"
  /* 1463 */ "pack\t\0"
  /* 1469 */ "fcvt.d.l\t\0"
  /* 1479 */ "fcvt.s.l\t\0"
  /* 1489 */ "c.jal\t\0"
  /* 1496 */ "unshfl\t\0"
  /* 1504 */ "tail\t\0"
  /* 1510 */ "ecall\t\0"
  /* 1517 */ "sll\t\0"
  /* 1522 */ "rol\t\0"
  /* 1527 */ "sc.d.rl\t\0"
  /* 1536 */ "amoadd.d.rl\t\0"
  /* 1549 */ "amoand.d.rl\t\0"
  /* 1562 */ "amomin.d.rl\t\0"
  /* 1575 */ "amoswap.d.rl\t\0"
  /* 1589 */ "lr.d.rl\t\0"
  /* 1598 */ "amoor.d.rl\t\0"
  /* 1610 */ "amoxor.d.rl\t\0"
  /* 1623 */ "amominu.d.rl\t\0"
  /* 1637 */ "amomaxu.d.rl\t\0"
  /* 1651 */ "amomax.d.rl\t\0"
  /* 1664 */ "sc.w.rl\t\0"
  /* 1673 */ "amoadd.w.rl\t\0"
  /* 1686 */ "amoand.w.rl\t\0"
  /* 1699 */ "amomin.w.rl\t\0"
  /* 1712 */ "amoswap.w.rl\t\0"
  /* 1726 */ "lr.w.rl\t\0"
  /* 1735 */ "amoor.w.rl\t\0"
  /* 1747 */ "amoxor.w.rl\t\0"
  /* 1760 */ "amominu.w.rl\t\0"
  /* 1774 */ "amomaxu.w.rl\t\0"
  /* 1788 */ "amomax.w.rl\t\0"
  /* 1801 */ "sc.d.aqrl\t\0"
  /* 1812 */ "amoadd.d.aqrl\t\0"
  /* 1827 */ "amoand.d.aqrl\t\0"
  /* 1842 */ "amomin.d.aqrl\t\0"
  /* 1857 */ "amoswap.d.aqrl\t\0"
  /* 1873 */ "lr.d.aqrl\t\0"
  /* 1884 */ "amoor.d.aqrl\t\0"
  /* 1898 */ "amoxor.d.aqrl\t\0"
  /* 1913 */ "amominu.d.aqrl\t\0"
  /* 1929 */ "amomaxu.d.aqrl\t\0"
  /* 1945 */ "amomax.d.aqrl\t\0"
  /* 1960 */ "sc.w.aqrl\t\0"
  /* 1971 */ "amoadd.w.aqrl\t\0"
  /* 1986 */ "amoand.w.aqrl\t\0"
  /* 2001 */ "amomin.w.aqrl\t\0"
  /* 2016 */ "amoswap.w.aqrl\t\0"
  /* 2032 */ "lr.w.aqrl\t\0"
  /* 2043 */ "amoor.w.aqrl\t\0"
  /* 2057 */ "amoxor.w.aqrl\t\0"
  /* 2072 */ "amominu.w.aqrl\t\0"
  /* 2088 */ "amomaxu.w.aqrl\t\0"
  /* 2104 */ "amomax.w.aqrl\t\0"
  /* 2119 */ "srl\t\0"
  /* 2124 */ "fsl\t\0"
  /* 2129 */ "clmul\t\0"
  /* 2136 */ "vsetvl\t\0"
  /* 2144 */ "viota.m\t\0"
  /* 2153 */ "vpopc.m\t\0"
  /* 2162 */ "vmsbf.m\t\0"
  /* 2171 */ "vmsif.m\t\0"
  /* 2180 */ "vmsof.m\t\0"
  /* 2189 */ "vfirst.m\t\0"
  /* 2199 */ "rem\t\0"
  /* 2204 */ "vfmerge.vfm\t\0"
  /* 2217 */ "vmadc.vim\t\0"
  /* 2228 */ "vadc.vim\t\0"
  /* 2238 */ "vmerge.vim\t\0"
  /* 2250 */ "vmand.mm\t\0"
  /* 2260 */ "vmnand.mm\t\0"
  /* 2271 */ "vmor.mm\t\0"
  /* 2280 */ "vmnor.mm\t\0"
  /* 2290 */ "vmxnor.mm\t\0"
  /* 2301 */ "vmxor.mm\t\0"
  /* 2311 */ "vmandnot.mm\t\0"
  /* 2324 */ "vmornot.mm\t\0"
  /* 2336 */ "vcompress.vm\t\0"
  /* 2350 */ "vmsbc.vvm\t\0"
  /* 2361 */ "vsbc.vvm\t\0"
  /* 2371 */ "vmadc.vvm\t\0"
  /* 2382 */ "vadc.vvm\t\0"
  /* 2392 */ "vmerge.vvm\t\0"
  /* 2404 */ "vmsbc.vxm\t\0"
  /* 2415 */ "vsbc.vxm\t\0"
  /* 2425 */ "vmadc.vxm\t\0"
  /* 2436 */ "vadc.vxm\t\0"
  /* 2446 */ "vmerge.vxm\t\0"
  /* 2458 */ "andn\t\0"
  /* 2464 */ "min\t\0"
  /* 2469 */ "c.addi4spn\t\0"
  /* 2481 */ "orn\t\0"
  /* 2486 */ "slo\t\0"
  /* 2491 */ "sro\t\0"
  /* 2496 */ "fence.tso\t\0"
  /* 2507 */ "bdep\t\0"
  /* 2513 */ "bfp\t\0"
  /* 2518 */ "bmatflip\t\0"
  /* 2528 */ "c.unimp\t\0"
  /* 2537 */ "jump\t\0"
  /* 2543 */ "c.nop\t\0"
  /* 2550 */ "c.addi16sp\t\0"
  /* 2562 */ "c.ldsp\t\0"
  /* 2570 */ "c.fldsp\t\0"
  /* 2579 */ "c.sdsp\t\0"
  /* 2587 */ "c.fsdsp\t\0"
  /* 2596 */ "c.lwsp\t\0"
  /* 2604 */ "c.flwsp\t\0"
  /* 2613 */ "c.swsp\t\0"
  /* 2621 */ "c.fswsp\t\0"
  /* 2630 */ "sc.d.aq\t\0"
  /* 2639 */ "amoadd.d.aq\t\0"
  /* 2652 */ "amoand.d.aq\t\0"
  /* 2665 */ "amomin.d.aq\t\0"
  /* 2678 */ "amoswap.d.aq\t\0"
  /* 2692 */ "lr.d.aq\t\0"
  /* 2701 */ "amoor.d.aq\t\0"
  /* 2713 */ "amoxor.d.aq\t\0"
  /* 2726 */ "amominu.d.aq\t\0"
  /* 2740 */ "amomaxu.d.aq\t\0"
  /* 2754 */ "amomax.d.aq\t\0"
  /* 2767 */ "sc.w.aq\t\0"
  /* 2776 */ "amoadd.w.aq\t\0"
  /* 2789 */ "amoand.w.aq\t\0"
  /* 2802 */ "amomin.w.aq\t\0"
  /* 2815 */ "amoswap.w.aq\t\0"
  /* 2829 */ "lr.w.aq\t\0"
  /* 2838 */ "amoor.w.aq\t\0"
  /* 2850 */ "amoxor.w.aq\t\0"
  /* 2863 */ "amominu.w.aq\t\0"
  /* 2877 */ "amomaxu.w.aq\t\0"
  /* 2891 */ "amomax.w.aq\t\0"
  /* 2904 */ "beq\t\0"
  /* 2909 */ "c.jr\t\0"
  /* 2915 */ "c.jalr\t\0"
  /* 2923 */ "sbclr\t\0"
  /* 2930 */ "clmulr\t\0"
  /* 2938 */ "c.or\t\0"
  /* 2944 */ "xnor\t\0"
  /* 2950 */ "ror\t\0"
  /* 2955 */ "bmator\t\0"
  /* 2963 */ "c.xor\t\0"
  /* 2970 */ "bmatxor\t\0"
  /* 2979 */ "fsr\t\0"
  /* 2984 */ "fsub.s\t\0"
  /* 2992 */ "fmsub.s\t\0"
  /* 3001 */ "fnmsub.s\t\0"
  /* 3011 */ "fcvt.d.s\t\0"
  /* 3021 */ "fadd.s\t\0"
  /* 3029 */ "fmadd.s\t\0"
  /* 3038 */ "fnmadd.s\t\0"
  /* 3048 */ "fle.s\t\0"
  /* 3055 */ "vfmv.f.s\t\0"
  /* 3065 */ "fsgnj.s\t\0"
  /* 3074 */ "fcvt.l.s\t\0"
  /* 3084 */ "fmul.s\t\0"
  /* 3092 */ "fmin.s\t\0"
  /* 3100 */ "fsgnjn.s\t\0"
  /* 3110 */ "feq.s\t\0"
  /* 3117 */ "fclass.s\t\0"
  /* 3127 */ "flt.s\t\0"
  /* 3134 */ "fsqrt.s\t\0"
  /* 3143 */ "fcvt.lu.s\t\0"
  /* 3154 */ "fcvt.wu.s\t\0"
  /* 3165 */ "fdiv.s\t\0"
  /* 3173 */ "fcvt.w.s\t\0"
  /* 3183 */ "vmv.x.s\t\0"
  /* 3192 */ "fmax.s\t\0"
  /* 3200 */ "fsgnjx.s\t\0"
  /* 3210 */ "csrrs\t\0"
  /* 3217 */ "vredand.vs\t\0"
  /* 3229 */ "vfredsum.vs\t\0"
  /* 3242 */ "vredsum.vs\t\0"
  /* 3254 */ "vfwredsum.vs\t\0"
  /* 3268 */ "vwredsum.vs\t\0"
  /* 3281 */ "vfredosum.vs\t\0"
  /* 3295 */ "vfwredosum.vs\t\0"
  /* 3310 */ "vfredmin.vs\t\0"
  /* 3323 */ "vredmin.vs\t\0"
  /* 3335 */ "vredor.vs\t\0"
  /* 3346 */ "vredxor.vs\t\0"
  /* 3358 */ "vwredsumu.vs\t\0"
  /* 3372 */ "vredminu.vs\t\0"
  /* 3385 */ "vredmaxu.vs\t\0"
  /* 3398 */ "vfredmax.vs\t\0"
  /* 3411 */ "vredmax.vs\t\0"
  /* 3423 */ "dret\t\0"
  /* 3429 */ "mret\t\0"
  /* 3435 */ "sret\t\0"
  /* 3441 */ "uret\t\0"
  /* 3447 */ "sbset\t\0"
  /* 3454 */ "blt\t\0"
  /* 3459 */ "slt\t\0"
  /* 3464 */ "pcnt\t\0"
  /* 3470 */ "c.not\t\0"
  /* 3477 */ "sbext\t\0"
  /* 3484 */ "lbu\t\0"
  /* 3489 */ "bgeu\t\0"
  /* 3495 */ "mulhu\t\0"
  /* 3502 */ "sltiu\t\0"
  /* 3509 */ "packu\t\0"
  /* 3516 */ "fcvt.d.lu\t\0"
  /* 3527 */ "fcvt.s.lu\t\0"
  /* 3538 */ "remu\t\0"
  /* 3544 */ "minu\t\0"
  /* 3550 */ "mulhsu\t\0"
  /* 3558 */ "bltu\t\0"
  /* 3564 */ "sltu\t\0"
  /* 3570 */ "divu\t\0"
  /* 3576 */ "fcvt.d.wu\t\0"
  /* 3587 */ "fcvt.s.wu\t\0"
  /* 3598 */ "subwu\t\0"
  /* 3605 */ "addwu\t\0"
  /* 3612 */ "addiwu\t\0"
  /* 3620 */ "lwu\t\0"
  /* 3625 */ "maxu\t\0"
  /* 3631 */ "vlb.v\t\0"
  /* 3638 */ "vlsb.v\t\0"
  /* 3646 */ "vssb.v\t\0"
  /* 3654 */ "vsb.v\t\0"
  /* 3661 */ "vlxb.v\t\0"
  /* 3669 */ "vsxb.v\t\0"
  /* 3677 */ "vsuxb.v\t\0"
  /* 3686 */ "vid.v\t\0"
  /* 3693 */ "vle.v\t\0"
  /* 3700 */ "vlse.v\t\0"
  /* 3708 */ "vsse.v\t\0"
  /* 3716 */ "vse.v\t\0"
  /* 3723 */ "vlxe.v\t\0"
  /* 3731 */ "vsxe.v\t\0"
  /* 3739 */ "vsuxe.v\t\0"
  /* 3748 */ "vfwcvt.f.f.v\t\0"
  /* 3762 */ "vfcvt.xu.f.v\t\0"
  /* 3776 */ "vfwcvt.xu.f.v\t\0"
  /* 3791 */ "vfcvt.x.f.v\t\0"
  /* 3804 */ "vfwcvt.x.f.v\t\0"
  /* 3818 */ "vlbff.v\t\0"
  /* 3827 */ "vleff.v\t\0"
  /* 3836 */ "vlhff.v\t\0"
  /* 3845 */ "vlbuff.v\t\0"
  /* 3855 */ "vlhuff.v\t\0"
  /* 3865 */ "vlwuff.v\t\0"
  /* 3875 */ "vlwff.v\t\0"
  /* 3884 */ "vlh.v\t\0"
  /* 3891 */ "vlsh.v\t\0"
  /* 3899 */ "vssh.v\t\0"
  /* 3907 */ "vsh.v\t\0"
  /* 3914 */ "vlxh.v\t\0"
  /* 3922 */ "vsxh.v\t\0"
  /* 3930 */ "vsuxh.v\t\0"
  /* 3939 */ "vl1r.v\t\0"
  /* 3947 */ "vs1r.v\t\0"
  /* 3955 */ "vmv1r.v\t\0"
  /* 3964 */ "vmv2r.v\t\0"
  /* 3973 */ "vmv4r.v\t\0"
  /* 3982 */ "vmv8r.v\t\0"
  /* 3991 */ "vfclass.v\t\0"
  /* 4002 */ "vfsqrt.v\t\0"
  /* 4012 */ "vlbu.v\t\0"
  /* 4020 */ "vlsbu.v\t\0"
  /* 4029 */ "vlxbu.v\t\0"
  /* 4038 */ "vlhu.v\t\0"
  /* 4046 */ "vlshu.v\t\0"
  /* 4055 */ "vlxhu.v\t\0"
  /* 4064 */ "vlwu.v\t\0"
  /* 4072 */ "vlswu.v\t\0"
  /* 4081 */ "vlxwu.v\t\0"
  /* 4090 */ "vfcvt.f.xu.v\t\0"
  /* 4104 */ "vfwcvt.f.xu.v\t\0"
  /* 4119 */ "vmv.v.v\t\0"
  /* 4128 */ "vlw.v\t\0"
  /* 4135 */ "vlsw.v\t\0"
  /* 4143 */ "vssw.v\t\0"
  /* 4151 */ "vsw.v\t\0"
  /* 4158 */ "vlxw.v\t\0"
  /* 4166 */ "vsxw.v\t\0"
  /* 4174 */ "vsuxw.v\t\0"
  /* 4183 */ "vfcvt.f.x.v\t\0"
  /* 4196 */ "vfwcvt.f.x.v\t\0"
  /* 4210 */ "grev\t\0"
  /* 4216 */ "div\t\0"
  /* 4221 */ "c.mv\t\0"
  /* 4227 */ "sbinv\t\0"
  /* 4234 */ "cmov\t\0"
  /* 4240 */ "vssra.vv\t\0"
  /* 4250 */ "vsra.vv\t\0"
  /* 4259 */ "vasub.vv\t\0"
  /* 4269 */ "vfsub.vv\t\0"
  /* 4279 */ "vfmsub.vv\t\0"
  /* 4290 */ "vfnmsub.vv\t\0"
  /* 4302 */ "vnmsub.vv\t\0"
  /* 4313 */ "vssub.vv\t\0"
  /* 4323 */ "vsub.vv\t\0"
  /* 4332 */ "vfwsub.vv\t\0"
  /* 4343 */ "vwsub.vv\t\0"
  /* 4353 */ "vfmsac.vv\t\0"
  /* 4364 */ "vfnmsac.vv\t\0"
  /* 4376 */ "vnmsac.vv\t\0"
  /* 4387 */ "vfwnmsac.vv\t\0"
  /* 4400 */ "vfwmsac.vv\t\0"
  /* 4412 */ "vmsbc.vv\t\0"
  /* 4422 */ "vfmacc.vv\t\0"
  /* 4433 */ "vfnmacc.vv\t\0"
  /* 4445 */ "vfwnmacc.vv\t\0"
  /* 4458 */ "vmacc.vv\t\0"
  /* 4468 */ "vfwmacc.vv\t\0"
  /* 4480 */ "vwmacc.vv\t\0"
  /* 4491 */ "vmadc.vv\t\0"
  /* 4501 */ "vaadd.vv\t\0"
  /* 4511 */ "vfadd.vv\t\0"
  /* 4521 */ "vfmadd.vv\t\0"
  /* 4532 */ "vfnmadd.vv\t\0"
  /* 4544 */ "vmadd.vv\t\0"
  /* 4554 */ "vsadd.vv\t\0"
  /* 4564 */ "vadd.vv\t\0"
  /* 4573 */ "vfwadd.vv\t\0"
  /* 4584 */ "vwadd.vv\t\0"
  /* 4594 */ "vand.vv\t\0"
  /* 4603 */ "vmfle.vv\t\0"
  /* 4613 */ "vmsle.vv\t\0"
  /* 4623 */ "vmfne.vv\t\0"
  /* 4633 */ "vmsne.vv\t\0"
  /* 4643 */ "vmulh.vv\t\0"
  /* 4653 */ "vfsgnj.vv\t\0"
  /* 4664 */ "vsll.vv\t\0"
  /* 4673 */ "vssrl.vv\t\0"
  /* 4683 */ "vsrl.vv\t\0"
  /* 4692 */ "vfmul.vv\t\0"
  /* 4702 */ "vsmul.vv\t\0"
  /* 4712 */ "vmul.vv\t\0"
  /* 4721 */ "vfwmul.vv\t\0"
  /* 4732 */ "vwmul.vv\t\0"
  /* 4742 */ "vrem.vv\t\0"
  /* 4751 */ "vfmin.vv\t\0"
  /* 4761 */ "vmin.vv\t\0"
  /* 4770 */ "vfsgnjn.vv\t\0"
  /* 4782 */ "vmfeq.vv\t\0"
  /* 4792 */ "vmseq.vv\t\0"
  /* 4802 */ "vrgather.vv\t\0"
  /* 4815 */ "vor.vv\t\0"
  /* 4823 */ "vxor.vv\t\0"
  /* 4832 */ "vmflt.vv\t\0"
  /* 4842 */ "vmslt.vv\t\0"
  /* 4852 */ "vasubu.vv\t\0"
  /* 4863 */ "vssubu.vv\t\0"
  /* 4874 */ "vwsubu.vv\t\0"
  /* 4885 */ "vwmaccu.vv\t\0"
  /* 4897 */ "vaaddu.vv\t\0"
  /* 4908 */ "vsaddu.vv\t\0"
  /* 4919 */ "vwaddu.vv\t\0"
  /* 4930 */ "vmsleu.vv\t\0"
  /* 4941 */ "vmulhu.vv\t\0"
  /* 4952 */ "vwmulu.vv\t\0"
  /* 4963 */ "vremu.vv\t\0"
  /* 4973 */ "vminu.vv\t\0"
  /* 4983 */ "vwmaccsu.vv\t\0"
  /* 4996 */ "vmulhsu.vv\t\0"
  /* 5008 */ "vwmulsu.vv\t\0"
  /* 5020 */ "vmsltu.vv\t\0"
  /* 5031 */ "vdivu.vv\t\0"
  /* 5041 */ "vmaxu.vv\t\0"
  /* 5051 */ "vfdiv.vv\t\0"
  /* 5061 */ "vdiv.vv\t\0"
  /* 5070 */ "vfmax.vv\t\0"
  /* 5080 */ "vmax.vv\t\0"
  /* 5089 */ "vfsgnjx.vv\t\0"
  /* 5101 */ "vnsra.wv\t\0"
  /* 5111 */ "vfwsub.wv\t\0"
  /* 5122 */ "vwsub.wv\t\0"
  /* 5132 */ "vfwadd.wv\t\0"
  /* 5143 */ "vwadd.wv\t\0"
  /* 5153 */ "vnsrl.wv\t\0"
  /* 5163 */ "vnclip.wv\t\0"
  /* 5174 */ "vwsubu.wv\t\0"
  /* 5185 */ "vwaddu.wv\t\0"
  /* 5196 */ "vnclipu.wv\t\0"
  /* 5208 */ "crc32.w\t\0"
  /* 5217 */ "crc32c.w\t\0"
  /* 5227 */ "sc.w\t\0"
  /* 5233 */ "fcvt.d.w\t\0"
  /* 5243 */ "amoadd.w\t\0"
  /* 5253 */ "amoand.w\t\0"
  /* 5263 */ "vfncvt.rod.f.f.w\t\0"
  /* 5281 */ "vfncvt.f.f.w\t\0"
  /* 5295 */ "vfncvt.xu.f.w\t\0"
  /* 5310 */ "vfncvt.x.f.w\t\0"
  /* 5324 */ "amomin.w\t\0"
  /* 5334 */ "amoswap.w\t\0"
  /* 5345 */ "lr.w\t\0"
  /* 5351 */ "amoor.w\t\0"
  /* 5360 */ "amoxor.w\t\0"
  /* 5370 */ "fcvt.s.w\t\0"
  /* 5380 */ "c.zext.w\t\0"
  /* 5390 */ "subu.w\t\0"
  /* 5398 */ "addu.w\t\0"
  /* 5406 */ "slliu.w\t\0"
  /* 5415 */ "amominu.w\t\0"
  /* 5426 */ "vfncvt.f.xu.w\t\0"
  /* 5441 */ "amomaxu.w\t\0"
  /* 5452 */ "vfncvt.f.x.w\t\0"
  /* 5466 */ "fmv.x.w\t\0"
  /* 5475 */ "amomax.w\t\0"
  /* 5485 */ "sraw\t\0"
  /* 5491 */ "c.subw\t\0"
  /* 5499 */ "gorcw\t\0"
  /* 5506 */ "c.addw\t\0"
  /* 5514 */ "clmulhw\t\0"
  /* 5523 */ "sraiw\t\0"
  /* 5530 */ "gorciw\t\0"
  /* 5538 */ "c.addiw\t\0"
  /* 5547 */ "slliw\t\0"
  /* 5554 */ "srliw\t\0"
  /* 5561 */ "sloiw\t\0"
  /* 5568 */ "sroiw\t\0"
  /* 5575 */ "sbclriw\t\0"
  /* 5584 */ "roriw\t\0"
  /* 5591 */ "fsriw\t\0"
  /* 5598 */ "sbsetiw\t\0"
  /* 5607 */ "greviw\t\0"
  /* 5615 */ "sbinviw\t\0"
  /* 5624 */ "packw\t\0"
  /* 5631 */ "c.lw\t\0"
  /* 5637 */ "c.flw\t\0"
  /* 5644 */ "unshflw\t\0"
  /* 5653 */ "sllw\t\0"
  /* 5659 */ "rolw\t\0"
  /* 5665 */ "srlw\t\0"
  /* 5671 */ "fslw\t\0"
  /* 5677 */ "clmulw\t\0"
  /* 5685 */ "remw\t\0"
  /* 5691 */ "slow\t\0"
  /* 5697 */ "srow\t\0"
  /* 5703 */ "bdepw\t\0"
  /* 5710 */ "bfpw\t\0"
  /* 5716 */ "sbclrw\t\0"
  /* 5724 */ "clmulrw\t\0"
  /* 5733 */ "rorw\t\0"
  /* 5739 */ "csrrw\t\0"
  /* 5746 */ "fsrw\t\0"
  /* 5752 */ "c.sw\t\0"
  /* 5758 */ "c.fsw\t\0"
  /* 5765 */ "sbsetw\t\0"
  /* 5773 */ "pcntw\t\0"
  /* 5780 */ "sbextw\t\0"
  /* 5788 */ "packuw\t\0"
  /* 5796 */ "remuw\t\0"
  /* 5803 */ "divuw\t\0"
  /* 5810 */ "grevw\t\0"
  /* 5817 */ "divw\t\0"
  /* 5823 */ "sbinvw\t\0"
  /* 5831 */ "clzw\t\0"
  /* 5837 */ "ctzw\t\0"
  /* 5843 */ "fmv.d.x\t\0"
  /* 5852 */ "vmv.s.x\t\0"
  /* 5861 */ "vmv.v.x\t\0"
  /* 5870 */ "fmv.w.x\t\0"
  /* 5879 */ "max\t\0"
  /* 5884 */ "cmix\t\0"
  /* 5890 */ "vssra.vx\t\0"
  /* 5900 */ "vsra.vx\t\0"
  /* 5909 */ "vasub.vx\t\0"
  /* 5919 */ "vnmsub.vx\t\0"
  /* 5930 */ "vrsub.vx\t\0"
  /* 5940 */ "vssub.vx\t\0"
  /* 5950 */ "vsub.vx\t\0"
  /* 5959 */ "vwsub.vx\t\0"
  /* 5969 */ "vnmsac.vx\t\0"
  /* 5980 */ "vmsbc.vx\t\0"
  /* 5990 */ "vmacc.vx\t\0"
  /* 6000 */ "vwmacc.vx\t\0"
  /* 6011 */ "vmadc.vx\t\0"
  /* 6021 */ "vaadd.vx\t\0"
  /* 6031 */ "vmadd.vx\t\0"
  /* 6041 */ "vsadd.vx\t\0"
  /* 6051 */ "vadd.vx\t\0"
  /* 6060 */ "vwadd.vx\t\0"
  /* 6070 */ "vand.vx\t\0"
  /* 6079 */ "vmsle.vx\t\0"
  /* 6089 */ "vmsne.vx\t\0"
  /* 6099 */ "vmulh.vx\t\0"
  /* 6109 */ "vsll.vx\t\0"
  /* 6118 */ "vssrl.vx\t\0"
  /* 6128 */ "vsrl.vx\t\0"
  /* 6137 */ "vsmul.vx\t\0"
  /* 6147 */ "vmul.vx\t\0"
  /* 6156 */ "vwmul.vx\t\0"
  /* 6166 */ "vrem.vx\t\0"
  /* 6175 */ "vmin.vx\t\0"
  /* 6184 */ "vslide1down.vx\t\0"
  /* 6200 */ "vslidedown.vx\t\0"
  /* 6215 */ "vslide1up.vx\t\0"
  /* 6229 */ "vslideup.vx\t\0"
  /* 6242 */ "vmseq.vx\t\0"
  /* 6252 */ "vrgather.vx\t\0"
  /* 6265 */ "vor.vx\t\0"
  /* 6273 */ "vxor.vx\t\0"
  /* 6282 */ "vwmaccus.vx\t\0"
  /* 6295 */ "vmsgt.vx\t\0"
  /* 6305 */ "vmslt.vx\t\0"
  /* 6315 */ "vasubu.vx\t\0"
  /* 6326 */ "vssubu.vx\t\0"
  /* 6337 */ "vwsubu.vx\t\0"
  /* 6348 */ "vwmaccu.vx\t\0"
  /* 6360 */ "vaaddu.vx\t\0"
  /* 6371 */ "vsaddu.vx\t\0"
  /* 6382 */ "vwaddu.vx\t\0"
  /* 6393 */ "vmsleu.vx\t\0"
  /* 6404 */ "vmulhu.vx\t\0"
  /* 6415 */ "vwmulu.vx\t\0"
  /* 6426 */ "vremu.vx\t\0"
  /* 6436 */ "vminu.vx\t\0"
  /* 6446 */ "vwmaccsu.vx\t\0"
  /* 6459 */ "vmulhsu.vx\t\0"
  /* 6471 */ "vwmulsu.vx\t\0"
  /* 6483 */ "vmsgtu.vx\t\0"
  /* 6494 */ "vmsltu.vx\t\0"
  /* 6505 */ "vdivu.vx\t\0"
  /* 6515 */ "vmaxu.vx\t\0"
  /* 6525 */ "vdiv.vx\t\0"
  /* 6534 */ "vmax.vx\t\0"
  /* 6543 */ "vnsra.wx\t\0"
  /* 6553 */ "vwsub.wx\t\0"
  /* 6563 */ "vwadd.wx\t\0"
  /* 6573 */ "vnsrl.wx\t\0"
  /* 6583 */ "vnclip.wx\t\0"
  /* 6594 */ "vwsubu.wx\t\0"
  /* 6605 */ "vwaddu.wx\t\0"
  /* 6616 */ "vnclipu.wx\t\0"
  /* 6628 */ "c.bnez\t\0"
  /* 6636 */ "clz\t\0"
  /* 6641 */ "c.beqz\t\0"
  /* 6649 */ "ctz\t\0"
  /* 6654 */ "# XRay Function Patchable RET.\0"
  /* 6685 */ "# XRay Typed Event Log.\0"
  /* 6709 */ "# XRay Custom Event Log.\0"
  /* 6734 */ "# XRay Function Enter.\0"
  /* 6757 */ "# XRay Tail Call Exit.\0"
  /* 6780 */ "# XRay Function Exit.\0"
  /* 6802 */ "LIFETIME_END\0"
  /* 6815 */ "BUNDLE\0"
  /* 6822 */ "DBG_VALUE\0"
  /* 6832 */ "DBG_LABEL\0"
  /* 6842 */ "LIFETIME_START\0"
  /* 6857 */ "# FEntry call\0"
};
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

  static const uint32_t OpInfo0[] = {
    0U,	// PHI
    0U,	// INLINEASM
    0U,	// INLINEASM_BR
    0U,	// CFI_INSTRUCTION
    0U,	// EH_LABEL
    0U,	// GC_LABEL
    0U,	// ANNOTATION_LABEL
    0U,	// KILL
    0U,	// EXTRACT_SUBREG
    0U,	// INSERT_SUBREG
    0U,	// IMPLICIT_DEF
    0U,	// SUBREG_TO_REG
    0U,	// COPY_TO_REGCLASS
    6823U,	// DBG_VALUE
    6833U,	// DBG_LABEL
    0U,	// REG_SEQUENCE
    0U,	// COPY
    6816U,	// BUNDLE
    6843U,	// LIFETIME_START
    6803U,	// LIFETIME_END
    0U,	// STACKMAP
    6858U,	// FENTRY_CALL
    0U,	// PATCHPOINT
    0U,	// LOAD_STACK_GUARD
    0U,	// PREALLOCATED_SETUP
    0U,	// PREALLOCATED_ARG
    0U,	// STATEPOINT
    0U,	// LOCAL_ESCAPE
    0U,	// FAULTING_OP
    0U,	// PATCHABLE_OP
    6735U,	// PATCHABLE_FUNCTION_ENTER
    6655U,	// PATCHABLE_RET
    6781U,	// PATCHABLE_FUNCTION_EXIT
    6758U,	// PATCHABLE_TAIL_CALL
    6710U,	// PATCHABLE_EVENT_CALL
    6686U,	// PATCHABLE_TYPED_EVENT_CALL
    0U,	// ICALL_BRANCH_FUNNEL
    0U,	// G_ADD
    0U,	// G_SUB
    0U,	// G_MUL
    0U,	// G_SDIV
    0U,	// G_UDIV
    0U,	// G_SREM
    0U,	// G_UREM
    0U,	// G_AND
    0U,	// G_OR
    0U,	// G_XOR
    0U,	// G_IMPLICIT_DEF
    0U,	// G_PHI
    0U,	// G_FRAME_INDEX
    0U,	// G_GLOBAL_VALUE
    0U,	// G_EXTRACT
    0U,	// G_UNMERGE_VALUES
    0U,	// G_INSERT
    0U,	// G_MERGE_VALUES
    0U,	// G_BUILD_VECTOR
    0U,	// G_BUILD_VECTOR_TRUNC
    0U,	// G_CONCAT_VECTORS
    0U,	// G_PTRTOINT
    0U,	// G_INTTOPTR
    0U,	// G_BITCAST
    0U,	// G_FREEZE
    0U,	// G_INTRINSIC_TRUNC
    0U,	// G_INTRINSIC_ROUND
    0U,	// G_READCYCLECOUNTER
    0U,	// G_LOAD
    0U,	// G_SEXTLOAD
    0U,	// G_ZEXTLOAD
    0U,	// G_INDEXED_LOAD
    0U,	// G_INDEXED_SEXTLOAD
    0U,	// G_INDEXED_ZEXTLOAD
    0U,	// G_STORE
    0U,	// G_INDEXED_STORE
    0U,	// G_ATOMIC_CMPXCHG_WITH_SUCCESS
    0U,	// G_ATOMIC_CMPXCHG
    0U,	// G_ATOMICRMW_XCHG
    0U,	// G_ATOMICRMW_ADD
    0U,	// G_ATOMICRMW_SUB
    0U,	// G_ATOMICRMW_AND
    0U,	// G_ATOMICRMW_NAND
    0U,	// G_ATOMICRMW_OR
    0U,	// G_ATOMICRMW_XOR
    0U,	// G_ATOMICRMW_MAX
    0U,	// G_ATOMICRMW_MIN
    0U,	// G_ATOMICRMW_UMAX
    0U,	// G_ATOMICRMW_UMIN
    0U,	// G_ATOMICRMW_FADD
    0U,	// G_ATOMICRMW_FSUB
    0U,	// G_FENCE
    0U,	// G_BRCOND
    0U,	// G_BRINDIRECT
    0U,	// G_INTRINSIC
    0U,	// G_INTRINSIC_W_SIDE_EFFECTS
    0U,	// G_ANYEXT
    0U,	// G_TRUNC
    0U,	// G_CONSTANT
    0U,	// G_FCONSTANT
    0U,	// G_VASTART
    0U,	// G_VAARG
    0U,	// G_SEXT
    0U,	// G_SEXT_INREG
    0U,	// G_ZEXT
    0U,	// G_SHL
    0U,	// G_LSHR
    0U,	// G_ASHR
    0U,	// G_FSHL
    0U,	// G_FSHR
    0U,	// G_ICMP
    0U,	// G_FCMP
    0U,	// G_SELECT
    0U,	// G_UADDO
    0U,	// G_UADDE
    0U,	// G_USUBO
    0U,	// G_USUBE
    0U,	// G_SADDO
    0U,	// G_SADDE
    0U,	// G_SSUBO
    0U,	// G_SSUBE
    0U,	// G_UMULO
    0U,	// G_SMULO
    0U,	// G_UMULH
    0U,	// G_SMULH
    0U,	// G_UADDSAT
    0U,	// G_SADDSAT
    0U,	// G_USUBSAT
    0U,	// G_SSUBSAT
    0U,	// G_FADD
    0U,	// G_FSUB
    0U,	// G_FMUL
    0U,	// G_FMA
    0U,	// G_FMAD
    0U,	// G_FDIV
    0U,	// G_FREM
    0U,	// G_FPOW
    0U,	// G_FEXP
    0U,	// G_FEXP2
    0U,	// G_FLOG
    0U,	// G_FLOG2
    0U,	// G_FLOG10
    0U,	// G_FNEG
    0U,	// G_FPEXT
    0U,	// G_FPTRUNC
    0U,	// G_FPTOSI
    0U,	// G_FPTOUI
    0U,	// G_SITOFP
    0U,	// G_UITOFP
    0U,	// G_FABS
    0U,	// G_FCOPYSIGN
    0U,	// G_FCANONICALIZE
    0U,	// G_FMINNUM
    0U,	// G_FMAXNUM
    0U,	// G_FMINNUM_IEEE
    0U,	// G_FMAXNUM_IEEE
    0U,	// G_FMINIMUM
    0U,	// G_FMAXIMUM
    0U,	// G_PTR_ADD
    0U,	// G_PTRMASK
    0U,	// G_SMIN
    0U,	// G_SMAX
    0U,	// G_UMIN
    0U,	// G_UMAX
    0U,	// G_BR
    0U,	// G_BRJT
    0U,	// G_INSERT_VECTOR_ELT
    0U,	// G_EXTRACT_VECTOR_ELT
    0U,	// G_SHUFFLE_VECTOR
    0U,	// G_CTTZ
    0U,	// G_CTTZ_ZERO_UNDEF
    0U,	// G_CTLZ
    0U,	// G_CTLZ_ZERO_UNDEF
    0U,	// G_CTPOP
    0U,	// G_BSWAP
    0U,	// G_BITREVERSE
    0U,	// G_FCEIL
    0U,	// G_FCOS
    0U,	// G_FSIN
    0U,	// G_FSQRT
    0U,	// G_FFLOOR
    0U,	// G_FRINT
    0U,	// G_FNEARBYINT
    0U,	// G_ADDRSPACE_CAST
    0U,	// G_BLOCK_ADDR
    0U,	// G_JUMP_TABLE
    0U,	// G_DYN_STACKALLOC
    0U,	// G_STRICT_FADD
    0U,	// G_STRICT_FSUB
    0U,	// G_STRICT_FMUL
    0U,	// G_STRICT_FDIV
    0U,	// G_STRICT_FREM
    0U,	// G_STRICT_FMA
    0U,	// G_STRICT_FSQRT
    0U,	// G_READ_REGISTER
    0U,	// G_WRITE_REGISTER
    9U,	// ADJCALLSTACKDOWN
    9U,	// ADJCALLSTACKUP
    9U,	// BuildPairF64Pseudo
    8648U,	// PseudoAddTPRel
    9U,	// PseudoAtomicLoadNand32
    9U,	// PseudoAtomicLoadNand64
    9U,	// PseudoBR
    9U,	// PseudoBRIND
    42472U,	// PseudoCALL
    9U,	// PseudoCALLIndirect
    1058280U,	// PseudoCALLReg
    9U,	// PseudoCmpXchg32
    9U,	// PseudoCmpXchg64
    37888480U,	// PseudoFLD
    37893640U,	// PseudoFLW
    37888500U,	// PseudoFSD
    37893761U,	// PseudoFSW
    281066U,	// PseudoJump
    1056800U,	// PseudoLA
    1057229U,	// PseudoLA_TLS_GD
    1057285U,	// PseudoLA_TLS_IE
    1056848U,	// PseudoLB
    1060253U,	// PseudoLBU
    1057242U,	// PseudoLD
    1057740U,	// PseudoLH
    1060266U,	// PseudoLHU
    1057812U,	// PseudoLI
    1056799U,	// PseudoLLA
    1062402U,	// PseudoLW
    1060389U,	// PseudoLWU
    9U,	// PseudoMaskedAtomicLoadAdd32
    9U,	// PseudoMaskedAtomicLoadMax32
    9U,	// PseudoMaskedAtomicLoadMin32
    9U,	// PseudoMaskedAtomicLoadNand32
    9U,	// PseudoMaskedAtomicLoadSub32
    9U,	// PseudoMaskedAtomicLoadUMax32
    9U,	// PseudoMaskedAtomicLoadUMin32
    9U,	// PseudoMaskedAtomicSwap32
    9U,	// PseudoMaskedCmpXchg32
    9U,	// PseudoRET
    37888084U,	// PseudoSB
    37888494U,	// PseudoSD
    37888976U,	// PseudoSH
    37893755U,	// PseudoSW
    42465U,	// PseudoTAIL
    9U,	// PseudoTAILIndirect
    9U,	// ReadCycleWide
    9U,	// Select_FPR32_Using_CC_GPR
    9U,	// Select_FPR64_Using_CC_GPR
    9U,	// Select_GPR_Using_CC_GPR
    9U,	// SplitF64Pseudo
    33563080U,	// ADD
    33563647U,	// ADDI
    33568165U,	// ADDIW
    33566237U,	// ADDIWU
    33568023U,	// ADDUW
    33568133U,	// ADDW
    33566230U,	// ADDWU
    8528066U,	// AMOADD_D
    8530512U,	// AMOADD_D_AQ
    8529685U,	// AMOADD_D_AQ_RL
    8529409U,	// AMOADD_D_RL
    8533116U,	// AMOADD_W
    8530649U,	// AMOADD_W_AQ
    8529844U,	// AMOADD_W_AQ_RL
    8529546U,	// AMOADD_W_RL
    8528076U,	// AMOAND_D
    8530525U,	// AMOAND_D_AQ
    8529700U,	// AMOAND_D_AQ_RL
    8529422U,	// AMOAND_D_RL
    8533126U,	// AMOAND_W
    8530662U,	// AMOAND_W_AQ
    8529859U,	// AMOAND_W_AQ_RL
    8529559U,	// AMOAND_W_RL
    8528260U,	// AMOMAXU_D
    8530613U,	// AMOMAXU_D_AQ
    8529802U,	// AMOMAXU_D_AQ_RL
    8529510U,	// AMOMAXU_D_RL
    8533314U,	// AMOMAXU_W
    8530750U,	// AMOMAXU_W_AQ
    8529961U,	// AMOMAXU_W_AQ_RL
    8529647U,	// AMOMAXU_W_RL
    8528306U,	// AMOMAX_D
    8530627U,	// AMOMAX_D_AQ
    8529818U,	// AMOMAX_D_AQ_RL
    8529524U,	// AMOMAX_D_RL
    8533348U,	// AMOMAX_W
    8530764U,	// AMOMAX_W_AQ
    8529977U,	// AMOMAX_W_AQ_RL
    8529661U,	// AMOMAX_W_RL
    8528238U,	// AMOMINU_D
    8530599U,	// AMOMINU_D_AQ
    8529786U,	// AMOMINU_D_AQ_RL
    8529496U,	// AMOMINU_D_RL
    8533288U,	// AMOMINU_W
    8530736U,	// AMOMINU_W_AQ
    8529945U,	// AMOMINU_W_AQ_RL
    8529633U,	// AMOMINU_W_RL
    8528128U,	// AMOMIN_D
    8530538U,	// AMOMIN_D_AQ
    8529715U,	// AMOMIN_D_AQ_RL
    8529435U,	// AMOMIN_D_RL
    8533197U,	// AMOMIN_W
    8530675U,	// AMOMIN_W_AQ
    8529874U,	// AMOMIN_W_AQ_RL
    8529572U,	// AMOMIN_W_RL
    8528172U,	// AMOOR_D
    8530574U,	// AMOOR_D_AQ
    8529757U,	// AMOOR_D_AQ_RL
    8529471U,	// AMOOR_D_RL
    8533224U,	// AMOOR_W
    8530711U,	// AMOOR_W_AQ
    8529916U,	// AMOOR_W_AQ_RL
    8529608U,	// AMOOR_W_RL
    8528148U,	// AMOSWAP_D
    8530551U,	// AMOSWAP_D_AQ
    8529730U,	// AMOSWAP_D_AQ_RL
    8529448U,	// AMOSWAP_D_RL
    8533207U,	// AMOSWAP_W
    8530688U,	// AMOSWAP_W_AQ
    8529889U,	// AMOSWAP_W_AQ_RL
    8529585U,	// AMOSWAP_W_RL
    8528181U,	// AMOXOR_D
    8530586U,	// AMOXOR_D_AQ
    8529771U,	// AMOXOR_D_AQ_RL
    8529483U,	// AMOXOR_D_RL
    8533233U,	// AMOXOR_W
    8530723U,	// AMOXOR_W_AQ
    8529930U,	// AMOXOR_W_AQ_RL
    8529620U,	// AMOXOR_W_RL
    33563111U,	// AND
    33563655U,	// ANDI
    33565083U,	// ANDN
    1056863U,	// AUIPC
    33565132U,	// BDEP
    33568328U,	// BDEPW
    33565529U,	// BEQ
    33566103U,	// BEXT
    33568406U,	// BEXTW
    33565138U,	// BFP
    33568335U,	// BFPW
    33563136U,	// BGE
    33566114U,	// BGEU
    33566079U,	// BLT
    33566183U,	// BLTU
    1059287U,	// BMATFLIP
    33565580U,	// BMATOR
    33565595U,	// BMATXOR
    33563152U,	// BNE
    33564754U,	// CLMUL
    33563592U,	// CLMULH
    33568139U,	// CLMULHW
    33565555U,	// CLMULR
    33568349U,	// CLMULRW
    33568302U,	// CLMULW
    1063405U,	// CLZ
    1062600U,	// CLZW
    4339453U,	// CMIX
    4337803U,	// CMOV
    1056821U,	// CRC32B
    1056830U,	// CRC32CB
    1056919U,	// CRC32CD
    1057711U,	// CRC32CH
    1061986U,	// CRC32CW
    1056883U,	// CRC32D
    1057702U,	// CRC32H
    1061977U,	// CRC32W
    401516U,	// CSRRC
    402421U,	// CSRRCI
    404619U,	// CSRRS
    402528U,	// CSRRSI
    407148U,	// CSRRW
    402849U,	// CSRRWI
    1063418U,	// CTZ
    1062606U,	// CTZW
    1196486U,	// C_ADD
    1197053U,	// C_ADDI
    1198583U,	// C_ADDI16SP
    33565094U,	// C_ADDI4SPN
    1201571U,	// C_ADDIW
    1197053U,	// C_ADDI_HINT_IMM_ZERO
    1197053U,	// C_ADDI_HINT_X0
    1197053U,	// C_ADDI_NOP
    1201539U,	// C_ADDW
    1196486U,	// C_ADD_HINT
    1196517U,	// C_AND
    1197061U,	// C_ANDI
    1063410U,	// C_BEQZ
    1063397U,	// C_BNEZ
    1454U,	// C_EBREAK
    2236894U,	// C_FLD
    2238987U,	// C_FLDSP
    2242054U,	// C_FLW
    2239021U,	// C_FLWSP
    2236914U,	// C_FSD
    2239004U,	// C_FSDSP
    2242175U,	// C_FSW
    2239038U,	// C_FSWSP
    42409U,	// C_J
    42450U,	// C_JAL
    43876U,	// C_JALR
    43870U,	// C_JR
    2236888U,	// C_LD
    2238979U,	// C_LDSP
    1057810U,	// C_LI
    1057810U,	// C_LI_HINT
    1057918U,	// C_LUI
    1057918U,	// C_LUI_HINT
    2242048U,	// C_LW
    2239013U,	// C_LWSP
    1060990U,	// C_MV
    1060990U,	// C_MV_HINT
    50079U,	// C_NEG
    2544U,	// C_NOP
    43504U,	// C_NOP_HINT
    52623U,	// C_NOT
    1198971U,	// C_OR
    2236908U,	// C_SD
    2238996U,	// C_SDSP
    1197089U,	// C_SLLI
    49163U,	// C_SLLI64_HINT
    1197089U,	// C_SLLI_HINT
    1197030U,	// C_SRAI
    49153U,	// C_SRAI64_HINT
    1197097U,	// C_SRLI
    49173U,	// C_SRLI64_HINT
    1196120U,	// C_SUB
    1201524U,	// C_SUBW
    2242169U,	// C_SW
    2239030U,	// C_SWSP
    2529U,	// C_UNIMP
    1198996U,	// C_XOR
    54533U,	// C_ZEXTW
    33566841U,	// DIV
    33566195U,	// DIVU
    33568428U,	// DIVUW
    33568442U,	// DIVW
    3424U,	// DRET
    1456U,	// EBREAK
    1511U,	// ECALL
    134226087U,	// FADD_D
    134228942U,	// FADD_S
    1057097U,	// FCLASS_D
    1059886U,	// FCLASS_S
    12592574U,	// FCVT_D_L
    12594621U,	// FCVT_D_LU
    1059780U,	// FCVT_D_S
    1062002U,	// FCVT_D_W
    1060345U,	// FCVT_D_WU
    12591459U,	// FCVT_LU_D
    12594248U,	// FCVT_LU_S
    12591334U,	// FCVT_L_D
    12594179U,	// FCVT_L_S
    12591423U,	// FCVT_S_D
    12592584U,	// FCVT_S_L
    12594632U,	// FCVT_S_LU
    12596475U,	// FCVT_S_W
    12594692U,	// FCVT_S_WU
    12591481U,	// FCVT_WU_D
    12594259U,	// FCVT_WU_S
    12591511U,	// FCVT_W_D
    12594278U,	// FCVT_W_S
    134226319U,	// FDIV_D
    134229086U,	// FDIV_S
    25081U,	// FENCE
    980U,	// FENCE_I
    2497U,	// FENCE_TSO
    33562911U,	// FEQ_D
    33565735U,	// FEQ_S
    2236896U,	// FLD
    33562838U,	// FLE_D
    33565673U,	// FLE_S
    33562963U,	// FLT_D
    33565752U,	// FLT_S
    2242056U,	// FLW
    268443823U,	// FMADD_D
    268446678U,	// FMADD_S
    33563050U,	// FMAX_D
    33565817U,	// FMAX_S
    33562872U,	// FMIN_D
    33565717U,	// FMIN_S
    268443780U,	// FMSUB_D
    268446641U,	// FMSUB_S
    134226160U,	// FMUL_D
    134229005U,	// FMUL_S
    1062612U,	// FMV_D_X
    1062639U,	// FMV_W_X
    1057185U,	// FMV_X_D
    1062235U,	// FMV_X_W
    268443832U,	// FNMADD_D
    268446687U,	// FNMADD_S
    268443789U,	// FNMSUB_D
    268446650U,	// FNMSUB_S
    2236916U,	// FSD
    33562890U,	// FSGNJN_D
    33565725U,	// FSGNJN_S
    33563068U,	// FSGNJX_D
    33565825U,	// FSGNJX_S
    33562845U,	// FSGNJ_D
    33565690U,	// FSGNJ_S
    16787533U,	// FSL
    16791080U,	// FSLW
    12591450U,	// FSQRT_D
    12594239U,	// FSQRT_S
    16788388U,	// FSR
    9306U,	// FSRI
    13784U,	// FSRIW
    16791155U,	// FSRW
    134226044U,	// FSUB_D
    134228905U,	// FSUB_S
    2242177U,	// FSW
    33562726U,	// GORC
    33563630U,	// GORCI
    33568155U,	// GORCIW
    33568124U,	// GORCW
    33566835U,	// GREV
    33564007U,	// GREVI
    33568232U,	// GREVIW
    33568435U,	// GREVW
    1058260U,	// JAL
    2239334U,	// JALR
    2236496U,	// LB
    2239901U,	// LBU
    2236890U,	// LD
    2237388U,	// LH
    2239914U,	// LHU
    532774U,	// LR_D
    535173U,	// LR_D_AQ
    534354U,	// LR_D_AQ_RL
    534070U,	// LR_D_RL
    537826U,	// LR_W
    535310U,	// LR_W_AQ
    534513U,	// LR_W_AQ_RL
    534207U,	// LR_W_RL
    1057920U,	// LUI
    2242050U,	// LW
    2240037U,	// LWU
    33568504U,	// MAX
    33566250U,	// MAXU
    33565089U,	// MIN
    33566169U,	// MINU
    3430U,	// MRET
    33564756U,	// MUL
    33563594U,	// MULH
    33566175U,	// MULHSU
    33566120U,	// MULHU
    33568304U,	// MULW
    33565565U,	// OR
    33563727U,	// ORI
    33565106U,	// ORN
    33564088U,	// PACK
    33563585U,	// PACKH
    33566134U,	// PACKU
    33568413U,	// PACKUW
    33568249U,	// PACKW
    1060233U,	// PCNT
    1062542U,	// PCNTW
    33564824U,	// REM
    33566163U,	// REMU
    33568421U,	// REMUW
    33568310U,	// REMW
    33564147U,	// ROL
    33568284U,	// ROLW
    33565575U,	// ROR
    33563726U,	// RORI
    33568209U,	// RORIW
    33568358U,	// RORW
    2236500U,	// SB
    33565548U,	// SBCLR
    33563718U,	// SBCLRI
    33568200U,	// SBCLRIW
    33568341U,	// SBCLRW
    33566102U,	// SBEXT
    33563766U,	// SBEXTI
    33568405U,	// SBEXTW
    33566852U,	// SBINV
    33564014U,	// SBINVI
    33568240U,	// SBINVIW
    33568448U,	// SBINVW
    33566072U,	// SBSET
    33563752U,	// SBSETI
    33568223U,	// SBSETIW
    33568390U,	// SBSETW
    8528033U,	// SC_D
    8530503U,	// SC_D_AQ
    8529674U,	// SC_D_AQ_RL
    8529400U,	// SC_D_RL
    8533100U,	// SC_W
    8530640U,	// SC_W_AQ
    8529833U,	// SC_W_AQ_RL
    8529537U,	// SC_W_RL
    2236910U,	// SD
    1056840U,	// SEXTB
    1057721U,	// SEXTH
    1056804U,	// SFENCE_VMA
    2237392U,	// SH
    33564123U,	// SHFL
    33563674U,	// SHFLI
    33568271U,	// SHFLW
    33564142U,	// SLL
    33563683U,	// SLLI
    33568031U,	// SLLIUW
    33568172U,	// SLLIW
    33568278U,	// SLLW
    33565111U,	// SLO
    33563706U,	// SLOI
    33568186U,	// SLOIW
    33568316U,	// SLOW
    33566084U,	// SLT
    33563760U,	// SLTI
    33566127U,	// SLTIU
    33566189U,	// SLTU
    33562672U,	// SRA
    33563624U,	// SRAI
    33568148U,	// SRAIW
    33568110U,	// SRAW
    3436U,	// SRET
    33564744U,	// SRL
    33563691U,	// SRLI
    33568179U,	// SRLIW
    33568290U,	// SRLW
    33565116U,	// SRO
    33563712U,	// SROI
    33568193U,	// SROIW
    33568322U,	// SROW
    33562714U,	// SUB
    33568015U,	// SUBUW
    33568118U,	// SUBW
    33566223U,	// SUBWU
    2242171U,	// SW
    2531U,	// UNIMP
    33564121U,	// UNSHFL
    33563672U,	// UNSHFLI
    33568269U,	// UNSHFLW
    3442U,	// URET
    67121954U,	// VAADDU_VV
    67123417U,	// VAADDU_VX
    67121558U,	// VAADD_VV
    67123078U,	// VAADD_VX
    100673717U,	// VADC_VIM
    100673871U,	// VADC_VVM
    100673925U,	// VADC_VXM
    67118262U,	// VADD_VI
    67121621U,	// VADD_VV
    67123108U,	// VADD_VX
    67118271U,	// VAND_VI
    67121651U,	// VAND_VV
    67123127U,	// VAND_VX
    67121909U,	// VASUBU_VV
    67123372U,	// VASUBU_VX
    67121316U,	// VASUB_VV
    67122966U,	// VASUB_VX
    33564961U,	// VCOMPRESS_VM
    67122088U,	// VDIVU_VV
    67123562U,	// VDIVU_VX
    67122118U,	// VDIV_VV
    67123582U,	// VDIV_VX
    67117760U,	// VFADD_VF
    67121568U,	// VFADD_VV
    3157912U,	// VFCLASS_V
    3158011U,	// VFCVT_F_XU_V
    3158104U,	// VFCVT_F_X_V
    3157683U,	// VFCVT_XU_F_V
    3157712U,	// VFCVT_X_F_V
    67117918U,	// VFDIV_VF
    67122108U,	// VFDIV_VV
    3156110U,	// VFIRST_M
    67117712U,	// VFMACC_VF
    67121479U,	// VFMACC_VV
    67117770U,	// VFMADD_VF
    67121578U,	// VFMADD_VV
    67117939U,	// VFMAX_VF
    67122127U,	// VFMAX_VV
    100673693U,	// VFMERGE_VFM
    67117866U,	// VFMIN_VF
    67121808U,	// VFMIN_VV
    67117664U,	// VFMSAC_VF
    67121410U,	// VFMSAC_VV
    67117619U,	// VFMSUB_VF
    67121336U,	// VFMSUB_VV
    67117845U,	// VFMUL_VF
    67121749U,	// VFMUL_VV
    1059824U,	// VFMV_F_S
    1057301U,	// VFMV_S_F
    1057311U,	// VFMV_V_F
    3159202U,	// VFNCVT_F_F_W
    3159347U,	// VFNCVT_F_XU_W
    3159373U,	// VFNCVT_F_X_W
    3159184U,	// VFNCVT_ROD_F_F_W
    3159216U,	// VFNCVT_XU_F_W
    3159231U,	// VFNCVT_X_F_W
    67117723U,	// VFNMACC_VF
    67121490U,	// VFNMACC_VV
    67117781U,	// VFNMADD_VF
    67121589U,	// VFNMADD_VV
    67117675U,	// VFNMSAC_VF
    67121421U,	// VFNMSAC_VV
    67117630U,	// VFNMSUB_VF
    67121347U,	// VFNMSUB_VV
    67117928U,	// VFRDIV_VF
    67120455U,	// VFREDMAX_VS
    67120367U,	// VFREDMIN_VS
    67120338U,	// VFREDOSUM_VS
    67120286U,	// VFREDSUM_VS
    67117642U,	// VFRSUB_VF
    67117876U,	// VFSGNJN_VF
    67121827U,	// VFSGNJN_VV
    67117949U,	// VFSGNJX_VF
    67122146U,	// VFSGNJX_VV
    67117834U,	// VFSGNJ_VF
    67121710U,	// VFSGNJ_VV
    3157923U,	// VFSQRT_V
    67117609U,	// VFSUB_VF
    67121326U,	// VFSUB_VV
    67117793U,	// VFWADD_VF
    67121630U,	// VFWADD_VV
    67117972U,	// VFWADD_WF
    67122189U,	// VFWADD_WV
    3157669U,	// VFWCVT_F_F_V
    3158025U,	// VFWCVT_F_XU_V
    3158117U,	// VFWCVT_F_X_V
    3157697U,	// VFWCVT_XU_F_V
    3157725U,	// VFWCVT_X_F_V
    67117748U,	// VFWMACC_VF
    67121525U,	// VFWMACC_VV
    67117700U,	// VFWMSAC_VF
    67121457U,	// VFWMSAC_VV
    67117855U,	// VFWMUL_VF
    67121778U,	// VFWMUL_VV
    67117735U,	// VFWNMACC_VF
    67121502U,	// VFWNMACC_VV
    67117687U,	// VFWNMSAC_VF
    67121444U,	// VFWNMSAC_VV
    67120352U,	// VFWREDOSUM_VS
    67120311U,	// VFWREDSUM_VS
    67117653U,	// VFWSUB_VF
    67121389U,	// VFWSUB_VV
    67117961U,	// VFWSUB_WF
    67122168U,	// VFWSUB_WV
    77415U,	// VID_V
    3156065U,	// VIOTA_M
    1814372U,	// VL1R_V
    3911403U,	// VLBFF_V
    3911430U,	// VLBUFF_V
    3911597U,	// VLBU_V
    3911216U,	// VLB_V
    3911412U,	// VLEFF_V
    3911278U,	// VLE_V
    3911421U,	// VLHFF_V
    3911440U,	// VLHUFF_V
    3911623U,	// VLHU_V
    3911469U,	// VLH_V
    896949U,	// VLSBU_V
    896567U,	// VLSB_V
    896629U,	// VLSE_V
    896975U,	// VLSHU_V
    896820U,	// VLSH_V
    897001U,	// VLSWU_V
    897064U,	// VLSW_V
    3911460U,	// VLWFF_V
    3911450U,	// VLWUFF_V
    3911649U,	// VLWU_V
    3911713U,	// VLW_V
    896958U,	// VLXBU_V
    896590U,	// VLXB_V
    896652U,	// VLXE_V
    896984U,	// VLXHU_V
    896843U,	// VLXH_V
    897010U,	// VLXWU_V
    897087U,	// VLXW_V
    67121515U,	// VMACC_VV
    67123047U,	// VMACC_VX
    33563810U,	// VMADC_VI
    100673706U,	// VMADC_VIM
    33567116U,	// VMADC_VV
    100673860U,	// VMADC_VVM
    33568636U,	// VMADC_VX
    100673914U,	// VMADC_VXM
    67121601U,	// VMADD_VV
    67123088U,	// VMADD_VX
    33564936U,	// VMANDNOT_MM
    33564875U,	// VMAND_MM
    67122098U,	// VMAXU_VV
    67123572U,	// VMAXU_VX
    67122137U,	// VMAX_VV
    67123591U,	// VMAX_VX
    100673727U,	// VMERGE_VIM
    100673881U,	// VMERGE_VVM
    100673935U,	// VMERGE_VXM
    67117888U,	// VMFEQ_VF
    67121839U,	// VMFEQ_VV
    67117804U,	// VMFGE_VF
    67117898U,	// VMFGT_VF
    67117814U,	// VMFLE_VF
    67121660U,	// VMFLE_VV
    67117908U,	// VMFLT_VF
    67121889U,	// VMFLT_VV
    67117824U,	// VMFNE_VF
    67121680U,	// VMFNE_VV
    67122030U,	// VMINU_VV
    67123493U,	// VMINU_VX
    67121818U,	// VMIN_VV
    67123232U,	// VMIN_VX
    33564885U,	// VMNAND_MM
    33564905U,	// VMNOR_MM
    33564949U,	// VMORNOT_MM
    33564896U,	// VMOR_MM
    33567037U,	// VMSBC_VV
    100673839U,	// VMSBC_VVM
    33568605U,	// VMSBC_VX
    100673893U,	// VMSBC_VXM
    3156083U,	// VMSBF_M
    67118356U,	// VMSEQ_VI
    67121849U,	// VMSEQ_VV
    67123299U,	// VMSEQ_VX
    67118428U,	// VMSGTU_VI
    67123540U,	// VMSGTU_VX
    67118396U,	// VMSGT_VI
    67123352U,	// VMSGT_VX
    3156092U,	// VMSIF_M
    67118417U,	// VMSLEU_VI
    67121987U,	// VMSLEU_VV
    67123450U,	// VMSLEU_VX
    67118280U,	// VMSLE_VI
    67121670U,	// VMSLE_VV
    67123136U,	// VMSLE_VX
    67122077U,	// VMSLTU_VV
    67123551U,	// VMSLTU_VX
    67121899U,	// VMSLT_VV
    67123362U,	// VMSLT_VX
    67118290U,	// VMSNE_VI
    67121690U,	// VMSNE_VV
    67123146U,	// VMSNE_VX
    3156101U,	// VMSOF_M
    67122053U,	// VMULHSU_VV
    67123516U,	// VMULHSU_VX
    67121998U,	// VMULHU_VV
    67123461U,	// VMULHU_VX
    67121700U,	// VMULH_VV
    67123156U,	// VMULH_VX
    67121769U,	// VMUL_VV
    67123204U,	// VMUL_VX
    1060724U,	// VMV1R_V
    1060733U,	// VMV2R_V
    1060742U,	// VMV4R_V
    1060751U,	// VMV8R_V
    1062621U,	// VMV_S_X
    1057757U,	// VMV_V_I
    1060888U,	// VMV_V_V
    1062630U,	// VMV_V_X
    1059952U,	// VMV_X_S
    33564915U,	// VMXNOR_MM
    33564926U,	// VMXOR_MM
    67118485U,	// VNCLIPU_WI
    67122253U,	// VNCLIPU_WV
    67123673U,	// VNCLIPU_WX
    67118474U,	// VNCLIP_WI
    67122220U,	// VNCLIP_WV
    67123640U,	// VNCLIP_WX
    67121433U,	// VNMSAC_VV
    67123026U,	// VNMSAC_VX
    67121359U,	// VNMSUB_VV
    67122976U,	// VNMSUB_VX
    67118454U,	// VNSRA_WI
    67122158U,	// VNSRA_WV
    67123600U,	// VNSRA_WX
    67118464U,	// VNSRL_WI
    67122210U,	// VNSRL_WV
    67123630U,	// VNSRL_WX
    67118379U,	// VOR_VI
    67121872U,	// VOR_VV
    67123322U,	// VOR_VX
    3156074U,	// VPOPC_M
    67120274U,	// VREDAND_VS
    67120442U,	// VREDMAXU_VS
    67120468U,	// VREDMAX_VS
    67120429U,	// VREDMINU_VS
    67120380U,	// VREDMIN_VS
    67120392U,	// VREDOR_VS
    67120299U,	// VREDSUM_VS
    67120403U,	// VREDXOR_VS
    67122020U,	// VREMU_VV
    67123483U,	// VREMU_VX
    67121799U,	// VREM_VV
    67123223U,	// VREM_VX
    67118366U,	// VRGATHER_VI
    67121859U,	// VRGATHER_VV
    67123309U,	// VRGATHER_VX
    67118232U,	// VRSUB_VI
    67122987U,	// VRSUB_VX
    1814380U,	// VS1R_V
    67118406U,	// VSADDU_VI
    67121965U,	// VSADDU_VV
    67123428U,	// VSADDU_VX
    67118252U,	// VSADD_VI
    67121611U,	// VSADD_VV
    67123098U,	// VSADD_VX
    100673850U,	// VSBC_VVM
    100673904U,	// VSBC_VXM
    3911239U,	// VSB_V
    33564761U,	// VSETVL
    20980785U,	// VSETVLI
    3911301U,	// VSE_V
    3911492U,	// VSH_V
    67123241U,	// VSLIDE1DOWN_VX
    67123272U,	// VSLIDE1UP_VX
    67118328U,	// VSLIDEDOWN_VI
    67123257U,	// VSLIDEDOWN_VX
    67118343U,	// VSLIDEUP_VI
    67123286U,	// VSLIDEUP_VX
    67118300U,	// VSLL_VI
    67121721U,	// VSLL_VV
    67123166U,	// VSLL_VX
    67121759U,	// VSMUL_VV
    67123194U,	// VSMUL_VX
    67118223U,	// VSRA_VI
    67121307U,	// VSRA_VV
    67122957U,	// VSRA_VX
    67118319U,	// VSRL_VI
    67121740U,	// VSRL_VV
    67123185U,	// VSRL_VX
    896575U,	// VSSB_V
    896637U,	// VSSE_V
    896828U,	// VSSH_V
    67118213U,	// VSSRA_VI
    67121297U,	// VSSRA_VV
    67122947U,	// VSSRA_VX
    67118309U,	// VSSRL_VI
    67121730U,	// VSSRL_VV
    67123175U,	// VSSRL_VX
    67121920U,	// VSSUBU_VV
    67123383U,	// VSSUBU_VX
    67121370U,	// VSSUB_VV
    67122997U,	// VSSUB_VX
    897072U,	// VSSW_V
    67121380U,	// VSUB_VV
    67123007U,	// VSUB_VX
    896606U,	// VSUXB_V
    896668U,	// VSUXE_V
    896859U,	// VSUXH_V
    897103U,	// VSUXW_V
    3911736U,	// VSW_V
    896598U,	// VSXB_V
    896660U,	// VSXE_V
    896851U,	// VSXH_V
    897095U,	// VSXW_V
    67121976U,	// VWADDU_VV
    67123439U,	// VWADDU_VX
    67122242U,	// VWADDU_WV
    67123662U,	// VWADDU_WX
    67121641U,	// VWADD_VV
    67123117U,	// VWADD_VX
    67122200U,	// VWADD_WV
    67123620U,	// VWADD_WX
    67122040U,	// VWMACCSU_VV
    67123503U,	// VWMACCSU_VX
    67123339U,	// VWMACCUS_VX
    67121942U,	// VWMACCU_VV
    67123405U,	// VWMACCU_VX
    67121537U,	// VWMACC_VV
    67123057U,	// VWMACC_VX
    67122065U,	// VWMULSU_VV
    67123528U,	// VWMULSU_VX
    67122009U,	// VWMULU_VV
    67123472U,	// VWMULU_VX
    67121789U,	// VWMUL_VV
    67123213U,	// VWMUL_VX
    67120415U,	// VWREDSUMU_VS
    67120325U,	// VWREDSUM_VS
    67121931U,	// VWSUBU_VV
    67123394U,	// VWSUBU_VX
    67122231U,	// VWSUBU_WV
    67123651U,	// VWSUBU_WX
    67121400U,	// VWSUB_VV
    67123016U,	// VWSUB_VX
    67122179U,	// VWSUB_WV
    67123610U,	// VWSUB_WX
    67118387U,	// VXOR_VI
    67121880U,	// VXOR_VV
    67123330U,	// VXOR_VX
    1037U,	// WFI
    33565569U,	// XNOR
    33565590U,	// XOR
    33563732U,	// XORI
  };

  O << "\t";

  // Emit the opcode for the instruction.
  uint32_t Bits = 0;
  Bits |= OpInfo0[MI->getOpcode()] << 0;
  //assert(Bits != 0 && "Cannot print this instruction.");
  O << AsmStrs+(Bits & 8191)-1;
  return AsmStrs+(Bits & 8191)-1;
}

StringRef printInst(const MCInst *MI, uint64_t Address,
                                 StringRef Annot, const MCSubtargetInfo &STI,
                                 raw_ostream &O, const MCRegisterInfo &MRI) { //buraya fazladan MRI parametresi
  bool Res = false;
  const MCInst *NewMI = MI;
  MCInst UncompressedMI;
    //const MCRegisterInfo &MRI;
      //std::unique_ptr<const MCRegisterInfo> MRI(
      //TheTarget->createMCRegInfo(TripleName));
  Res = uncompressInst(UncompressedMI, *MI, MRI, STI);
  if (Res)
    NewMI = const_cast<MCInst *>(&UncompressedMI);
    //O<<" ara ";
    printAliasInstr(NewMI, Address, STI, O);
    //O<<" ara ";
    return printInstruction(NewMI, Address, STI, O);
//O<<" ara ";
}



}


/////////////////////////////ekledim

namespace {
struct FilterResult {
  // True if the section should not be skipped.
  bool Keep;

  // True if the index counter should be incremented, even if the section should
  // be skipped. For example, sections may be skipped if they are not included
  // in the --section flag, but we still want those to count toward the section
  // count.
  bool IncrementIndex;
};
} // namespace

static FilterResult checkSectionFilter(object::SectionRef S) {
  if (FilterSections.empty())
    return {/*Keep=*/true, /*IncrementIndex=*/true};

  Expected<StringRef> SecNameOrErr = S.getName();
  if (!SecNameOrErr) {
    consumeError(SecNameOrErr.takeError());
    return {/*Keep=*/false, /*IncrementIndex=*/false};
  }
  StringRef SecName = *SecNameOrErr;

  // StringSet does not allow empty key so avoid adding sections with
  // no name (such as the section with index 0) here.
  if (!SecName.empty())
    FoundSectionSet.insert(SecName);

  // Only show the section if it's in the FilterSections list, but always
  // increment so the indexing is stable.
  return {/*Keep=*/is_contained(FilterSections, SecName),
          /*IncrementIndex=*/true};
}

SectionFilter objdump::ToolSectionFilter(object::ObjectFile const &O,
                                         uint64_t *Idx) {
  // Start at UINT64_MAX so that the first index returned after an increment is
  // zero (after the unsigned wrap).
  if (Idx)
    *Idx = UINT64_MAX;
  return SectionFilter(
      [Idx](object::SectionRef S) {
        FilterResult Result = checkSectionFilter(S);
        if (Idx != nullptr && Result.IncrementIndex)
          *Idx += 1;
        return Result.Keep;
      },
      O);
}

std::string objdump::getFileNameForError(const object::Archive::Child &C,
                                         unsigned Index) {
  Expected<StringRef> NameOrErr = C.getName();
  if (NameOrErr)
    return std::string(NameOrErr.get());
  // If we have an error getting the name then we print the index of the archive
  // member. Since we are already in an error state, we just ignore this error.
  consumeError(NameOrErr.takeError());
  return "<file index: " + std::to_string(Index) + ">";
}

void objdump::reportWarning(Twine Message, StringRef File) {
  // Output order between errs() and outs() matters especially for archive
  // files where the output is per member object.
  outs().flush();
  WithColor::warning(errs(), ToolName)
      << "'" << File << "': " << Message << "\n";
}

LLVM_ATTRIBUTE_NORETURN void objdump::reportError(StringRef File,
                                                  Twine Message) {
  outs().flush();
  WithColor::error(errs(), ToolName) << "'" << File << "': " << Message << "\n";
  exit(1);
}

LLVM_ATTRIBUTE_NORETURN void objdump::reportError(Error E, StringRef FileName,
                                                  StringRef ArchiveName,
                                                  StringRef ArchitectureName) {
  assert(E);
  outs().flush();
  WithColor::error(errs(), ToolName);
  if (ArchiveName != "")
    errs() << ArchiveName << "(" << FileName << ")";
  else
    errs() << "'" << FileName << "'";
  if (!ArchitectureName.empty())
    errs() << " (for architecture " << ArchitectureName << ")";
  errs() << ": ";
  logAllUnhandledErrors(std::move(E), errs());
  exit(1);
}

static void reportCmdLineWarning(Twine Message) {
  WithColor::warning(errs(), ToolName) << Message << "\n";
}

LLVM_ATTRIBUTE_NORETURN static void reportCmdLineError(Twine Message) {
  WithColor::error(errs(), ToolName) << Message << "\n";
  exit(1);
}

static void warnOnNoMatchForSections() {
  SetVector<StringRef> MissingSections;
  for (StringRef S : FilterSections) {
    if (FoundSectionSet.count(S))
      return;
    // User may specify a unnamed section. Don't warn for it.
    if (!S.empty())
      MissingSections.insert(S);
  }

  // Warn only if no section in FilterSections is matched.
  for (StringRef S : MissingSections)
    reportCmdLineWarning("section '" + S +
                         "' mentioned in a -j/--section option, but not "
                         "found in any input file");
}

static const Target *getTarget(const ObjectFile *Obj) {
  // Figure out the target triple.
  Triple TheTriple("unknown-unknown-unknown");
  if (TripleName.empty()) {
    TheTriple = Obj->makeTriple();
  } else {
    TheTriple.setTriple(Triple::normalize(TripleName));
    auto Arch = Obj->getArch();
    if (Arch == Triple::arm || Arch == Triple::armeb)
      Obj->setARMSubArch(TheTriple);
  }

  // Get the target specific parser.
  std::string Error;
  const Target *TheTarget = TargetRegistry::lookupTarget(ArchName, TheTriple,
                                                         Error);
  if (!TheTarget)
    reportError(Obj->getFileName(), "can't find target: " + Error);

  // Update the triple name and return the found target.
  TripleName = TheTriple.getTriple();
  return TheTarget;
}

bool objdump::isRelocAddressLess(RelocationRef A, RelocationRef B) {
  return A.getOffset() < B.getOffset();
}

static Error getRelocationValueString(const RelocationRef &Rel,
                                      SmallVectorImpl<char> &Result) {
  const ObjectFile *Obj = Rel.getObject();
  if (auto *ELF = dyn_cast<ELFObjectFileBase>(Obj))
    return getELFRelocationValueString(ELF, Rel, Result);
  if (auto *COFF = dyn_cast<COFFObjectFile>(Obj))
    return getCOFFRelocationValueString(COFF, Rel, Result);
  if (auto *Wasm = dyn_cast<WasmObjectFile>(Obj))
    return getWasmRelocationValueString(Wasm, Rel, Result);
  if (auto *MachO = dyn_cast<MachOObjectFile>(Obj))
    return getMachORelocationValueString(MachO, Rel, Result);
  if (auto *XCOFF = dyn_cast<XCOFFObjectFile>(Obj))
    return getXCOFFRelocationValueString(XCOFF, Rel, Result);
  llvm_unreachable("unknown object file format");
}

/// Indicates whether this relocation should hidden when listing
/// relocations, usually because it is the trailing part of a multipart
/// relocation that will be printed as part of the leading relocation.
static bool getHidden(RelocationRef RelRef) {
  auto *MachO = dyn_cast<MachOObjectFile>(RelRef.getObject());
  if (!MachO)
    return false;

  unsigned Arch = MachO->getArch();
  DataRefImpl Rel = RelRef.getRawDataRefImpl();
  uint64_t Type = MachO->getRelocationType(Rel);

  // On arches that use the generic relocations, GENERIC_RELOC_PAIR
  // is always hidden.
  if (Arch == Triple::x86 || Arch == Triple::arm || Arch == Triple::ppc)
    return Type == MachO::GENERIC_RELOC_PAIR;

  if (Arch == Triple::x86_64) {
    // On x86_64, X86_64_RELOC_UNSIGNED is hidden only when it follows
    // an X86_64_RELOC_SUBTRACTOR.
    if (Type == MachO::X86_64_RELOC_UNSIGNED && Rel.d.a > 0) {
      DataRefImpl RelPrev = Rel;
      RelPrev.d.a--;
      uint64_t PrevType = MachO->getRelocationType(RelPrev);
      if (PrevType == MachO::X86_64_RELOC_SUBTRACTOR)
        return true;
    }
  }

  return false;
}

// anonymous namespace
// namespace olduğu için sadece bu dosyada kullanılabiliyor içindekiler
// ama örneğin llvm:: ya da object:: gibi bi olaya gerek olmuyo direk kullanılabiliyo 

namespace {

/// Get the column at which we want to start printing the instruction
/// disassembly, taking into account anything which appears to the left of it.
unsigned getInstStartColumn(const MCSubtargetInfo &STI) {
  return NoShowRawInsn ? 16 : STI.getTargetTriple().isX86() ? 40 : 24;
}

/// Stores a single expression representing the location of a source-level
/// variable, along with the PC range for which that expression is valid.
struct LiveVariable {
  DWARFLocationExpression LocExpr;
  const char *VarName;
  DWARFUnit *Unit;
  const DWARFDie FuncDie;

  LiveVariable(const DWARFLocationExpression &LocExpr, const char *VarName,
               DWARFUnit *Unit, const DWARFDie FuncDie)
      : LocExpr(LocExpr), VarName(VarName), Unit(Unit), FuncDie(FuncDie) {}

  bool liveAtAddress(object::SectionedAddress Addr) {
    if (LocExpr.Range == None)
      return false;
    return LocExpr.Range->SectionIndex == Addr.SectionIndex &&
           LocExpr.Range->LowPC <= Addr.Address &&
           LocExpr.Range->HighPC > Addr.Address;
  }

  void print(raw_ostream &OS, const MCRegisterInfo &MRI) const {
    DataExtractor Data({LocExpr.Expr.data(), LocExpr.Expr.size()},
                       Unit->getContext().isLittleEndian(), 0);
    DWARFExpression Expression(Data, Unit->getAddressByteSize());
    Expression.printCompact(OS, MRI);
  }
};

/// Helper class for printing source variable locations alongside disassembly.
class LiveVariablePrinter {
  // Information we want to track about one column in which we are printing a
  // variable live range.
  struct Column {
    unsigned VarIdx = NullVarIdx;
    bool LiveIn = false;
    bool LiveOut = false;
    bool MustDrawLabel  = false;

    bool isActive() const { return VarIdx != NullVarIdx; }

    static constexpr unsigned NullVarIdx = std::numeric_limits<unsigned>::max();
  };

  // All live variables we know about in the object/image file.
  std::vector<LiveVariable> LiveVariables;

  // The columns we are currently drawing.
  IndexedMap<Column> ActiveCols;

  const MCRegisterInfo &MRI;
  const MCSubtargetInfo &STI;

  void addVariable(DWARFDie FuncDie, DWARFDie VarDie) {
    uint64_t FuncLowPC, FuncHighPC, SectionIndex;
    FuncDie.getLowAndHighPC(FuncLowPC, FuncHighPC, SectionIndex);
    const char *VarName = VarDie.getName(DINameKind::ShortName);
    DWARFUnit *U = VarDie.getDwarfUnit();

    Expected<DWARFLocationExpressionsVector> Locs =
        VarDie.getLocations(dwarf::DW_AT_location);
    if (!Locs) {
      // If the variable doesn't have any locations, just ignore it. We don't
      // report an error or warning here as that could be noisy on optimised
      // code.
      consumeError(Locs.takeError());
      return;
    }

    for (const DWARFLocationExpression &LocExpr : *Locs) {
      if (LocExpr.Range) {
        LiveVariables.emplace_back(LocExpr, VarName, U, FuncDie);
      } else {
        // If the LocExpr does not have an associated range, it is valid for
        // the whole of the function.
        // TODO: technically it is not valid for any range covered by another
        // LocExpr, does that happen in reality?
        DWARFLocationExpression WholeFuncExpr{
            DWARFAddressRange(FuncLowPC, FuncHighPC, SectionIndex),
            LocExpr.Expr};
        LiveVariables.emplace_back(WholeFuncExpr, VarName, U, FuncDie);
      }
    }
  }

  void addFunction(DWARFDie D) {
    for (const DWARFDie &Child : D.children()) {
      if (Child.getTag() == dwarf::DW_TAG_variable ||
          Child.getTag() == dwarf::DW_TAG_formal_parameter)
        addVariable(D, Child);
      else
        addFunction(Child);
    }
  }

  // Get the column number (in characters) at which the first live variable
  // line should be printed.
  unsigned getIndentLevel() const {
    return DbgIndent + getInstStartColumn(STI);
  }

  // Indent to the first live-range column to the right of the currently
  // printed line, and return the index of that column.
  // TODO: formatted_raw_ostream uses "column" to mean a number of characters
  // since the last \n, and we use it to mean the number of slots in which we
  // put live variable lines. Pick a less overloaded word.
  unsigned moveToFirstVarColumn(formatted_raw_ostream &OS) {
    // Logical column number: column zero is the first column we print in, each
    // logical column is 2 physical columns wide.
    unsigned FirstUnprintedLogicalColumn =
        std::max((int)(OS.getColumn() - getIndentLevel() + 1) / 2, 0);
    // Physical column number: the actual column number in characters, with
    // zero being the left-most side of the screen.
    unsigned FirstUnprintedPhysicalColumn =
        getIndentLevel() + FirstUnprintedLogicalColumn * 2;

    if (FirstUnprintedPhysicalColumn > OS.getColumn())
      OS.PadToColumn(FirstUnprintedPhysicalColumn);

    return FirstUnprintedLogicalColumn;
  }

  unsigned findFreeColumn() {
    for (unsigned ColIdx = 0; ColIdx < ActiveCols.size(); ++ColIdx)
      if (!ActiveCols[ColIdx].isActive())
        return ColIdx;

    size_t OldSize = ActiveCols.size();
    ActiveCols.grow(std::max<size_t>(OldSize * 2, 1));
    return OldSize;
  }

public:
  LiveVariablePrinter(const MCRegisterInfo &MRI, const MCSubtargetInfo &STI)
      : LiveVariables(), ActiveCols(Column()), MRI(MRI), STI(STI) {}

  void dump() const {
    for (const LiveVariable &LV : LiveVariables) {
      dbgs() << LV.VarName << " @ " << LV.LocExpr.Range << ": ";
      LV.print(dbgs(), MRI);
      //degistirdim
      //dbgs() << "\n";
    }
  }

  void addCompileUnit(DWARFDie D) {
    if (D.getTag() == dwarf::DW_TAG_subprogram)
      addFunction(D);
    else
      for (const DWARFDie &Child : D.children())
        addFunction(Child);
  }

  /// Update to match the state of the instruction between ThisAddr and
  /// NextAddr. In the common case, any live range active at ThisAddr is
  /// live-in to the instruction, and any live range active at NextAddr is
  /// live-out of the instruction. If IncludeDefinedVars is false, then live
  /// ranges starting at NextAddr will be ignored.
  void update(object::SectionedAddress ThisAddr,
              object::SectionedAddress NextAddr, bool IncludeDefinedVars) {
    // First, check variables which have already been assigned a column, so
    // that we don't change their order.
    SmallSet<unsigned, 8> CheckedVarIdxs;
    for (unsigned ColIdx = 0, End = ActiveCols.size(); ColIdx < End; ++ColIdx) {
      if (!ActiveCols[ColIdx].isActive())
        continue;
      CheckedVarIdxs.insert(ActiveCols[ColIdx].VarIdx);
      LiveVariable &LV = LiveVariables[ActiveCols[ColIdx].VarIdx];
      ActiveCols[ColIdx].LiveIn = LV.liveAtAddress(ThisAddr);
      ActiveCols[ColIdx].LiveOut = LV.liveAtAddress(NextAddr);
      LLVM_DEBUG(dbgs() << "pass 1, " << ThisAddr.Address << "-"
                        << NextAddr.Address << ", " << LV.VarName << ", Col "
                        << ColIdx << ": LiveIn=" << ActiveCols[ColIdx].LiveIn
                        << ", LiveOut=" << ActiveCols[ColIdx].LiveOut << "\n");

      if (!ActiveCols[ColIdx].LiveIn && !ActiveCols[ColIdx].LiveOut)
        ActiveCols[ColIdx].VarIdx = Column::NullVarIdx;
    }

    // Next, look for variables which don't already have a column, but which
    // are now live.
    if (IncludeDefinedVars) {
      for (unsigned VarIdx = 0, End = LiveVariables.size(); VarIdx < End;
           ++VarIdx) {
        if (CheckedVarIdxs.count(VarIdx))
          continue;
        LiveVariable &LV = LiveVariables[VarIdx];
        bool LiveIn = LV.liveAtAddress(ThisAddr);
        bool LiveOut = LV.liveAtAddress(NextAddr);
        if (!LiveIn && !LiveOut)
          continue;

        unsigned ColIdx = findFreeColumn();
        LLVM_DEBUG(dbgs() << "pass 2, " << ThisAddr.Address << "-"
                          << NextAddr.Address << ", " << LV.VarName << ", Col "
                          << ColIdx << ": LiveIn=" << LiveIn
                          << ", LiveOut=" << LiveOut << "\n");
        ActiveCols[ColIdx].VarIdx = VarIdx;
        ActiveCols[ColIdx].LiveIn = LiveIn;
        ActiveCols[ColIdx].LiveOut = LiveOut;
        ActiveCols[ColIdx].MustDrawLabel = true;
      }
    }
  }

  enum class LineChar {
    RangeStart,
    RangeMid,
    RangeEnd,
    LabelVert,
    LabelCornerNew,
    LabelCornerActive,
    LabelHoriz,
  };
  const char *getLineChar(LineChar C) const {
    bool IsASCII = DbgVariables == DVASCII;
    switch (C) {
    case LineChar::RangeStart:
      return IsASCII ? "^" : u8"\u2548";
    case LineChar::RangeMid:
      return IsASCII ? "|" : u8"\u2503";
    case LineChar::RangeEnd:
      return IsASCII ? "v" : u8"\u253b";
    case LineChar::LabelVert:
      return IsASCII ? "|" : u8"\u2502";
    case LineChar::LabelCornerNew:
      return IsASCII ? "/" : u8"\u250c";
    case LineChar::LabelCornerActive:
      return IsASCII ? "|" : u8"\u2520";
    case LineChar::LabelHoriz:
      return IsASCII ? "-" : u8"\u2500";
    }
    llvm_unreachable("Unhandled LineChar enum");
  }

  /// Print live ranges to the right of an existing line. This assumes the
  /// line is not an instruction, so doesn't start or end any live ranges, so
  /// we only need to print active ranges or empty columns. If AfterInst is
  /// true, this is being printed after the last instruction fed to update(),
  /// otherwise this is being printed before it.
  void printAfterOtherLine(formatted_raw_ostream &OS, bool AfterInst) { // bu da lazim
    if (ActiveCols.size()) {
      unsigned FirstUnprintedColumn = moveToFirstVarColumn(OS);
      for (size_t ColIdx = FirstUnprintedColumn, End = ActiveCols.size();
           ColIdx < End; ++ColIdx) {
        if (ActiveCols[ColIdx].isActive()) {
          if ((AfterInst && ActiveCols[ColIdx].LiveOut) ||
              (!AfterInst && ActiveCols[ColIdx].LiveIn))
            OS << getLineChar(LineChar::RangeMid);
          else if (!AfterInst && ActiveCols[ColIdx].LiveOut)
            OS << getLineChar(LineChar::LabelVert);
          else
            OS << " ";
        }
        OS << " ";
      }
    }
    //OS << "\n"; //degistirdim
  }

  /// Print any live variable range info needed to the right of a
  /// non-instruction line of disassembly. This is where we print the variable
  /// names and expressions, with thin line-drawing characters connecting them
  /// to the live range which starts at the next instruction. If MustPrint is
  /// true, we have to print at least one line (with the continuation of any
  /// already-active live ranges) because something has already been printed
  /// earlier on this line.
  void printBetweenInsts(formatted_raw_ostream &OS, bool MustPrint) {
    bool PrintedSomething = false;
    for (unsigned ColIdx = 0, End = ActiveCols.size(); ColIdx < End; ++ColIdx) {
      if (ActiveCols[ColIdx].isActive() && ActiveCols[ColIdx].MustDrawLabel) {
        // First we need to print the live range markers for any active
        // columns to the left of this one.
        OS.PadToColumn(getIndentLevel());
        for (unsigned ColIdx2 = 0; ColIdx2 < ColIdx; ++ColIdx2) {
          if (ActiveCols[ColIdx2].isActive()) {
            if (ActiveCols[ColIdx2].MustDrawLabel &&
                           !ActiveCols[ColIdx2].LiveIn)
              OS << getLineChar(LineChar::LabelVert) << " ";
            else
              OS << getLineChar(LineChar::RangeMid) << " ";
          } else
            OS << "  ";
        }

        // Then print the variable name and location of the new live range,
        // with box drawing characters joining it to the live range line.
        OS << getLineChar(ActiveCols[ColIdx].LiveIn
                              ? LineChar::LabelCornerActive
                              : LineChar::LabelCornerNew)
           << getLineChar(LineChar::LabelHoriz) << " ";
        WithColor(OS, raw_ostream::GREEN)
            << LiveVariables[ActiveCols[ColIdx].VarIdx].VarName;
        OS << " = ";
        {
          WithColor ExprColor(OS, raw_ostream::CYAN);
          LiveVariables[ActiveCols[ColIdx].VarIdx].print(OS, MRI);
        }

        // If there are any columns to the right of the expression we just
        // printed, then continue their live range lines.
        unsigned FirstUnprintedColumn = moveToFirstVarColumn(OS);
        for (unsigned ColIdx2 = FirstUnprintedColumn, End = ActiveCols.size();
             ColIdx2 < End; ++ColIdx2) {
          if (ActiveCols[ColIdx2].isActive() && ActiveCols[ColIdx2].LiveIn)
            OS << getLineChar(LineChar::RangeMid) << " ";
          else
            OS << "  ";
        }

        //OS << "\n"; //degistirdim
        PrintedSomething = true;
      }
    }

    for (unsigned ColIdx = 0, End = ActiveCols.size(); ColIdx < End; ++ColIdx)
      if (ActiveCols[ColIdx].isActive())
        ActiveCols[ColIdx].MustDrawLabel = false;

    // If we must print something (because we printed a line/column number),
    // but don't have any new variables to print, then print a line which
    // just continues any existing live ranges.
    if (MustPrint && !PrintedSomething)
      printAfterOtherLine(OS, false);
  }

  /// Print the live variable ranges to the right of a disassembled instruction.
  void printAfterInst(formatted_raw_ostream &OS) {
    if (!ActiveCols.size())
      return;
    unsigned FirstUnprintedColumn = moveToFirstVarColumn(OS);
    for (unsigned ColIdx = FirstUnprintedColumn, End = ActiveCols.size();
         ColIdx < End; ++ColIdx) {
      if (!ActiveCols[ColIdx].isActive())
        OS << "  ";
      else if (ActiveCols[ColIdx].LiveIn && ActiveCols[ColIdx].LiveOut)
        OS << getLineChar(LineChar::RangeMid) << " ";
      else if (ActiveCols[ColIdx].LiveOut)
        OS << getLineChar(LineChar::RangeStart) << " ";
      else if (ActiveCols[ColIdx].LiveIn)
        OS << getLineChar(LineChar::RangeEnd) << " ";
      else
        llvm_unreachable("var must be live in or out!");
    //////ekleme
    //degistirdim
    //OS << "\b";
    }
  }
};

class SourcePrinter {
protected:
  DILineInfo OldLineInfo;
  const ObjectFile *Obj = nullptr;
  std::unique_ptr<symbolize::LLVMSymbolizer> Symbolizer;
  // File name to file contents of source.
  std::unordered_map<std::string, std::unique_ptr<MemoryBuffer>> SourceCache;
  // Mark the line endings of the cached source.
  std::unordered_map<std::string, std::vector<StringRef>> LineCache;
  // Keep track of missing sources.
  StringSet<> MissingSources;
  // Only emit 'no debug info' warning once.
  bool WarnedNoDebugInfo;

private:
  bool cacheSource(const DILineInfo& LineInfoFile);

  void printLines(formatted_raw_ostream &OS, const DILineInfo &LineInfo,
                  StringRef Delimiter, LiveVariablePrinter &LVP);

  void printSources(formatted_raw_ostream &OS, const DILineInfo &LineInfo,
                    StringRef ObjectFilename, StringRef Delimiter,
                    LiveVariablePrinter &LVP);

public:
  SourcePrinter() = default;
  SourcePrinter(const ObjectFile *Obj, StringRef DefaultArch)
      : Obj(Obj), WarnedNoDebugInfo(false) {
    symbolize::LLVMSymbolizer::Options SymbolizerOpts;
    SymbolizerOpts.PrintFunctions =
        DILineInfoSpecifier::FunctionNameKind::LinkageName;
    SymbolizerOpts.Demangle = Demangle;
    SymbolizerOpts.DefaultArch = std::string(DefaultArch);
    Symbolizer.reset(new symbolize::LLVMSymbolizer(SymbolizerOpts));
  }
  virtual ~SourcePrinter() = default;
  virtual void printSourceLine(formatted_raw_ostream &OS,
                               object::SectionedAddress Address,
                               StringRef ObjectFilename,
                               LiveVariablePrinter &LVP,
                               StringRef Delimiter = "; ");
};

bool SourcePrinter::cacheSource(const DILineInfo &LineInfo) {
  std::unique_ptr<MemoryBuffer> Buffer;
  if (LineInfo.Source) {
    Buffer = MemoryBuffer::getMemBuffer(*LineInfo.Source);
  } else {
    auto BufferOrError = MemoryBuffer::getFile(LineInfo.FileName);
    if (!BufferOrError) {
      if (MissingSources.insert(LineInfo.FileName).second)
        reportWarning("failed to find source " + LineInfo.FileName,
                      Obj->getFileName());
      return false;
    }
    Buffer = std::move(*BufferOrError);
  }
  // Chomp the file to get lines
  const char *BufferStart = Buffer->getBufferStart(),
             *BufferEnd = Buffer->getBufferEnd();
  std::vector<StringRef> &Lines = LineCache[LineInfo.FileName];
  const char *Start = BufferStart;
  for (const char *I = BufferStart; I != BufferEnd; ++I)
    if (*I == '\n') {
      Lines.emplace_back(Start, I - Start - (BufferStart < I && I[-1] == '\r'));
      Start = I + 1;
    }
  if (Start < BufferEnd)
    Lines.emplace_back(Start, BufferEnd - Start);
  SourceCache[LineInfo.FileName] = std::move(Buffer);
  return true;
}

void SourcePrinter::printSourceLine(formatted_raw_ostream &OS,
                                    object::SectionedAddress Address,
                                    StringRef ObjectFilename,
                                    LiveVariablePrinter &LVP,
                                    StringRef Delimiter) {
  if (!Symbolizer)
    return;

  DILineInfo LineInfo = DILineInfo();
  auto ExpectedLineInfo = Symbolizer->symbolizeCode(*Obj, Address);
  std::string ErrorMessage;
  if (!ExpectedLineInfo)
    ErrorMessage = toString(ExpectedLineInfo.takeError());
  else
    LineInfo = *ExpectedLineInfo;

  if (LineInfo.FileName == DILineInfo::BadString) {
    if (!WarnedNoDebugInfo) {
      std::string Warning =
          "failed to parse debug information for " + ObjectFilename.str();
      if (!ErrorMessage.empty())
        Warning += ": " + ErrorMessage;
      reportWarning(Warning, ObjectFilename);
      WarnedNoDebugInfo = true;
    }
  }

  if (PrintLines)
    printLines(OS, LineInfo, Delimiter, LVP);
  if (PrintSource)
    printSources(OS, LineInfo, ObjectFilename, Delimiter, LVP);
  OldLineInfo = LineInfo;
}

void SourcePrinter::printLines(formatted_raw_ostream &OS,
                               const DILineInfo &LineInfo, StringRef Delimiter,
                               LiveVariablePrinter &LVP) {
  bool PrintFunctionName = LineInfo.FunctionName != DILineInfo::BadString &&
                           LineInfo.FunctionName != OldLineInfo.FunctionName;
  if (PrintFunctionName) {
    OS << Delimiter << LineInfo.FunctionName;
    // If demangling is successful, FunctionName will end with "()". Print it
    // only if demangling did not run or was unsuccessful.
    if (!StringRef(LineInfo.FunctionName).endswith("()"))
      OS << "()";
    OS << ":\n";
  }
  if (LineInfo.FileName != DILineInfo::BadString && LineInfo.Line != 0 &&
      (OldLineInfo.Line != LineInfo.Line ||
       OldLineInfo.FileName != LineInfo.FileName || PrintFunctionName)) {
    OS << Delimiter << LineInfo.FileName << ":" << LineInfo.Line;
    LVP.printBetweenInsts(OS, true);
  }
}

void SourcePrinter::printSources(formatted_raw_ostream &OS,
                                 const DILineInfo &LineInfo,
                                 StringRef ObjectFilename, StringRef Delimiter,
                                 LiveVariablePrinter &LVP) {
  if (LineInfo.FileName == DILineInfo::BadString || LineInfo.Line == 0 ||
      (OldLineInfo.Line == LineInfo.Line &&
       OldLineInfo.FileName == LineInfo.FileName))
    return;

  if (SourceCache.find(LineInfo.FileName) == SourceCache.end())
    if (!cacheSource(LineInfo))
      return;
  auto LineBuffer = LineCache.find(LineInfo.FileName);
  if (LineBuffer != LineCache.end()) {
    if (LineInfo.Line > LineBuffer->second.size()) {
      reportWarning(
          formatv(
              "debug info line number {0} exceeds the number of lines in {1}",
              LineInfo.Line, LineInfo.FileName),
          ObjectFilename);
      return;
    }
    // Vector begins at 0, line numbers are non-zero
    OS << Delimiter << LineBuffer->second[LineInfo.Line - 1];
    LVP.printBetweenInsts(OS, true);
  }
}

static bool isAArch64Elf(const ObjectFile *Obj) {
  const auto *Elf = dyn_cast<ELFObjectFileBase>(Obj);
  return Elf && Elf->getEMachine() == ELF::EM_AARCH64;
}

static bool isArmElf(const ObjectFile *Obj) {
  const auto *Elf = dyn_cast<ELFObjectFileBase>(Obj);
  return Elf && Elf->getEMachine() == ELF::EM_ARM;
}

static bool hasMappingSymbols(const ObjectFile *Obj) {
  return isArmElf(Obj) || isAArch64Elf(Obj);
}

static void printRelocation(formatted_raw_ostream &OS, StringRef FileName,
                            const RelocationRef &Rel, uint64_t Address,
                            bool Is64Bits) {
  StringRef Fmt = Is64Bits ? "\t\t%016" PRIx64 ":  " : "\t\t\t%08" PRIx64 ":  ";
  SmallString<16> Name;
  SmallString<32> Val;
  Rel.getTypeName(Name);
  if (Error E = getRelocationValueString(Rel, Val))
    reportError(std::move(E), FileName);
  OS << format(Fmt.data(), Address) << Name << "\t" << Val;
}

// MCInstPrinter.cpp tan port
// neden namespace?
// cunku using namespace llvm; diyince dumpbytes mcinstprinter.h taki 
// dumpbytesa giriyor --> llvm::dumpbytes burada aynısı olamayacağı için hatali oluyor
// ya yeni namespace tanımlayacaktım ya da işe yaramaz fazladan parametre ekleyip override etmiş olacaktım.
// namespaceler override niyetine çok iyi bee...
/*
  namespace dosya{
    string filename = string(g_argv[2]);
    fstream OSS(filename.substr(filename.find('.')) + ".hex");
  }
*/

////////////////// ters basmak için reverse class templatei
template <typename C>
struct reverse_wrapper {

    C & c_;
    reverse_wrapper(C & c) :  c_(c) {}

    typename C::reverse_iterator begin() {return c_.rbegin();}
    typename C::reverse_iterator end() {return c_.rend(); }
};

template <typename C, size_t N>
struct reverse_wrapper< C[N] >{

    C (&c_)[N];
    reverse_wrapper( C(&c)[N] ) : c_(c) {}

    typename std::reverse_iterator<const C *> begin() { return std::rbegin(c_); }
    typename std::reverse_iterator<const C *> end() { return std::rend(c_); }
};


template <typename C>
reverse_wrapper<C> r_wrap(C & c) {
    return reverse_wrapper<C>(c);
}
///////////////////

using namespace std;




    //string filename = "out";//= string(g_argv[2]);
    //StringRef filename = g_argv[2];
    //fstream OSS(filename + ".hex", fstream::in | fstream::out | fstream::trunc);//"out" + ".hex"
    //fstream OSS(filename.substr(filename.find('.')) + ".hex", fstream::in | fstream::out | fstream::trunc);
    //OSS(filename.substr(filename.find('.')) + ".hex");
    fstream OSS("out.hex", fstream::in | fstream::out | fstream::trunc);
bool flag = false;
namespace porteddump {
 void dumpBytes(ArrayRef<uint8_t> bytes, raw_ostream &OS, bool encrypt) {
//add = false; sub = false;
  //if(!dosya.empty() && flag){ flag = false;
  //  ifstream dos(dosya);
  //  string option = "";
  //  char character;
  //  if (dos.is_open()){
  //    while(dos >> std::noskipws >> character){
  //      //if(character='a') add = true;
  //      if(character != ' ' || character != '\n')
  //        option += string(1,character);
  //      else {
  //        if( option == "add"                ){ add = true; option = ""; } //else sub=true;
  //        if( option == "sub"                ){ sub = true; option = ""; }
  //      }
  //    }
  //  }
  //}
//int bite = atoi(bits.c_str());
//int array[2];
//for (int i = 1; i >= 0; i--) {
//    array[i] = bite % 10;
//    bite /= 10;
//}

//if(array[0] == 1) add = true;
//if(array[1] == 1) sub = true;

   static const char hex_rep[] = "0123456789abcdef";
   bool First = true;
   for (char i: r_wrap(bytes)) { //OS<<hex_rep[(i & 0xF0) >> 4]<<"\n"; if(!dosya.empty()) OS<<dosya; //OS<<hex_rep[i & 0xF]<<"\n"; //ekledim
     if (First)
       First = false;
     else
       OSS << ' '; //i = i xor 255;

    
/////////////ek
//if(add && sub && flag) { i = i xor 125; flag = false;}
//     if(add && flag) { i = i xor 255; flag = false;} //ff 
//     if(sub && flag) { i = i xor 170; flag = false;} //aa
     

     if(encrypt) i = i xor 170;
     //if(add) { i = i xor 255;} //ff 
     //if(sub) { i = i xor 170;} //aa
     OSS << hex_rep[(i & 0xF0) >> 4];
     OSS << hex_rep[i & 0xF];
   } 
 }
}
class PrettyPrinter {
public:
  virtual ~PrettyPrinter() = default;
  virtual void // bu printinste giriyor.
  printInst(MCInstPrinter &IP, const MCInst *MI, ArrayRef<uint8_t> Bytes,
            object::SectionedAddress Address, formatted_raw_ostream &OS,
            StringRef Annot, MCSubtargetInfo const &STI, SourcePrinter *SP,
            StringRef ObjectFilename, std::vector<RelocationRef> *Rels,
            LiveVariablePrinter &LVP, const MCRegisterInfo &MRI) { // buraya fazladan MRI parametresi verelim sonra printInste oradan da uncompressInste 
    if (SP && (PrintSource || PrintLines))
      SP->printSourceLine(OS, Address, ObjectFilename, LVP);
    LVP.printBetweenInsts(OS, false);

    size_t Start = OS.tell();
    //degistirdim
    //if (!NoLeadingAddr)
    //  OS << format("%8" PRIx64 ":", Address.Address);

int bitArr[90];
for(int i=89;i>=0;i--){
  bitArr[i] = 0;
}

for(int i=0;i<bits.length();i++){
  bitArr[i] = bits[i] - '0';
}

// hepsini baslangicta falsea esitlemeyi dene ya da else kullan.

if(bitArr[0 ] == 1) beq                = true;
if(bitArr[1 ] == 1) bne                = true;
if(bitArr[2 ] == 1) blt                = true;
if(bitArr[3 ] == 1) bge                = true;
if(bitArr[4 ] == 1) bltu               = true;
if(bitArr[5 ] == 1) bgeu               = true;
if(bitArr[6 ] == 1) jalr               = true;
if(bitArr[7 ] == 1) jal                = true;
if(bitArr[8 ] == 1) lui                = true;
if(bitArr[9 ] == 1) auipc              = true;
if(bitArr[10] == 1) addi               = true;
if(bitArr[11] == 1) slli               = true;
if(bitArr[12] == 1) slti               = true;
if(bitArr[13] == 1) sltiu              = true;
if(bitArr[14] == 1) xori               = true;
if(bitArr[15] == 1) srli               = true;
if(bitArr[16] == 1) srai               = true;
if(bitArr[17] == 1) ori                = true;
if(bitArr[18] == 1) andi               = true;
if(bitArr[19] == 1) add                = true;
if(bitArr[20] == 1) sub                = true;
if(bitArr[21] == 1) sll                = true;
if(bitArr[22] == 1) slt                = true;
if(bitArr[23] == 1) sltu               = true;
if(bitArr[24] == 1) xor_               = true; 
if(bitArr[25] == 1) srl                = true;
if(bitArr[26] == 1) sra                = true;
if(bitArr[27] == 1) or_                = true; 
if(bitArr[28] == 1) and_               = true; 
if(bitArr[29] == 1) addiw              = true;
if(bitArr[30] == 1) slliw              = true;
if(bitArr[31] == 1) srliw              = true;
if(bitArr[32] == 1) sraiw              = true;
if(bitArr[33] == 1) addw               = true;
if(bitArr[34] == 1) subw               = true;
if(bitArr[35] == 1) sllw               = true;
if(bitArr[36] == 1) srlw               = true;
if(bitArr[37] == 1) sraw               = true;
if(bitArr[38] == 1) lb                 = true;
if(bitArr[39] == 1) lh                 = true;
if(bitArr[40] == 1) lw                 = true;
if(bitArr[41] == 1) ld                 = true;
if(bitArr[42] == 1) lbu                = true;
if(bitArr[43] == 1) lhu                = true;
if(bitArr[44] == 1) lwu                = true;
if(bitArr[45] == 1) sb                 = true;
if(bitArr[46] == 1) sh                 = true;
if(bitArr[47] == 1) sw                 = true;
if(bitArr[48] == 1) sd                 = true;
if(bitArr[49] == 1) fence              = true;
if(bitArr[50] == 1) fence_i            = true;
if(bitArr[51] == 1) mul                = true;
if(bitArr[52] == 1) mulh               = true;
if(bitArr[53] == 1) mulhsu             = true;
if(bitArr[54] == 1) mulhu              = true;
if(bitArr[55] == 1) objdump::div       = true; // div is ambigious hatasindan dolayi
if(bitArr[56] == 1) divu               = true;
if(bitArr[57] == 1) rem                = true;
if(bitArr[58] == 1) remu               = true;
if(bitArr[59] == 1) mulw               = true;
if(bitArr[60] == 1) divw               = true;
if(bitArr[61] == 1) divuw              = true;
if(bitArr[62] == 1) remw               = true;
if(bitArr[63] == 1) remuw              = true;
if(bitArr[64] == 1) lr_w               = true;
if(bitArr[65] == 1) sc_w               = true;
if(bitArr[66] == 1) lr_d               = true;
if(bitArr[67] == 1) sc_d               = true;
if(bitArr[68] == 1) ecall              = true;
if(bitArr[69] == 1) ebreak             = true;
if(bitArr[70] == 1) uret               = true;
if(bitArr[71] == 1) mret               = true;
if(bitArr[72] == 1) dret               = true;
if(bitArr[73] == 1) sfence_vma         = true;
if(bitArr[74] == 1) wfi                = true;
if(bitArr[75] == 1) csrrw              = true;
if(bitArr[76] == 1) csrrs              = true;
if(bitArr[77] == 1) csrrc              = true;
if(bitArr[78] == 1) csrrwi             = true;
if(bitArr[79] == 1) csrrsi             = true;
if(bitArr[80] == 1) csrrci             = true;
if(bitArr[81] == 1) slli_rv32          = true;
if(bitArr[82] == 1) srli_rv32          = true;
if(bitArr[83] == 1) srai_rv32          = true;
if(bitArr[84] == 1) rdcycle            = true;
if(bitArr[85] == 1) rdtime             = true;
if(bitArr[86] == 1) rdinstret          = true;
if(bitArr[87] == 1) rdcycleh           = true;
if(bitArr[88] == 1) rdtimeh            = true;
if(bitArr[89] == 1) rdinstreth         = true;


    if (!NoShowRawInsn) {
      //OS << ' ';
      bool encrypt = false;

      //uint64_t Addr =
      //  Address.Address + (STI.getTargetTriple().isX86() ? Bytes.size() : 0);
//
      //llvm::StringRef opcode = portedprint::printInst(MI, Addr, "", STI, OS, MRI);

      if     (beq          && (MI->getOpcode() == RISCV::BEQ  || MI->getOpcode() == RISCV::C_BEQZ)) encrypt = true;
      else if(bne          && (MI->getOpcode() == RISCV::BNE  || MI->getOpcode() == RISCV::C_BNEZ)) encrypt = true;
      else if(blt          && (MI->getOpcode() == RISCV::BLT                                     )) encrypt = true;     
      else if(bge          && (MI->getOpcode() == RISCV::BGE                                     )) encrypt = true;     
      else if(bltu         && (MI->getOpcode() == RISCV::BLTU                                    )) encrypt = true;     
      else if(bgeu         && (MI->getOpcode() == RISCV::BGEU                                    )) encrypt = true;     
      else if(jalr         && (MI->getOpcode() == RISCV::JALR || MI->getOpcode() == RISCV::C_JALR)) encrypt = true;     
      else if(jal          && (MI->getOpcode() == RISCV::JAL  || MI->getOpcode() == RISCV::C_JAL )) encrypt = true;     
      else if(lui          && (MI->getOpcode() == RISCV::LUI  || MI->getOpcode() == RISCV::C_LUI || MI->getOpcode() == RISCV::C_LUI_HINT)) encrypt = true;     
      else if(auipc        && (MI->getOpcode() == RISCV::AUIPC                                   )) encrypt = true;     
      else if(addi         && (MI->getOpcode() == RISCV::ADDI  || MI->getOpcode() == RISCV::C_ADDI || MI->getOpcode() == RISCV::C_ADDI16SP || MI->getOpcode() == RISCV::C_ADDI4SPN || MI->getOpcode() == RISCV::C_ADDI_HINT_X0 || MI->getOpcode() == RISCV::C_ADDI_NOP || MI->getOpcode() == RISCV::C_ADDI_HINT_IMM_ZERO)) encrypt = true; 
      else if(slli         && (MI->getOpcode() == RISCV::SLLI  || MI->getOpcode() == RISCV::C_SLLI || MI->getOpcode() == RISCV::C_SLLI64_HINT || MI->getOpcode() == RISCV::C_SLLI_HINT)) encrypt = true;     
      else if(slti         && (MI->getOpcode() == RISCV::SLTI                                    ))  encrypt = true;     
      else if(sltiu        && (MI->getOpcode() == RISCV::SLTIU                                   )) encrypt = true;     
      else if(xori         && (MI->getOpcode() == RISCV::XORI                                    )) encrypt = true;     
      else if(srli         && (MI->getOpcode() == RISCV::SRLI  || MI->getOpcode() == RISCV::C_SRLI || MI->getOpcode() == RISCV::C_SRLI64_HINT)) encrypt = true;     
      else if(srai         && (MI->getOpcode() == RISCV::SRAI  || MI->getOpcode() == RISCV::C_SRAI || MI->getOpcode() == RISCV::C_SRAI64_HINT)) encrypt = true;     
      else if(ori          && (MI->getOpcode() == RISCV::ORI                                     )) encrypt = true;     
      else if(andi         && (MI->getOpcode() == RISCV::ANDI || MI->getOpcode() == RISCV::C_ANDI)) encrypt = true;     
      else if(add          && (MI->getOpcode() == RISCV::ADD  || MI->getOpcode() == RISCV::C_ADD || MI->getOpcode() == RISCV::C_ADD_HINT)) encrypt = true;     
      else if(sub          && (MI->getOpcode() == RISCV::SUB  || MI->getOpcode() == RISCV::C_SUB || MI->getOpcode() == RISCV::G_SUB)) encrypt = true;     
      else if(sll          && (MI->getOpcode() == RISCV::SLL                                     )) encrypt = true;     
      else if(slt          && (MI->getOpcode() == RISCV::SLT                                     )) encrypt = true;     
      else if(sltu         && (MI->getOpcode() == RISCV::SLTU                                    )) encrypt = true;     
      else if(xor_         && (MI->getOpcode() == RISCV::XOR || MI->getOpcode() == RISCV::C_XOR || MI->getOpcode() == RISCV::G_XOR || MI->getOpcode() == RISCV::G_ATOMICRMW_XOR)) encrypt = true;     
      else if(srl          && (MI->getOpcode() == RISCV::SRL                                     )) encrypt = true;     
      else if(sra          && (MI->getOpcode() == RISCV::SRA                                     )) encrypt = true;     
      else if(or_          && (MI->getOpcode() == RISCV::OR  || MI->getOpcode() == RISCV::ORN || MI->getOpcode() == RISCV::C_OR || MI->getOpcode() == RISCV::G_OR || MI->getOpcode() == RISCV::G_ATOMICRMW_OR)) encrypt = true;     
      else if(and_         && (MI->getOpcode() == RISCV::AND || MI->getOpcode() == RISCV::ANDN || MI->getOpcode() == RISCV::C_AND || MI->getOpcode() == RISCV::G_AND || MI->getOpcode() == RISCV::G_ATOMICRMW_AND)) encrypt = true; //amoandleri almadim
      else if(addiw        && (MI->getOpcode() == RISCV::ADDIW || MI->getOpcode() == RISCV::C_ADDIW )) encrypt = true;  //c_addiwu yu almadim   
      else if(slliw        && (MI->getOpcode() == RISCV::SLLIW                                   )) encrypt = true; // slliuw yi almadim   
      else if(srliw        && (MI->getOpcode() == RISCV::SRLIW                                   )) encrypt = true;     
      else if(sraiw        && (MI->getOpcode() == RISCV::SRAIW                                   )) encrypt = true;     
      else if(addw         && (MI->getOpcode() == RISCV::ADDW || MI->getOpcode() == RISCV::C_ADDW )) encrypt = true; // amolari ve adduw addwu yu almadim  
      else if(subw         && (MI->getOpcode() == RISCV::SUBW || MI->getOpcode() == RISCV::C_SUBW )) encrypt = true; // subwu ve subuw u almadim     
      else if(sllw         && (MI->getOpcode() == RISCV::SLLW                                    )) encrypt = true;     
      else if(srlw         && (MI->getOpcode() == RISCV::SRLW                                    )) encrypt = true;     
      else if(sraw         && (MI->getOpcode() == RISCV::SRAW                                    )) encrypt = true;     
      else if(lb           && (MI->getOpcode() == RISCV::LB   || MI->getOpcode() == RISCV::PseudoLB)) encrypt = true;     
      else if(lh           && (MI->getOpcode() == RISCV::LH   || MI->getOpcode() == RISCV::PseudoLH)) encrypt = true;     
      else if(lw           && (MI->getOpcode() == RISCV::LW   || MI->getOpcode() == RISCV::C_LW || MI->getOpcode() == RISCV::C_LWSP || MI->getOpcode() == RISCV::PseudoLW)) encrypt = true;     
      else if(ld           && (MI->getOpcode() == RISCV::LD   || MI->getOpcode() == RISCV::C_LD || MI->getOpcode() == RISCV::C_LDSP || MI->getOpcode() == RISCV::PseudoLD)) encrypt = true;     
      else if(lbu          && (MI->getOpcode() == RISCV::LBU  || MI->getOpcode() == RISCV::PseudoLBU)) encrypt = true;     
      else if(lhu          && (MI->getOpcode() == RISCV::LHU  || MI->getOpcode() == RISCV::PseudoLHU)) encrypt = true;     
      else if(lwu          && (MI->getOpcode() == RISCV::LWU  || MI->getOpcode() == RISCV::PseudoLWU)) encrypt = true;     
      else if(sb           && (MI->getOpcode() == RISCV::SB   || MI->getOpcode() == RISCV::PseudoSB)) encrypt = true; // sbde daha cok var ama almadim    
      else if(sh           && (MI->getOpcode() == RISCV::SH  || MI->getOpcode() == RISCV::PseudoSH)) encrypt = true;     
      else if(sw           && (MI->getOpcode() == RISCV::SW  || MI->getOpcode() == RISCV::C_SW || MI->getOpcode() == RISCV::C_SWSP || MI->getOpcode() == RISCV::PseudoSW)) encrypt = true;     
      else if(sd           && (MI->getOpcode() == RISCV::SD  || MI->getOpcode() == RISCV::C_SD || MI->getOpcode() == RISCV::C_SDSP || MI->getOpcode() == RISCV::G_SDIV || MI->getOpcode() == RISCV::PseudoSD)) encrypt = true;     
      else if(fence        && (MI->getOpcode() == RISCV::FENCE || MI->getOpcode() == RISCV::FENCE_TSO || MI->getOpcode() == RISCV::G_FENCE)) encrypt = true;     
      else if(fence_i      && (MI->getOpcode() == RISCV::FENCE_I                                 )) encrypt = true;     
      else if(mul          && (MI->getOpcode() == RISCV::MUL   || MI->getOpcode() == RISCV::G_MUL)) encrypt = true;     
      else if(mulh         && (MI->getOpcode() == RISCV::MULH                                    )) encrypt = true;     
      else if(mulhsu       && (MI->getOpcode() == RISCV::MULHSU                                  )) encrypt = true;     
      else if(mulhu        && (MI->getOpcode() == RISCV::MULHU                                   )) encrypt = true;     
      else if(objdump::div && (MI->getOpcode() == RISCV::DIV                                     )) encrypt = true;     
      else if(divu         && (MI->getOpcode() == RISCV::DIVU                                    )) encrypt = true;     
      else if(rem          && (MI->getOpcode() == RISCV::REM                                     )) encrypt = true;     
      else if(remu         && (MI->getOpcode() == RISCV::REMU                                    )) encrypt = true;     
      else if(mulw         && (MI->getOpcode() == RISCV::MULW                                    )) encrypt = true;     
      else if(divw         && (MI->getOpcode() == RISCV::DIVW                                    )) encrypt = true;     
      else if(divuw        && (MI->getOpcode() == RISCV::DIVUW                                   )) encrypt = true;     
      else if(remw         && (MI->getOpcode() == RISCV::REMW                                    )) encrypt = true;     
      else if(remuw        && (MI->getOpcode() == RISCV::REMUW                                   )) encrypt = true;     
      else if(lr_w         && (MI->getOpcode() == RISCV::LR_W  || MI->getOpcode() == RISCV::LR_W_AQ || MI->getOpcode() == RISCV::LR_W_AQ_RL || MI->getOpcode() == RISCV::LR_W_RL)) encrypt = true;     
      else if(sc_w         && (MI->getOpcode() == RISCV::SC_W  || MI->getOpcode() == RISCV::SC_W_AQ || MI->getOpcode() == RISCV::SC_W_AQ_RL || MI->getOpcode() == RISCV::SC_W_RL)) encrypt = true;     
      else if(lr_d         && (MI->getOpcode() == RISCV::LR_D  || MI->getOpcode() == RISCV::LR_D_AQ || MI->getOpcode() == RISCV::LR_D_AQ_RL || MI->getOpcode() == RISCV::LR_D_RL)) encrypt = true;     
      else if(sc_d         && (MI->getOpcode() == RISCV::SC_D  || MI->getOpcode() == RISCV::SC_D_AQ || MI->getOpcode() == RISCV::SC_D_AQ_RL || MI->getOpcode() == RISCV::SC_D_RL)) encrypt = true;     
      else if(ecall        && (MI->getOpcode() == RISCV::ECALL                                   )) encrypt = true;     
      else if(ebreak       && (MI->getOpcode() == RISCV::EBREAK || MI->getOpcode() == RISCV::C_EBREAK)) encrypt = true;     
      else if(uret         && (MI->getOpcode() == RISCV::URET                                    )) encrypt = true;     
      else if(mret         && (MI->getOpcode() == RISCV::MRET                                    )) encrypt = true;     
      else if(dret         && (MI->getOpcode() == RISCV::DRET                                    )) encrypt = true;     
      else if(sfence_vma   && (MI->getOpcode() == RISCV::SFENCE_VMA                              )) encrypt = true;     
      else if(wfi          && (MI->getOpcode() == RISCV::WFI                                     )) encrypt = true;     
      else if(csrrw        && (MI->getOpcode() == RISCV::CSRRW                                   )) encrypt = true;     
      else if(csrrs        && (MI->getOpcode() == RISCV::CSRRS                                   )) encrypt = true;     
      else if(csrrc        && (MI->getOpcode() == RISCV::CSRRC                                   )) encrypt = true;     
      else if(csrrwi       && (MI->getOpcode() == RISCV::CSRRWI                                  )) encrypt = true;     
      else if(csrrsi       && (MI->getOpcode() == RISCV::CSRRSI                                  )) encrypt = true;     
      else if(csrrci       && (MI->getOpcode() == RISCV::CSRRCI                                  )) encrypt = true;     
      //lse if(slli_rv32    && (MI->getOpcode() == RISCV::                                   )) encrypt = true;     
      //lse if(srli_rv32    && opcodeList[82] == opcode) encrypt = true;     
      //lse if(srai_rv32    && opcodeList[83] == opcode) encrypt = true;     
      //lse if(rdcycle      && opcodeList[84] == opcode) encrypt = true;     
      //lse if(rdtime       && opcodeList[85] == opcode) encrypt = true;     
      //lse if(rdinstret    && opcodeList[86] == opcode) encrypt = true;     
      //lse if(rdcycleh     && opcodeList[87] == opcode) encrypt = true;     
      //lse if(rdtimeh      && opcodeList[88] == opcode) encrypt = true;     
      //lse if(rdinstreth   && opcodeList[89] == opcode) encrypt = true;  
      porteddump::dumpBytes(Bytes, OS, encrypt); // hexi burada yaziyor
    }

    // The output of printInst starts with a tab. Print some spaces so that
    // the tab has 1 column and advances to the target tab stop.
    unsigned TabStop = getInstStartColumn(STI);
    unsigned Column = OS.tell() - Start;
    OS.indent(Column < TabStop - 1 ? TabStop - 1 - Column : 7 - Column % 8);
//////////////////////////////////////////////////////////////////////////////////////////
string a = std::to_string( MI->getOpcode() ) ; // unsigned int to string
//if(MI->getOpcode() == 371) {OS<<"addiiiii"; flag = true;}
//Instruction::getOpcodeName(MI->getOpcode());
//OS<<a;
StringRef b = IP.getOpcodeName(MI->getOpcode());
//OS<<MCInstPrinter::getOpcodeName(MI->getOpcode()); // ekledim
//if(MI->getOpcode() == RISCV::ADDI)
OS<<"\n";
//OS<<b;
//if(MI->getOpcode() == RISCV::SW) OS<<b;//"basarili";//IP.getOpcodeName(MI->getOpcode())"";
//if(addi && MI->getOpcode() == RISCV::C_ADDI4SPN        /*&& opcodeList[10] == opcode*/) OS<<"GIRDIaddi";//encrypt = true;   
//////////////////////////////////////////////////7



    if (MI) { // degistirdim
      // See MCInstPrinter::printInst. On targets where a PC relative immediate
      // is relative to the next instruction and the length of a MCInst is
      // difficult to measure (x86), this is the address of the next
      // instruction.
      uint64_t Addr =
          Address.Address + (STI.getTargetTriple().isX86() ? Bytes.size() : 0);
      //IP.printInst(MI, Addr, "", STI, OS);
      //portedprint::printInstruction(MI, Addr, STI, OS);//ekledim
      portedprint::printInst(MI, Addr, "", STI, OS, MRI); //buraya fazladan MRI parametresi
    } //else
      //OS << "\t<unknown>";
  }
};
PrettyPrinter PrettyPrinterInst;

class HexagonPrettyPrinter : public PrettyPrinter {
public:
  void printLead(ArrayRef<uint8_t> Bytes, uint64_t Address,
                 formatted_raw_ostream &OS) { OS<<"deneme"; // buraya girmiyor demek ki
    uint32_t opcode =
      (Bytes[3] << 24) | (Bytes[2] << 16) | (Bytes[1] << 8) | Bytes[0];
    //if (!NoLeadingAddr)
    //  OS << format("%8" PRIx64 ":", Address);
    if (!NoShowRawInsn) {
      OS << "\t";
      //dumpBytes(Bytes.slice(0, 4), OS);
      //OS << format("\t%08" PRIx32, opcode);
      //degistirdim
      OS << opcode;
    }
  }
  //void printInst(MCInstPrinter &IP, const MCInst *MI, ArrayRef<uint8_t> Bytes,
  //               object::SectionedAddress Address, formatted_raw_ostream &OS,
  //               StringRef Annot, MCSubtargetInfo const &STI, SourcePrinter *SP,
  //               StringRef ObjectFilename, std::vector<RelocationRef> *Rels,
  //               LiveVariablePrinter &LVP) override {
  //  if (SP && (PrintSource || PrintLines))
  //    SP->printSourceLine(OS, Address, ObjectFilename, LVP, "");
  //  if (!MI) {
  //    printLead(Bytes, Address.Address, OS);
  //    //OS << " <unknown>";
  //    return;
  //  }
  //  std::string Buffer;
  //  {
  //    raw_string_ostream TempStream(Buffer);
  //    IP.printInst(MI, Address.Address, "", STI, TempStream);
  //  }
  //  StringRef Contents(Buffer);
  //  // Split off bundle attributes
  //  auto PacketBundle = Contents.rsplit('\n');
  //  // Split off first instruction from the rest
  //  auto HeadTail = PacketBundle.first.split('\n');
  //  auto Preamble = " { ";
  //  auto Separator = "";
//
  //  // Hexagon's packets require relocations to be inline rather than
  //  // clustered at the end of the packet.
  //  std::vector<RelocationRef>::const_iterator RelCur = Rels->begin();
  //  std::vector<RelocationRef>::const_iterator RelEnd = Rels->end();
  //  auto PrintReloc = [&]() -> void {
  //    while ((RelCur != RelEnd) && (RelCur->getOffset() <= Address.Address)) {
  //      if (RelCur->getOffset() == Address.Address) {
  //        printRelocation(OS, ObjectFilename, *RelCur, Address.Address, false);
  //        return;
  //      }
  //      ++RelCur;
  //    }
  //  };
//
  //  while (!HeadTail.first.empty()) {
  //    OS << Separator;
  //    Separator = "\n";
  //    if (SP && (PrintSource || PrintLines))
  //      SP->printSourceLine(OS, Address, ObjectFilename, LVP, "");
  //    printLead(Bytes, Address.Address, OS);
  //    OS << Preamble;
  //    Preamble = "   ";
  //    StringRef Inst;
  //    auto Duplex = HeadTail.first.split('\v');
  //    if (!Duplex.second.empty()) {
  //      OS << Duplex.first;
  //      OS << "; ";
  //      Inst = Duplex.second;
  //    }
  //    else
  //      Inst = HeadTail.first;
  //    OS << Inst;
  //    HeadTail = HeadTail.second.split('\n');
  //    if (HeadTail.first.empty())
  //      OS << " } " << PacketBundle.second;
  //    PrintReloc();
  //    Bytes = Bytes.slice(4);
  //    Address.Address += 4;
  //  }
  //}
};
HexagonPrettyPrinter HexagonPrettyPrinterInst;

class AMDGCNPrettyPrinter : public PrettyPrinter {
public:
  //void printInst(MCInstPrinter &IP, const MCInst *MI, ArrayRef<uint8_t> Bytes,
  //               object::SectionedAddress Address, formatted_raw_ostream &OS,
  //               StringRef Annot, MCSubtargetInfo const &STI, SourcePrinter *SP,
  //               StringRef ObjectFilename, std::vector<RelocationRef> *Rels,
  //               LiveVariablePrinter &LVP) override {
  //  if (SP && (PrintSource || PrintLines))
  //    SP->printSourceLine(OS, Address, ObjectFilename, LVP);
//
  //  if (MI) {
  //    SmallString<40> InstStr;
  //    raw_svector_ostream IS(InstStr);
//
  //    IP.printInst(MI, Address.Address, "", STI, IS);
//
  //    OS << left_justify(IS.str(), 60);
  //  } else {
  //    // an unrecognized encoding - this is probably data so represent it
  //    // using the .long directive, or .byte directive if fewer than 4 bytes
  //    // remaining
  //    if (Bytes.size() >= 4) {
  //      OS << format("\t.long 0x%08" PRIx32 " ",
  //                   support::endian::read32<support::little>(Bytes.data()));
  //      OS.indent(42);
  //    } else {
  //        OS << format("\t.byte 0x%02" PRIx8, Bytes[0]);
  //        for (unsigned int i = 1; i < Bytes.size(); i++)
  //          OS << format(", 0x%02" PRIx8, Bytes[i]);
  //        OS.indent(55 - (6 * Bytes.size()));
  //    }
  //  }
//
  //  OS << format("// %012" PRIX64 ":", Address.Address);
  //  if (Bytes.size() >= 4) {
  //    // D should be casted to uint32_t here as it is passed by format to
  //    // snprintf as vararg.
  //    for (uint32_t D : makeArrayRef(
  //             reinterpret_cast<const support::little32_t *>(Bytes.data()),
  //             Bytes.size() / 4))
  //      OS << format(" %08" PRIX32, D);
  //  } else {
  //    for (unsigned char B : Bytes)
  //      OS << format(" %02" PRIX8, B);
  //  }
//
  //  if (!Annot.empty())
  //    OS << " // " << Annot;
  //}
};
AMDGCNPrettyPrinter AMDGCNPrettyPrinterInst;

class BPFPrettyPrinter : public PrettyPrinter {
public:
  //void printInst(MCInstPrinter &IP, const MCInst *MI, ArrayRef<uint8_t> Bytes,
  //               object::SectionedAddress Address, formatted_raw_ostream &OS,
  //               StringRef Annot, MCSubtargetInfo const &STI, SourcePrinter *SP,
  //               StringRef ObjectFilename, std::vector<RelocationRef> *Rels,
  //               LiveVariablePrinter &LVP) override {
  //  if (SP && (PrintSource || PrintLines))
  //    SP->printSourceLine(OS, Address, ObjectFilename, LVP);//degistirdim
  //  //if (!NoLeadingAddr)
  //  //  OS << format("%8" PRId64 ":", Address.Address / 8);
  //  if (!NoShowRawInsn) {
  //    OS << "\t";
  //    //dumpBytes(Bytes, OS);
  //  }
  //  if (MI)
  //    IP.printInst(MI, Address.Address, "", STI, OS);
  //  //else
  //  //  OS << "\t<unknown>";
  //}
};
BPFPrettyPrinter BPFPrettyPrinterInst;

PrettyPrinter &selectPrettyPrinter(Triple const &Triple) {
  switch(Triple.getArch()) {
  default:
    return PrettyPrinterInst;
  case Triple::hexagon:
    return HexagonPrettyPrinterInst;
  case Triple::amdgcn:
    return AMDGCNPrettyPrinterInst;
  case Triple::bpfel:
  case Triple::bpfeb:
    return BPFPrettyPrinterInst;
  }
}
}

static uint8_t getElfSymbolType(const ObjectFile *Obj, const SymbolRef &Sym) {
  assert(Obj->isELF());
  if (auto *Elf32LEObj = dyn_cast<ELF32LEObjectFile>(Obj))
    return Elf32LEObj->getSymbol(Sym.getRawDataRefImpl())->getType();
  if (auto *Elf64LEObj = dyn_cast<ELF64LEObjectFile>(Obj))
    return Elf64LEObj->getSymbol(Sym.getRawDataRefImpl())->getType();
  if (auto *Elf32BEObj = dyn_cast<ELF32BEObjectFile>(Obj))
    return Elf32BEObj->getSymbol(Sym.getRawDataRefImpl())->getType();
  if (auto *Elf64BEObj = cast<ELF64BEObjectFile>(Obj))
    return Elf64BEObj->getSymbol(Sym.getRawDataRefImpl())->getType();
  llvm_unreachable("Unsupported binary format");
}

template <class ELFT> static void
addDynamicElfSymbols(const ELFObjectFile<ELFT> *Obj,
                     std::map<SectionRef, SectionSymbolsTy> &AllSymbols) {
  for (auto Symbol : Obj->getDynamicSymbolIterators()) {
    uint8_t SymbolType = Symbol.getELFType();
    if (SymbolType == ELF::STT_SECTION)
      continue;

    uint64_t Address = unwrapOrError(Symbol.getAddress(), Obj->getFileName());
    // ELFSymbolRef::getAddress() returns size instead of value for common
    // symbols which is not desirable for disassembly output. Overriding.
    if (SymbolType == ELF::STT_COMMON)
      Address = Obj->getSymbol(Symbol.getRawDataRefImpl())->st_value;

    StringRef Name = unwrapOrError(Symbol.getName(), Obj->getFileName());
    if (Name.empty())
      continue;

    section_iterator SecI =
        unwrapOrError(Symbol.getSection(), Obj->getFileName());
    if (SecI == Obj->section_end())
      continue;

    AllSymbols[*SecI].emplace_back(Address, Name, SymbolType);
  }
}

static void
addDynamicElfSymbols(const ObjectFile *Obj,
                     std::map<SectionRef, SectionSymbolsTy> &AllSymbols) {
  assert(Obj->isELF());
  if (auto *Elf32LEObj = dyn_cast<ELF32LEObjectFile>(Obj))
    addDynamicElfSymbols(Elf32LEObj, AllSymbols);
  else if (auto *Elf64LEObj = dyn_cast<ELF64LEObjectFile>(Obj))
    addDynamicElfSymbols(Elf64LEObj, AllSymbols);
  else if (auto *Elf32BEObj = dyn_cast<ELF32BEObjectFile>(Obj))
    addDynamicElfSymbols(Elf32BEObj, AllSymbols);
  else if (auto *Elf64BEObj = cast<ELF64BEObjectFile>(Obj))
    addDynamicElfSymbols(Elf64BEObj, AllSymbols);
  else
    llvm_unreachable("Unsupported binary format");
}

static void addPltEntries(const ObjectFile *Obj,
                          std::map<SectionRef, SectionSymbolsTy> &AllSymbols,
                          StringSaver &Saver) {
  Optional<SectionRef> Plt = None;
  for (const SectionRef &Section : Obj->sections()) {
    Expected<StringRef> SecNameOrErr = Section.getName();
    if (!SecNameOrErr) {
      consumeError(SecNameOrErr.takeError());
      continue;
    }
    if (*SecNameOrErr == ".plt")
      Plt = Section;
  }
  if (!Plt)
    return;
  if (auto *ElfObj = dyn_cast<ELFObjectFileBase>(Obj)) {
    for (auto PltEntry : ElfObj->getPltAddresses()) {
      SymbolRef Symbol(PltEntry.first, ElfObj);
      uint8_t SymbolType = getElfSymbolType(Obj, Symbol);

      StringRef Name = unwrapOrError(Symbol.getName(), Obj->getFileName());
      if (!Name.empty())
        AllSymbols[*Plt].emplace_back(
            PltEntry.second, Saver.save((Name + "@plt").str()), SymbolType);
    }
  }
}

// Normally the disassembly output will skip blocks of zeroes. This function
// returns the number of zero bytes that can be skipped when dumping the
// disassembly of the instructions in Buf.
static size_t countSkippableZeroBytes(ArrayRef<uint8_t> Buf) {
  // Find the number of leading zeroes.
  size_t N = 0;
  while (N < Buf.size() && !Buf[N])
    ++N;

  // We may want to skip blocks of zero bytes, but unless we see
  // at least 8 of them in a row.
  if (N < 8)
    return 0;

  // We skip zeroes in multiples of 4 because do not want to truncate an
  // instruction if it starts with a zero byte.
  return N & ~0x3;
}

// Returns a map from sections to their relocations.
static std::map<SectionRef, std::vector<RelocationRef>>
getRelocsMap(object::ObjectFile const &Obj) {
  std::map<SectionRef, std::vector<RelocationRef>> Ret;
  uint64_t I = (uint64_t)-1;
  for (SectionRef Sec : Obj.sections()) {
    ++I;
    Expected<section_iterator> RelocatedOrErr = Sec.getRelocatedSection();
    if (!RelocatedOrErr)
      reportError(Obj.getFileName(),
                  "section (" + Twine(I) +
                      "): failed to get a relocated section: " +
                      toString(RelocatedOrErr.takeError()));

    section_iterator Relocated = *RelocatedOrErr;
    if (Relocated == Obj.section_end() || !checkSectionFilter(*Relocated).Keep)
      continue;
    std::vector<RelocationRef> &V = Ret[*Relocated];
    for (const RelocationRef &R : Sec.relocations())
      V.push_back(R);
    // Sort relocations by address.
    llvm::stable_sort(V, isRelocAddressLess);
  }
  return Ret;
}

// Used for --adjust-vma to check if address should be adjusted by the
// specified value for a given section.
// For ELF we do not adjust non-allocatable sections like debug ones,
// because they are not loadable.
// TODO: implement for other file formats.
static bool shouldAdjustVA(const SectionRef &Section) {
  const ObjectFile *Obj = Section.getObject();
  if (Obj->isELF())
    return ELFSectionRef(Section).getFlags() & ELF::SHF_ALLOC;
  return false;
}


typedef std::pair<uint64_t, char> MappingSymbolPair;
static char getMappingSymbolKind(ArrayRef<MappingSymbolPair> MappingSymbols,
                                 uint64_t Address) {
  auto It =
      partition_point(MappingSymbols, [Address](const MappingSymbolPair &Val) {
        return Val.first <= Address;
      });
  // Return zero for any address before the first mapping symbol; this means
  // we should use the default disassembly mode, depending on the target.
  if (It == MappingSymbols.begin())
    return '\x00';
  return (It - 1)->second;
}

static uint64_t dumpARMELFData(uint64_t SectionAddr, uint64_t Index,
                               uint64_t End, const ObjectFile *Obj,
                               ArrayRef<uint8_t> Bytes,
                               ArrayRef<MappingSymbolPair> MappingSymbols,
                               raw_ostream &OS) {
  support::endianness Endian =
      Obj->isLittleEndian() ? support::little : support::big; 
      // endian kısmı 
  OS << format("%8" PRIx64 ":\t", SectionAddr + Index);
  if (Index + 4 <= End) {
    //dumpBytes(Bytes.slice(Index, 4), OS);
    OS << "\t.word\t"
           << format_hex(support::endian::read32(Bytes.data() + Index, Endian),
                         10);
    return 4;
  }
  if (Index + 2 <= End) {
    //dumpBytes(Bytes.slice(Index, 2), OS);
    OS << "\t\t.short\t"
           << format_hex(support::endian::read16(Bytes.data() + Index, Endian),
                         6);
    return 2;
  }
  //dumpBytes(Bytes.slice(Index, 1), OS);
  OS << "\t\t.byte\t" << format_hex(Bytes[0], 4);
  return 1;
}

static void dumpELFData(uint64_t SectionAddr, uint64_t Index, uint64_t End,
                        ArrayRef<uint8_t> Bytes) {
  // print out data up to 8 bytes at a time in hex and ascii
  uint8_t AsciiData[9] = {'\0'};
  uint8_t Byte;
  int NumBytes = 0;

  for (; Index < End; ++Index) {
    if (NumBytes == 0)
      outs() << format("%8" PRIx64 ":", SectionAddr + Index);
    Byte = Bytes.slice(Index)[0];
    outs() << format(" %02x", Byte);
    AsciiData[NumBytes] = isPrint(Byte) ? Byte : '.';

    uint8_t IndentOffset = 0;
    NumBytes++;
    if (Index == End - 1 || NumBytes > 8) {
      // Indent the space for less than 8 bytes data.
      // 2 spaces for byte and one for space between bytes
      IndentOffset = 3 * (8 - NumBytes);
      for (int Excess = NumBytes; Excess < 8; Excess++)
        AsciiData[Excess] = '\0';
      NumBytes = 8;
    }
    if (NumBytes == 8) {
      AsciiData[8] = '\0';
      outs() << std::string(IndentOffset, ' ') << "         ";
      outs() << reinterpret_cast<char *>(AsciiData);
      //outs() << '\n'; //degistirdim
      NumBytes = 0;
    }
  }
}

SymbolInfoTy objdump::createSymbolInfo(const ObjectFile *Obj,
                                       const SymbolRef &Symbol) {
  const StringRef FileName = Obj->getFileName();
  const uint64_t Addr = unwrapOrError(Symbol.getAddress(), FileName);
  const StringRef Name = unwrapOrError(Symbol.getName(), FileName);

  if (Obj->isXCOFF() && SymbolDescription) {
    const auto *XCOFFObj = cast<XCOFFObjectFile>(Obj);
    DataRefImpl SymbolDRI = Symbol.getRawDataRefImpl();

    const uint32_t SymbolIndex = XCOFFObj->getSymbolIndex(SymbolDRI.p);
    Optional<XCOFF::StorageMappingClass> Smc =
        getXCOFFSymbolCsectSMC(XCOFFObj, Symbol);
    return SymbolInfoTy(Addr, Name, Smc, SymbolIndex,
                        isLabel(XCOFFObj, Symbol));
  } else
    return SymbolInfoTy(Addr, Name,
                        Obj->isELF() ? getElfSymbolType(Obj, Symbol)
                                     : (uint8_t)ELF::STT_NOTYPE);
}

static SymbolInfoTy createDummySymbolInfo(const ObjectFile *Obj,
                                          const uint64_t Addr, StringRef &Name,
                                          uint8_t Type) {
  if (Obj->isXCOFF() && SymbolDescription)
    return SymbolInfoTy(Addr, Name, None, None, false);
  else
    return SymbolInfoTy(Addr, Name, Type);
}

static void disassembleObject(const Target *TheTarget, const ObjectFile *Obj,
                              MCContext &Ctx, MCDisassembler *PrimaryDisAsm,
                              MCDisassembler *SecondaryDisAsm,
                              const MCInstrAnalysis *MIA, MCInstPrinter *IP,
                              const MCSubtargetInfo *PrimarySTI,
                              const MCSubtargetInfo *SecondarySTI,
                              PrettyPrinter &PIP,
                              SourcePrinter &SP, bool InlineRelocs) {
  const MCSubtargetInfo *STI = PrimarySTI;
  MCDisassembler *DisAsm = PrimaryDisAsm;

  std::map<SectionRef, std::vector<RelocationRef>> RelocMap;
  if (InlineRelocs)
    RelocMap = getRelocsMap(*Obj); // sectionda
  bool Is64Bits = Obj->getBytesInAddress() > 4; // printrelocationda kullaniliyor is64bits

  // Create a mapping from virtual address to symbol name.  This is used to
  // pretty print the symbols while disassembling.
  std::map<SectionRef, SectionSymbolsTy> AllSymbols;
  SectionSymbolsTy AbsoluteSymbols;
  const StringRef FileName = Obj->getFileName();

  for (const SymbolRef &Symbol : Obj->symbols()) {
    StringRef Name = unwrapOrError(Symbol.getName(), FileName);

    if (Obj->isELF() && getElfSymbolType(Obj, Symbol) == ELF::STT_SECTION)
      continue;


    section_iterator SecI = unwrapOrError(Symbol.getSection(), FileName);
    if (SecI != Obj->section_end())
      AllSymbols[*SecI].push_back(createSymbolInfo(Obj, Symbol));
    else
      AbsoluteSymbols.push_back(createSymbolInfo(Obj, Symbol));
  }

  if (AllSymbols.empty() && Obj->isELF())
    addDynamicElfSymbols(Obj, AllSymbols);

  BumpPtrAllocator A;
  StringSaver Saver(A);
  addPltEntries(Obj, AllSymbols, Saver);

  // Create a mapping from virtual address to section. An empty section can
  // cause more than one section at the same address. Sort such sections to be
  // before same-addressed non-empty sections so that symbol lookups prefer the
  // non-empty section.
  std::vector<std::pair<uint64_t, SectionRef>> SectionAddresses;
  for (SectionRef Sec : Obj->sections())
    SectionAddresses.emplace_back(Sec.getAddress(), Sec);
  llvm::stable_sort(SectionAddresses, [](const auto &LHS, const auto &RHS) {
    if (LHS.first != RHS.first)
      return LHS.first < RHS.first;
    return LHS.second.getSize() < RHS.second.getSize();
  });

  // Sort all the symbols, this allows us to use a simple binary search to find
  // Multiple symbols can have the same address. Use a stable sort to stabilize
  // the output.
  StringSet<> FoundDisasmSymbolSet;
  for (std::pair<const SectionRef, SectionSymbolsTy> &SecSyms : AllSymbols)
    stable_sort(SecSyms.second);
  stable_sort(AbsoluteSymbols);

  std::unique_ptr<DWARFContext> DICtx;
  LiveVariablePrinter LVP(*Ctx.getRegisterInfo(), *STI);

  if (DbgVariables != DVDisabled) {
    DICtx = DWARFContext::create(*Obj);
    for (const std::unique_ptr<DWARFUnit> &CU : DICtx->compile_units())
      LVP.addCompileUnit(CU->getUnitDIE(false));
  }

  LLVM_DEBUG(LVP.dump());

  for (const SectionRef &Section : ToolSectionFilter(*Obj)) {
    if (FilterSections.empty() &&
        (!Section.isText() || Section.isVirtual()))
      continue;

    uint64_t SectionAddr = Section.getAddress();
    uint64_t SectSize = Section.getSize();
    if (!SectSize)
      continue;

    // Get the list of all the symbols in this section.
    SectionSymbolsTy &Symbols = AllSymbols[Section];
    std::vector<MappingSymbolPair> MappingSymbols;
    if (hasMappingSymbols(Obj)) {
      for (const auto &Symb : Symbols) {
        uint64_t Address = Symb.Addr;
        StringRef Name = Symb.Name;
        if (Name.startswith("$d"))
          MappingSymbols.emplace_back(Address - SectionAddr, 'd');
        if (Name.startswith("$x"))
          MappingSymbols.emplace_back(Address - SectionAddr, 'x');
        if (Name.startswith("$a"))
          MappingSymbols.emplace_back(Address - SectionAddr, 'a');
        if (Name.startswith("$t"))
          MappingSymbols.emplace_back(Address - SectionAddr, 't');
      }
    }

    llvm::sort(MappingSymbols);

    if (Obj->isELF() && Obj->getArch() == Triple::amdgcn) {
      // AMDGPU disassembler uses symbolizer for printing labels
      std::unique_ptr<MCRelocationInfo> RelInfo(
        TheTarget->createMCRelocationInfo(TripleName, Ctx));
      if (RelInfo) {
        std::unique_ptr<MCSymbolizer> Symbolizer(
          TheTarget->createMCSymbolizer(
            TripleName, nullptr, nullptr, &Symbols, &Ctx, std::move(RelInfo)));
        DisAsm->setSymbolizer(std::move(Symbolizer));
      }
    }

    StringRef SegmentName = "";
    /*
    if (MachO) {
      DataRefImpl DR = Section.getRawDataRefImpl();
      SegmentName = MachO->getSectionFinalSegmentName(DR);
    }
*/
    StringRef SectionName = unwrapOrError(Section.getName(), Obj->getFileName());
    // If the section has no symbol at the start, just insert a dummy one.
    if (Symbols.empty() || Symbols[0].Addr != 0) {
      Symbols.insert(Symbols.begin(),
                     createDummySymbolInfo(Obj, SectionAddr, SectionName,
                                           Section.isText() ? ELF::STT_FUNC
                                                            : ELF::STT_OBJECT));
    }

    SmallString<40> Comments;
    raw_svector_ostream CommentStream(Comments);

    ArrayRef<uint8_t> Bytes = arrayRefFromStringRef(
        unwrapOrError(Section.getContents(), Obj->getFileName()));

    uint64_t VMAAdjustment = 0;
    if (shouldAdjustVA(Section))
      VMAAdjustment = AdjustVMA;

    uint64_t Size;
    uint64_t Index;

    std::vector<RelocationRef> Rels = RelocMap[Section];
    std::vector<RelocationRef>::const_iterator RelCur = Rels.begin();
    std::vector<RelocationRef>::const_iterator RelEnd = Rels.end();
    // Disassemble symbol by symbol.
    for (unsigned SI = 0, SE = Symbols.size(); SI != SE; ++SI) {
      std::string SymbolName = Symbols[SI].Name.str();

      // Skip if --disassemble-symbols is not empty and the symbol is not in
      // the list.
      if (!DisasmSymbolSet.empty() && !DisasmSymbolSet.count(SymbolName))
        continue;

      uint64_t Start = Symbols[SI].Addr;
      if (Start < SectionAddr || StopAddress <= Start)
        continue;
      else
        FoundDisasmSymbolSet.insert(SymbolName);

      // The end is the section end, the beginning of the next symbol, or
      // --stop-address.
      uint64_t End = std::min<uint64_t>(SectionAddr + SectSize, StopAddress);
      if (SI + 1 < SE)
        End = std::min(End, Symbols[SI + 1].Addr);
      if (Start >= End || End <= StartAddress)
        continue;
      Start -= SectionAddr;
      End -= SectionAddr;


      if (Obj->isELF() && Obj->getArch() == Triple::amdgcn) {
        if (Symbols[SI].Type == ELF::STT_AMDGPU_HSA_KERNEL) {
          // skip amd_kernel_code_t at the begining of kernel symbol (256 bytes)
          Start += 256;
        }
        if (SI == SE - 1 ||
            Symbols[SI + 1].Type == ELF::STT_AMDGPU_HSA_KERNEL) {
          // cut trailing zeroes at the end of kernel
          // cut up to 256 bytes
          const uint64_t EndAlign = 256;
          const auto Limit = End - (std::min)(EndAlign, End - Start);
          while (End > Limit &&
            *reinterpret_cast<const support::ulittle32_t*>(&Bytes[End - 4]) == 0)
            End -= 4;
        }
      }


      Start += Size;

      Index = Start;
      if (SectionAddr < StartAddress)
        Index = std::max<uint64_t>(Index, StartAddress - SectionAddr);

      // If there is a data/common symbol inside an ELF text section and we are
      // only disassembling text (applicable all architectures), we are in a
      // situation where we must print the data and not disassemble it.
      if (Obj->isELF() && Section.isText()) {
        uint8_t SymTy = Symbols[SI].Type;
        if (SymTy == ELF::STT_OBJECT || SymTy == ELF::STT_COMMON) {
          dumpELFData(SectionAddr, Index, End, Bytes);
          Index = End;
        }
      }

      bool CheckARMELFData = hasMappingSymbols(Obj) &&
                             Symbols[SI].Type != ELF::STT_OBJECT;

      formatted_raw_ostream FOS(outs());
      while (Index < End) {

            uint64_t MaxOffset = End - Index;

            if (RelCur != RelEnd)
              MaxOffset = RelCur->getOffset() - Index;

            if (size_t N =
                    countSkippableZeroBytes(Bytes.slice(Index, MaxOffset))) {
              //FOS << "\t\t..." << '\n'; // degistirdim yoruma aldim
              Index += N;
              continue;
            }

          MCInst Inst;
          bool Disassembled =
              DisAsm->getInstruction(Inst, Size, Bytes.slice(Index),
                                     SectionAddr + Index, CommentStream);
          if (Size == 0)
            Size = 1;

          LVP.update({Index, Section.getIndex()},
                     {Index + Size, Section.getIndex()}, Index + Size != End);
//degistirdim
//const Target *TheTarget = getTarget(Obj); //buraya geliyomus zaten
      std::unique_ptr<const MCRegisterInfo> MRI(
      TheTarget->createMCRegInfo(TripleName));
          PIP.printInst(
              *IP, Disassembled ? &Inst : nullptr, Bytes.slice(Index, Size), // ekledim
              {SectionAddr + Index + VMAAdjustment, Section.getIndex()}, FOS,
              "", *STI, &SP, Obj->getFileName(), &Rels, LVP, *MRI); //buraya fazladan MRI parametresi ekleyelim.

          if (Disassembled && MIA) { // bura cok onemli disassembled olmazsa yazmıyor hexleri
            uint64_t Target;
            bool PrintTarget =
                MIA->evaluateBranch(Inst, SectionAddr + Index, Size, Target);
            if (!PrintTarget)
              if (Optional<uint64_t> MaybeTarget =
                      MIA->evaluateMemoryOperandAddress(
                          Inst, SectionAddr + Index, Size)) {
                Target = *MaybeTarget;
                PrintTarget = true;
                FOS << "  # " << Twine::utohexstr(Target); // bura da onemli
              }

          }
        

        LVP.printAfterInst(FOS); //bura printafterinstteki boşluklar olmasa, bu fonk çalışmasa hexi dosyaya basmıyor
        OSS << "\n"; // FOS degistirdim

        // Hexagon does this in pretty printer
        if (Obj->getArch() != Triple::hexagon) {
          // Print relocation for instruction and data.
          while (RelCur != RelEnd) {
            uint64_t Offset = RelCur->getOffset();
            // If this relocation is hidden, skip it.
            if (getHidden(*RelCur) || SectionAddr + Offset < StartAddress) {
              ++RelCur;
              continue;
            }

            // Stop when RelCur's offset is past the disassembled
            // instruction/data. Note that it's possible the disassembled data
            // is not the complete data: we might see the relocation printed in
            // the middle of the data, but this matches the binutils objdump
            // output.
            if (Offset >= Index + Size)
              break;

            // When --adjust-vma is used, update the address printed.
            if (RelCur->getSymbol() != Obj->symbol_end()) {
              Expected<section_iterator> SymSI =
                  RelCur->getSymbol()->getSection();
              if (SymSI && *SymSI != Obj->section_end() &&
                  shouldAdjustVA(**SymSI))
                Offset += AdjustVMA;
            }

            printRelocation(FOS, Obj->getFileName(), *RelCur,
                            SectionAddr + Offset, Is64Bits);
            LVP.printAfterOtherLine(FOS, true);
            ++RelCur;
          }
        }

        Index += Size;
      }
    }
  }
}

//static void disassembleObject(const Target *TheTarget, const ObjectFile *Obj,
//                              MCContext &Ctx, MCDisassembler *PrimaryDisAsm,
//                              MCDisassembler *SecondaryDisAsm,
//                              const MCInstrAnalysis *MIA, MCInstPrinter *IP,
//                              const MCSubtargetInfo *PrimarySTI,
//                              const MCSubtargetInfo *SecondarySTI,
//                              PrettyPrinter &PIP,
//                              SourcePrinter &SP, bool InlineRelocs) {
//  const MCSubtargetInfo *STI = PrimarySTI;
//  MCDisassembler *DisAsm = PrimaryDisAsm;
//  bool PrimaryIsThumb = false;
//  if (isArmElf(Obj))
//    PrimaryIsThumb = STI->checkFeatures("+thumb-mode");
//
//  std::map<SectionRef, std::vector<RelocationRef>> RelocMap;
//  if (InlineRelocs)
//    RelocMap = getRelocsMap(*Obj);
//  bool Is64Bits = Obj->getBytesInAddress() > 4;
//
//  // Create a mapping from virtual address to symbol name.  This is used to
//  // pretty print the symbols while disassembling.
//  std::map<SectionRef, SectionSymbolsTy> AllSymbols;
//  SectionSymbolsTy AbsoluteSymbols;
//  const StringRef FileName = Obj->getFileName();
//  const MachOObjectFile *MachO = dyn_cast<const MachOObjectFile>(Obj);
//  for (const SymbolRef &Symbol : Obj->symbols()) {
//    StringRef Name = unwrapOrError(Symbol.getName(), FileName);
//    if (Name.empty() && !(Obj->isXCOFF() && SymbolDescription))
//      continue;
//
//    if (Obj->isELF() && getElfSymbolType(Obj, Symbol) == ELF::STT_SECTION)
//      continue;
//
//    // Don't ask a Mach-O STAB symbol for its section unless you know that
//    // STAB symbol's section field refers to a valid section index. Otherwise
//    // the symbol may error trying to load a section that does not exist.
//    if (MachO) {
//      DataRefImpl SymDRI = Symbol.getRawDataRefImpl();
//      uint8_t NType = (MachO->is64Bit() ?
//                       MachO->getSymbol64TableEntry(SymDRI).n_type:
//                       MachO->getSymbolTableEntry(SymDRI).n_type);
//      if (NType & MachO::N_STAB)
//        continue;
//    }
//
//    section_iterator SecI = unwrapOrError(Symbol.getSection(), FileName);
//    if (SecI != Obj->section_end())
//      AllSymbols[*SecI].push_back(createSymbolInfo(Obj, Symbol));
//    else
//      AbsoluteSymbols.push_back(createSymbolInfo(Obj, Symbol));
//  }
//
//  if (AllSymbols.empty() && Obj->isELF())
//    addDynamicElfSymbols(Obj, AllSymbols);
//
//  BumpPtrAllocator A;
//  StringSaver Saver(A);
//  addPltEntries(Obj, AllSymbols, Saver);
//
//  // Create a mapping from virtual address to section. An empty section can
//  // cause more than one section at the same address. Sort such sections to be
//  // before same-addressed non-empty sections so that symbol lookups prefer the
//  // non-empty section.
//  std::vector<std::pair<uint64_t, SectionRef>> SectionAddresses;
//  for (SectionRef Sec : Obj->sections())
//    SectionAddresses.emplace_back(Sec.getAddress(), Sec);
//  llvm::stable_sort(SectionAddresses, [](const auto &LHS, const auto &RHS) {
//    if (LHS.first != RHS.first)
//      return LHS.first < RHS.first;
//    return LHS.second.getSize() < RHS.second.getSize();
//  });
//
//  // Linked executables (.exe and .dll files) typically don't include a real
//  // symbol table but they might contain an export table.
//  if (const auto *COFFObj = dyn_cast<COFFObjectFile>(Obj)) {
//    for (const auto &ExportEntry : COFFObj->export_directories()) {
//      StringRef Name;
//      if (Error E = ExportEntry.getSymbolName(Name))
//        reportError(std::move(E), Obj->getFileName());
//      if (Name.empty())
//        continue;
//
//      uint32_t RVA;
//      if (Error E = ExportEntry.getExportRVA(RVA))
//        reportError(std::move(E), Obj->getFileName());
//
//      uint64_t VA = COFFObj->getImageBase() + RVA;
//      auto Sec = partition_point(
//          SectionAddresses, [VA](const std::pair<uint64_t, SectionRef> &O) {
//            return O.first <= VA;
//          });
//      if (Sec != SectionAddresses.begin()) {
//        --Sec;
//        AllSymbols[Sec->second].emplace_back(VA, Name, ELF::STT_NOTYPE);
//      } else
//        AbsoluteSymbols.emplace_back(VA, Name, ELF::STT_NOTYPE);
//    }
//  }
//
//  // Sort all the symbols, this allows us to use a simple binary search to find
//  // Multiple symbols can have the same address. Use a stable sort to stabilize
//  // the output.
//  StringSet<> FoundDisasmSymbolSet;
//  for (std::pair<const SectionRef, SectionSymbolsTy> &SecSyms : AllSymbols)
//    stable_sort(SecSyms.second);
//  stable_sort(AbsoluteSymbols);
//
//  std::unique_ptr<DWARFContext> DICtx;
//  LiveVariablePrinter LVP(*Ctx.getRegisterInfo(), *STI);
//
//  if (DbgVariables != DVDisabled) {
//    DICtx = DWARFContext::create(*Obj);
//    for (const std::unique_ptr<DWARFUnit> &CU : DICtx->compile_units())
//      LVP.addCompileUnit(CU->getUnitDIE(false));
//  }
//
//  LLVM_DEBUG(LVP.dump());
//
//  for (const SectionRef &Section : ToolSectionFilter(*Obj)) {
//    if (FilterSections.empty() && !DisassembleAll &&
//        (!Section.isText() || Section.isVirtual()))
//      continue;
//
//    uint64_t SectionAddr = Section.getAddress();
//    uint64_t SectSize = Section.getSize();
//    if (!SectSize)
//      continue;
//
//    // Get the list of all the symbols in this section.
//    SectionSymbolsTy &Symbols = AllSymbols[Section];
//    std::vector<MappingSymbolPair> MappingSymbols;
//    if (hasMappingSymbols(Obj)) {
//      for (const auto &Symb : Symbols) {
//        uint64_t Address = Symb.Addr;
//        StringRef Name = Symb.Name;
//        if (Name.startswith("$d"))
//          MappingSymbols.emplace_back(Address - SectionAddr, 'd');
//        if (Name.startswith("$x"))
//          MappingSymbols.emplace_back(Address - SectionAddr, 'x');
//        if (Name.startswith("$a"))
//          MappingSymbols.emplace_back(Address - SectionAddr, 'a');
//        if (Name.startswith("$t"))
//          MappingSymbols.emplace_back(Address - SectionAddr, 't');
//      }
//    }
//
//    llvm::sort(MappingSymbols);
//
//    if (Obj->isELF() && Obj->getArch() == Triple::amdgcn) {
//      // AMDGPU disassembler uses symbolizer for printing labels
//      std::unique_ptr<MCRelocationInfo> RelInfo(
//        TheTarget->createMCRelocationInfo(TripleName, Ctx));
//      if (RelInfo) {
//        std::unique_ptr<MCSymbolizer> Symbolizer(
//          TheTarget->createMCSymbolizer(
//            TripleName, nullptr, nullptr, &Symbols, &Ctx, std::move(RelInfo)));
//        DisAsm->setSymbolizer(std::move(Symbolizer));
//      }
//    }
//
//    StringRef SegmentName = "";
//    if (MachO) {
//      DataRefImpl DR = Section.getRawDataRefImpl();
//      SegmentName = MachO->getSectionFinalSegmentName(DR);
//    }
//
//    StringRef SectionName = unwrapOrError(Section.getName(), Obj->getFileName());
//    // If the section has no symbol at the start, just insert a dummy one.
//    if (Symbols.empty() || Symbols[0].Addr != 0) {
//      Symbols.insert(Symbols.begin(),
//                     createDummySymbolInfo(Obj, SectionAddr, SectionName,
//                                           Section.isText() ? ELF::STT_FUNC
//                                                            : ELF::STT_OBJECT));
//    }
//
//    SmallString<40> Comments;
//    raw_svector_ostream CommentStream(Comments);
//
//    ArrayRef<uint8_t> Bytes = arrayRefFromStringRef(
//        unwrapOrError(Section.getContents(), Obj->getFileName())); // bytes i aldigi yer
//
//    uint64_t VMAAdjustment = 0;
//    if (shouldAdjustVA(Section))
//      VMAAdjustment = AdjustVMA;
//
//    uint64_t Size;
//    uint64_t Index;
//    bool PrintedSection = false;
//    std::vector<RelocationRef> Rels = RelocMap[Section];
//    std::vector<RelocationRef>::const_iterator RelCur = Rels.begin();
//    std::vector<RelocationRef>::const_iterator RelEnd = Rels.end();
//    // Disassemble symbol by symbol.
//    for (unsigned SI = 0, SE = Symbols.size(); SI != SE; ++SI) {
//      std::string SymbolName = Symbols[SI].Name.str();
//      if (Demangle)
//        SymbolName = demangle(SymbolName);
//
//      // Skip if --disassemble-symbols is not empty and the symbol is not in
//      // the list.
//      if (!DisasmSymbolSet.empty() && !DisasmSymbolSet.count(SymbolName))
//        continue;
//
//      uint64_t Start = Symbols[SI].Addr;
//      if (Start < SectionAddr || StopAddress <= Start)
//        continue;
//      else
//        FoundDisasmSymbolSet.insert(SymbolName);
//
//      // The end is the section end, the beginning of the next symbol, or
//      // --stop-address.
//      uint64_t End = std::min<uint64_t>(SectionAddr + SectSize, StopAddress);
//      if (SI + 1 < SE)
//        End = std::min(End, Symbols[SI + 1].Addr);
//      if (Start >= End || End <= StartAddress)
//        continue;
//      Start -= SectionAddr;
//      End -= SectionAddr;
//
//      if (!PrintedSection) {
//        PrintedSection = true;
//        //outs() << "\nDisassembly of section ";
//        //if (!SegmentName.empty())
//          //outs() << SegmentName << ",";
//        //outs() << SectionName << ":\n";
//      }
//
//      if (Obj->isELF() && Obj->getArch() == Triple::amdgcn) {
//        if (Symbols[SI].Type == ELF::STT_AMDGPU_HSA_KERNEL) {
//          // skip amd_kernel_code_t at the begining of kernel symbol (256 bytes)
//          Start += 256;
//        }
//        if (SI == SE - 1 ||
//            Symbols[SI + 1].Type == ELF::STT_AMDGPU_HSA_KERNEL) {
//          // cut trailing zeroes at the end of kernel
//          // cut up to 256 bytes
//          const uint64_t EndAlign = 256;
//          const auto Limit = End - (std::min)(EndAlign, End - Start);
//          while (End > Limit &&
//            *reinterpret_cast<const support::ulittle32_t*>(&Bytes[End - 4]) == 0)
//            End -= 4;
//        }
//      }
//
//      //outs() << '\n';
//      //if (!NoLeadingAddr)
//        //outs() << format(Is64Bits ? "%016" PRIx64 " " : "%08" PRIx64 " ",
//          //               SectionAddr + Start + VMAAdjustment);
//      /*if (Obj->isXCOFF() && SymbolDescription) {
//        outs() << getXCOFFSymbolDescription(Symbols[SI], SymbolName) << ":\n";
//      } else
//        outs() << '<' << SymbolName << ">:\n";
//
//      // Don't print raw contents of a virtual section. A virtual section
//      // doesn't have any contents in the file.
//      if (Section.isVirtual()) {
//        outs() << "...\n";
//        continue;
//      }*/
//
//      auto Status = DisAsm->onSymbolStart(Symbols[SI], Size,
//                                          Bytes.slice(Start, End - Start),
//                                          SectionAddr + Start, CommentStream);
//      // To have round trippable disassembly, we fall back to decoding the
//      // remaining bytes as instructions.
//      //
//      // If there is a failure, we disassemble the failed region as bytes before
//      // falling back. The target is expected to print nothing in this case.
//      //
//      // If there is Success or SoftFail i.e no 'real' failure, we go ahead by
//      // Size bytes before falling back.
//      // So if the entire symbol is 'eaten' by the target:
//      //   Start += Size  // Now Start = End and we will never decode as
//      //                  // instructions
//      //
//      // Right now, most targets return None i.e ignore to treat a symbol
//      // separately. But WebAssembly decodes preludes for some symbols.
//      //
//      if (Status.hasValue()) {
//        if (Status.getValue() == MCDisassembler::Fail) {
//          outs() << "// Error in decoding " << SymbolName
//                 << " : Decoding failed region as bytes.\n";
//          for (uint64_t I = 0; I < Size; ++I) {
//            outs() << "\t.byte\t " << format_hex(Bytes[I], 1, /*Upper=*/true)
//                   << "\n";
//          }
//        }
//      } else {
//        Size = 0;
//      }
//
//      Start += Size;
//
//      Index = Start;
//      if (SectionAddr < StartAddress)
//        Index = std::max<uint64_t>(Index, StartAddress - SectionAddr);
//bool DisassembleAll = false;
//      // If there is a data/common symbol inside an ELF text section and we are
//      // only disassembling text (applicable all architectures), we are in a
//      // situation where we must print the data and not disassemble it.
//      if (Obj->isELF() && !DisassembleAll && Section.isText()) {
//        uint8_t SymTy = Symbols[SI].Type;
//        if (SymTy == ELF::STT_OBJECT || SymTy == ELF::STT_COMMON) {
//          dumpELFData(SectionAddr, Index, End, Bytes);
//          Index = End;
//        }
//      }
//
//      bool CheckARMELFData = hasMappingSymbols(Obj) &&
//                             Symbols[SI].Type != ELF::STT_OBJECT &&
//                             !DisassembleAll;
//      bool DumpARMELFData = false;
//      formatted_raw_ostream FOS(outs());
//      while (Index < End) {
//        // ARM and AArch64 ELF binaries can interleave data and text in the
//        // same section. We rely on the markers introduced to understand what
//        // we need to dump. If the data marker is within a function, it is
//        // denoted as a word/short etc.
//        if (CheckARMELFData) {
//          char Kind = getMappingSymbolKind(MappingSymbols, Index);
//          DumpARMELFData = Kind == 'd';
//          if (SecondarySTI) {
//            if (Kind == 'a') {
//              STI = PrimaryIsThumb ? SecondarySTI : PrimarySTI;
//              DisAsm = PrimaryIsThumb ? SecondaryDisAsm : PrimaryDisAsm;
//            } else if (Kind == 't') {
//              STI = PrimaryIsThumb ? PrimarySTI : SecondarySTI;
//              DisAsm = PrimaryIsThumb ? PrimaryDisAsm : SecondaryDisAsm;
//            }
//          }
//        }
//bool DisassembleZeroes = false;
//        if (DumpARMELFData) {
//          Size = dumpARMELFData(SectionAddr, Index, End, Obj, Bytes,
//                                MappingSymbols, FOS);
//        } else {
//          // When -z or --disassemble-zeroes are given we always dissasemble
//          // them. Otherwise we might want to skip zero bytes we see.
//          if (!DisassembleZeroes) {
//            uint64_t MaxOffset = End - Index;
//            // For --reloc: print zero blocks patched by relocations, so that
//            // relocations can be shown in the dump.
//            if (RelCur != RelEnd)
//              MaxOffset = RelCur->getOffset() - Index;
//
//            if (size_t N =
//                    countSkippableZeroBytes(Bytes.slice(Index, MaxOffset))) {
//              FOS << "\t\t..." << '\n';
//              Index += N;
//              continue;
//            }
//          }
//
//          // Disassemble a real instruction or a data when disassemble all is
//          // provided
//          MCInst Inst;
//          bool Disassembled =
//              DisAsm->getInstruction(Inst, Size, Bytes.slice(Index),
//                                     SectionAddr + Index, CommentStream);
//          if (Size == 0)
//            Size = 1;
//
//          LVP.update({Index, Section.getIndex()},
//                     {Index + Size, Section.getIndex()}, Index + Size != End);
////degistirdim
//          PIP.printInst(
//              *IP, !Disassembled ? &Inst : nullptr, Bytes.slice(Index, Size),
//              {SectionAddr + Index + VMAAdjustment, Section.getIndex()}, FOS,
//              "", *STI, &SP, Obj->getFileName(), &Rels, LVP); LVP.printAfterInst(FOS); FOS << "\n";} Index += Size;} // while in bittigi yer
//              } } // buraya 4 tane } ekleyince kapandı blocklar
////          /*FOS << CommentStream.str();
////          Comments.clear();*/
////
////          // If disassembly has failed, avoid analysing invalid/incomplete
////          // instruction information. Otherwise, try to resolve the target
////          // address (jump target or memory operand address) and print it on the
////          // right of the instruction.
////          if (Disassembled && MIA) {
////            uint64_t Target;
////            bool PrintTarget =
////                MIA->evaluateBranch(Inst, SectionAddr + Index, Size, Target);
////            if (!PrintTarget)
////              if (Optional<uint64_t> MaybeTarget =
////                      MIA->evaluateMemoryOperandAddress(
////                          Inst, SectionAddr + Index, Size)) {
////                Target = *MaybeTarget;
////                PrintTarget = true;
////                FOS << "  # " << Twine::utohexstr(Target);
////              }
////            if (PrintTarget) {
////              // In a relocatable object, the target's section must reside in
////              // the same section as the call instruction or it is accessed
////              // through a relocation.
////              //
////              // In a non-relocatable object, the target may be in any section.
////              // In that case, locate the section(s) containing the target
////              // address and find the symbol in one of those, if possible.
////              //
////              // N.B. We don't walk the relocations in the relocatable case yet.
////              std::vector<const SectionSymbolsTy *> TargetSectionSymbols;
////              if (!Obj->isRelocatableObject()) {
////                auto It = llvm::partition_point(
////                    SectionAddresses,
////                    [=](const std::pair<uint64_t, SectionRef> &O) {
////                      return O.first <= Target;
////                    });
////                uint64_t TargetSecAddr = 0;
////                while (It != SectionAddresses.begin()) {
////                  --It;
////                  if (TargetSecAddr == 0)
////                    TargetSecAddr = It->first;
////                  if (It->first != TargetSecAddr)
////                    break;
////                  TargetSectionSymbols.push_back(&AllSymbols[It->second]);
////                }
////              } else {
////                TargetSectionSymbols.push_back(&Symbols);
////              }
////              TargetSectionSymbols.push_back(&AbsoluteSymbols);
////
////              // Find the last symbol in the first candidate section whose
////              // offset is less than or equal to the target. If there are no
////              // such symbols, try in the next section and so on, before finally
////              // using the nearest preceding absolute symbol (if any), if there
////              // are no other valid symbols.
////              const SymbolInfoTy *TargetSym = nullptr;
////              for (const SectionSymbolsTy *TargetSymbols :
////                   TargetSectionSymbols) {
////                auto It = llvm::partition_point(
////                    *TargetSymbols,
////                    [=](const SymbolInfoTy &O) { return O.Addr <= Target; });
////                if (It != TargetSymbols->begin()) {
////                  TargetSym = &*(It - 1);
////                  break;
////                }
////              }
////
////              if (TargetSym != nullptr) {
////                uint64_t TargetAddress = TargetSym->Addr;
////                std::string TargetName = TargetSym->Name.str();
////                if (Demangle)
////                  TargetName = demangle(TargetName);
////
////                FOS << " <" << TargetName;
////                uint64_t Disp = Target - TargetAddress;
////                if (Disp)
////                  FOS << "+0x" << Twine::utohexstr(Disp);
////                FOS << '>';
////              }
////            }
////          }
////        }
////
////        LVP.printAfterInst(FOS);
////        FOS << "\n";
////
////        // Hexagon does this in pretty printer
////        if (Obj->getArch() != Triple::hexagon) {
////          // Print relocation for instruction and data.
////          while (RelCur != RelEnd) {
////            uint64_t Offset = RelCur->getOffset();
////            // If this relocation is hidden, skip it.
////            if (getHidden(*RelCur) || SectionAddr + Offset < StartAddress) {
////              ++RelCur;
////              continue;
////            }
////
////            // Stop when RelCur's offset is past the disassembled
////            // instruction/data. Note that it's possible the disassembled data
////            // is not the complete data: we might see the relocation printed in
////            // the middle of the data, but this matches the binutils objdump
////            // output.
////            if (Offset >= Index + Size)
////              break;
////
////            // When --adjust-vma is used, update the address printed.
////            if (RelCur->getSymbol() != Obj->symbol_end()) {
////              Expected<section_iterator> SymSI =
////                  RelCur->getSymbol()->getSection();
////              if (SymSI && *SymSI != Obj->section_end() &&
////                  shouldAdjustVA(**SymSI))
////                Offset += AdjustVMA;
////            }
////
////            printRelocation(FOS, Obj->getFileName(), *RelCur,
////                            SectionAddr + Offset, Is64Bits);
////            LVP.printAfterOtherLine(FOS, true);
////            ++RelCur;
////          }
////        }
////
////        Index += Size;
////      }
////    }
////  }
////  StringSet<> MissingDisasmSymbolSet =
////      set_difference(DisasmSymbolSet, FoundDisasmSymbolSet);
////  for (StringRef Sym : MissingDisasmSymbolSet.keys())
////    reportWarning("failed to disassemble missing symbol " + Sym, FileName);
//}

static void disassembleObject(const ObjectFile *Obj, bool InlineRelocs) {
  const Target *TheTarget = getTarget(Obj);

  // Package up features to be passed to target/subtarget
  SubtargetFeatures Features = Obj->getFeatures();
  if (!MAttrs.empty())
    for (unsigned I = 0; I != MAttrs.size(); ++I)
      Features.AddFeature(MAttrs[I]);

  std::unique_ptr<const MCRegisterInfo> MRI(
      TheTarget->createMCRegInfo(TripleName));
  if (!MRI)
    reportError(Obj->getFileName(),
                "no register info for target " + TripleName);

  // Set up disassembler.
  MCTargetOptions MCOptions;
  std::unique_ptr<const MCAsmInfo> AsmInfo(
      TheTarget->createMCAsmInfo(*MRI, TripleName, MCOptions));
  if (!AsmInfo)
    reportError(Obj->getFileName(),
                "no assembly info for target " + TripleName);
  std::unique_ptr<const MCSubtargetInfo> STI(
      TheTarget->createMCSubtargetInfo(TripleName, MCPU, Features.getString()));
  if (!STI)
    reportError(Obj->getFileName(),
                "no subtarget info for target " + TripleName);
  std::unique_ptr<const MCInstrInfo> MII(TheTarget->createMCInstrInfo());
  if (!MII)
    reportError(Obj->getFileName(),
                "no instruction info for target " + TripleName);
  MCObjectFileInfo MOFI;
  MCContext Ctx(AsmInfo.get(), MRI.get(), &MOFI);
  // FIXME: for now initialize MCObjectFileInfo with default values
  MOFI.InitMCObjectFileInfo(Triple(TripleName), false, Ctx);

  std::unique_ptr<MCDisassembler> DisAsm(
      TheTarget->createMCDisassembler(*STI, Ctx));
  if (!DisAsm)
    reportError(Obj->getFileName(), "no disassembler for target " + TripleName);

  // If we have an ARM object file, we need a second disassembler, because
  // ARM CPUs have two different instruction sets: ARM mode, and Thumb mode.
  // We use mapping symbols to switch between the two assemblers, where
  // appropriate.
  std::unique_ptr<MCDisassembler> SecondaryDisAsm;
  std::unique_ptr<const MCSubtargetInfo> SecondarySTI;
  if (isArmElf(Obj) && !STI->checkFeatures("+mclass")) {
    if (STI->checkFeatures("+thumb-mode"))
      Features.AddFeature("-thumb-mode");
    else
      Features.AddFeature("+thumb-mode");
    SecondarySTI.reset(TheTarget->createMCSubtargetInfo(TripleName, MCPU,
                                                        Features.getString()));
    SecondaryDisAsm.reset(TheTarget->createMCDisassembler(*SecondarySTI, Ctx));
  }

  std::unique_ptr<const MCInstrAnalysis> MIA(
      TheTarget->createMCInstrAnalysis(MII.get()));

  int AsmPrinterVariant = AsmInfo->getAssemblerDialect();
  std::unique_ptr<MCInstPrinter> IP(TheTarget->createMCInstPrinter(
      Triple(TripleName), AsmPrinterVariant, *AsmInfo, *MII, *MRI));
  if (!IP)
    reportError(Obj->getFileName(),
                "no instruction printer for target " + TripleName);
  IP->setPrintImmHex(PrintImmHex);
  IP->setPrintBranchImmAsAddress(true);

  PrettyPrinter &PIP = selectPrettyPrinter(Triple(TripleName));
  SourcePrinter SP(Obj, TheTarget->getName());

  for (StringRef Opt : DisassemblerOptions)
    if (!IP->applyTargetSpecificCLOption(Opt))
      reportError(Obj->getFileName(),
                  "Unrecognized disassembler option: " + Opt);

  disassembleObject(TheTarget, Obj, Ctx, DisAsm.get(), SecondaryDisAsm.get(),
                    MIA.get(), IP.get(), STI.get(), SecondarySTI.get(), PIP,
                    SP, InlineRelocs);
}

void objdump::printRelocations(const ObjectFile *Obj) {
  StringRef Fmt = Obj->getBytesInAddress() > 4 ? "%016" PRIx64 :
                                                 "%08" PRIx64;
  // Regular objdump doesn't print relocations in non-relocatable object
  // files.
  if (!Obj->isRelocatableObject())
    return;

  // Build a mapping from relocation target to a vector of relocation
  // sections. Usually, there is an only one relocation section for
  // each relocated section.
  MapVector<SectionRef, std::vector<SectionRef>> SecToRelSec;
  uint64_t Ndx;
  for (const SectionRef &Section : ToolSectionFilter(*Obj, &Ndx)) {
    if (Section.relocation_begin() == Section.relocation_end())
      continue;
    Expected<section_iterator> SecOrErr = Section.getRelocatedSection();
    if (!SecOrErr)
      reportError(Obj->getFileName(),
                  "section (" + Twine(Ndx) +
                      "): unable to get a relocation target: " +
                      toString(SecOrErr.takeError()));
    SecToRelSec[**SecOrErr].push_back(Section);
  }

  for (std::pair<SectionRef, std::vector<SectionRef>> &P : SecToRelSec) {
    StringRef SecName = unwrapOrError(P.first.getName(), Obj->getFileName());
    outs() << "RELOCATION RECORDS FOR [" << SecName << "]:\n";
    uint32_t OffsetPadding = (Obj->getBytesInAddress() > 4 ? 16 : 8);
    uint32_t TypePadding = 24;
    outs() << left_justify("OFFSET", OffsetPadding) << " "
           << left_justify("TYPE", TypePadding) << " "
           << "VALUE\n";

    for (SectionRef Section : P.second) {
      for (const RelocationRef &Reloc : Section.relocations()) {
        uint64_t Address = Reloc.getOffset();
        SmallString<32> RelocName;
        SmallString<32> ValueStr;
        if (Address < StartAddress || Address > StopAddress || getHidden(Reloc))
          continue;
        Reloc.getTypeName(RelocName);
        if (Error E = getRelocationValueString(Reloc, ValueStr))
          reportError(std::move(E), Obj->getFileName());

        outs() << format(Fmt.data(), Address) << " "
               << left_justify(RelocName, TypePadding) << " " << ValueStr
               << "\n";
      }
    }
    outs() << "\n"; // buradaki \n olmayinca hex codeunu dosyaya basmiyor onemli
    //degistirdim
  }
}

void objdump::printDynamicRelocations(const ObjectFile *Obj) {
  // For the moment, this option is for ELF only
  if (!Obj->isELF())
    return;

  const auto *Elf = dyn_cast<ELFObjectFileBase>(Obj);
  if (!Elf || Elf->getEType() != ELF::ET_DYN) {
    reportError(Obj->getFileName(), "not a dynamic object");
    return;
  }

  std::vector<SectionRef> DynRelSec = Obj->dynamic_relocation_sections();
  if (DynRelSec.empty())
    return;

  outs() << "DYNAMIC RELOCATION RECORDS\n";
  StringRef Fmt = Obj->getBytesInAddress() > 4 ? "%016" PRIx64 : "%08" PRIx64;
  for (const SectionRef &Section : DynRelSec)
    for (const RelocationRef &Reloc : Section.relocations()) {
      uint64_t Address = Reloc.getOffset();
      SmallString<32> RelocName;
      SmallString<32> ValueStr;
      Reloc.getTypeName(RelocName);
      if (Error E = getRelocationValueString(Reloc, ValueStr))
        reportError(std::move(E), Obj->getFileName());
      outs() << format(Fmt.data(), Address) << " " << RelocName << " "
             << ValueStr << "\n";
    }
}

// Returns true if we need to show LMA column when dumping section headers. We
// show it only when the platform is ELF and either we have at least one section
// whose VMA and LMA are different and/or when --show-lma flag is used.
static bool shouldDisplayLMA(const ObjectFile *Obj) {
  if (!Obj->isELF())
    return false;
  for (const SectionRef &S : ToolSectionFilter(*Obj))
    if (S.getAddress() != getELFSectionLMA(S))
      return true;
  return ShowLMA;
}

static size_t getMaxSectionNameWidth(const ObjectFile *Obj) {
  // Default column width for names is 13 even if no names are that long.
  size_t MaxWidth = 13;
  for (const SectionRef &Section : ToolSectionFilter(*Obj)) {
    StringRef Name = unwrapOrError(Section.getName(), Obj->getFileName());
    MaxWidth = std::max(MaxWidth, Name.size());
  }
  return MaxWidth;
}

void objdump::printSectionHeaders(const ObjectFile *Obj) {
  size_t NameWidth = getMaxSectionNameWidth(Obj);
  size_t AddressWidth = 2 * Obj->getBytesInAddress();
  bool HasLMAColumn = shouldDisplayLMA(Obj);
  if (HasLMAColumn)
    outs() << "Sections:\n"
              "Idx "
           << left_justify("Name", NameWidth) << " Size     "
           << left_justify("VMA", AddressWidth) << " "
           << left_justify("LMA", AddressWidth) << " Type\n";
  else
    outs() << "Sections:\n"
              "Idx "
           << left_justify("Name", NameWidth) << " Size     "
           << left_justify("VMA", AddressWidth) << " Type\n";

  uint64_t Idx;
  for (const SectionRef &Section : ToolSectionFilter(*Obj, &Idx)) {
    StringRef Name = unwrapOrError(Section.getName(), Obj->getFileName());
    uint64_t VMA = Section.getAddress();
    if (shouldAdjustVA(Section))
      VMA += AdjustVMA;

    uint64_t Size = Section.getSize();

    std::string Type = Section.isText() ? "TEXT" : "";
    if (Section.isData())
      Type += Type.empty() ? "DATA" : " DATA";
    if (Section.isBSS())
      Type += Type.empty() ? "BSS" : " BSS";

    if (HasLMAColumn)
      outs() << format("%3" PRIu64 " %-*s %08" PRIx64 " ", Idx, NameWidth,
                       Name.str().c_str(), Size)
             << format_hex_no_prefix(VMA, AddressWidth) << " "
             << format_hex_no_prefix(getELFSectionLMA(Section), AddressWidth)
             << " " << Type << "\n";
    else
      outs() << format("%3" PRIu64 " %-*s %08" PRIx64 " ", Idx, NameWidth,
                       Name.str().c_str(), Size)
             << format_hex_no_prefix(VMA, AddressWidth) << " " << Type << "\n";
  }
  //outs() << "\n"; //degistirdim
}

void objdump::printSectionContents(const ObjectFile *Obj) {
  for (const SectionRef &Section : ToolSectionFilter(*Obj)) {
    StringRef Name = unwrapOrError(Section.getName(), Obj->getFileName());
    uint64_t BaseAddr = Section.getAddress();
    uint64_t Size = Section.getSize();
    if (!Size)
      continue;

    outs() << "Contents of section " << Name << ":\n";
    if (Section.isBSS()) {
      outs() << format("<skipping contents of bss section at [%04" PRIx64
                       ", %04" PRIx64 ")>\n",
                       BaseAddr, BaseAddr + Size);
      continue;
    }

    StringRef Contents = unwrapOrError(Section.getContents(), Obj->getFileName());

    // Dump out the content as hex and printable ascii characters.
    for (std::size_t Addr = 0, End = Contents.size(); Addr < End; Addr += 16) {
      outs() << format(" %04" PRIx64 " ", BaseAddr + Addr);
      // Dump line of hex.
      for (std::size_t I = 0; I < 16; ++I) {
        if (I != 0 && I % 4 == 0)
          outs() << ' ';
        if (Addr + I < End)
          outs() << hexdigit((Contents[Addr + I] >> 4) & 0xF, true)
                 << hexdigit(Contents[Addr + I] & 0xF, true);
        else
          outs() << "  ";
      }
      // Print ascii.
      outs() << "  ";
      for (std::size_t I = 0; I < 16 && Addr + I < End; ++I) {
        if (isPrint(static_cast<unsigned char>(Contents[Addr + I]) & 0xFF))
          outs() << Contents[Addr + I];
        else
          outs() << ".";
      }
      //outs() << "\n"; //degistirdim
    }
  }
}

void objdump::printSymbolTable(const ObjectFile *O, StringRef ArchiveName,
                               StringRef ArchitectureName, bool DumpDynamic) {
  if (O->isCOFF() && !DumpDynamic) {
    outs() << "SYMBOL TABLE:\n";
    printCOFFSymbolTable(cast<const COFFObjectFile>(O));
    return;
  }

  const StringRef FileName = O->getFileName();

  if (!DumpDynamic) {
    outs() << "SYMBOL TABLE:\n";
    for (auto I = O->symbol_begin(); I != O->symbol_end(); ++I)
      printSymbol(O, *I, FileName, ArchiveName, ArchitectureName, DumpDynamic);
    return;
  }

  outs() << "DYNAMIC SYMBOL TABLE:\n";
  if (!O->isELF()) {
    reportWarning(
        "this operation is not currently supported for this file format",
        FileName);
    return;
  }

  const ELFObjectFileBase *ELF = cast<const ELFObjectFileBase>(O);
  for (auto I = ELF->getDynamicSymbolIterators().begin();
       I != ELF->getDynamicSymbolIterators().end(); ++I)
    printSymbol(O, *I, FileName, ArchiveName, ArchitectureName, DumpDynamic);
}

void objdump::printSymbol(const ObjectFile *O, const SymbolRef &Symbol,
                          StringRef FileName, StringRef ArchiveName,
                          StringRef ArchitectureName, bool DumpDynamic) {
  const MachOObjectFile *MachO = dyn_cast<const MachOObjectFile>(O);
  uint64_t Address = unwrapOrError(Symbol.getAddress(), FileName, ArchiveName,
                                   ArchitectureName);
  if ((Address < StartAddress) || (Address > StopAddress))
    return;
  SymbolRef::Type Type =
      unwrapOrError(Symbol.getType(), FileName, ArchiveName, ArchitectureName);
  uint32_t Flags =
      unwrapOrError(Symbol.getFlags(), FileName, ArchiveName, ArchitectureName);

  // Don't ask a Mach-O STAB symbol for its section unless you know that
  // STAB symbol's section field refers to a valid section index. Otherwise
  // the symbol may error trying to load a section that does not exist.
  bool IsSTAB = false;
  if (MachO) {
    DataRefImpl SymDRI = Symbol.getRawDataRefImpl();
    uint8_t NType =
        (MachO->is64Bit() ? MachO->getSymbol64TableEntry(SymDRI).n_type
                          : MachO->getSymbolTableEntry(SymDRI).n_type);
    if (NType & MachO::N_STAB)
      IsSTAB = true;
  }
  section_iterator Section = IsSTAB
                                 ? O->section_end()
                                 : unwrapOrError(Symbol.getSection(), FileName,
                                                 ArchiveName, ArchitectureName);

  StringRef Name;
  if (Type == SymbolRef::ST_Debug && Section != O->section_end()) {
    if (Expected<StringRef> NameOrErr = Section->getName())
      Name = *NameOrErr;
    else
      consumeError(NameOrErr.takeError());

  } else {
    Name = unwrapOrError(Symbol.getName(), FileName, ArchiveName,
                         ArchitectureName);
  }

  bool Global = Flags & SymbolRef::SF_Global;
  bool Weak = Flags & SymbolRef::SF_Weak;
  bool Absolute = Flags & SymbolRef::SF_Absolute;
  bool Common = Flags & SymbolRef::SF_Common;
  bool Hidden = Flags & SymbolRef::SF_Hidden;

  char GlobLoc = ' ';
  if ((Section != O->section_end() || Absolute) && !Weak)
    GlobLoc = Global ? 'g' : 'l';
  char IFunc = ' ';
  if (O->isELF()) {
    if (ELFSymbolRef(Symbol).getELFType() == ELF::STT_GNU_IFUNC)
      IFunc = 'i';
    if (ELFSymbolRef(Symbol).getBinding() == ELF::STB_GNU_UNIQUE)
      GlobLoc = 'u';
  }

  char Debug = ' ';
  if (DumpDynamic)
    Debug = 'D';
  else if (Type == SymbolRef::ST_Debug || Type == SymbolRef::ST_File)
    Debug = 'd';

  char FileFunc = ' ';
  if (Type == SymbolRef::ST_File)
    FileFunc = 'f';
  else if (Type == SymbolRef::ST_Function)
    FileFunc = 'F';
  else if (Type == SymbolRef::ST_Data)
    FileFunc = 'O';

  const char *Fmt = O->getBytesInAddress() > 4 ? "%016" PRIx64 : "%08" PRIx64;

  outs() << format(Fmt, Address) << " "
         << GlobLoc            // Local -> 'l', Global -> 'g', Neither -> ' '
         << (Weak ? 'w' : ' ') // Weak?
         << ' '                // Constructor. Not supported yet.
         << ' '                // Warning. Not supported yet.
         << IFunc              // Indirect reference to another symbol.
         << Debug              // Debugging (d) or dynamic (D) symbol.
         << FileFunc           // Name of function (F), file (f) or object (O).
         << ' ';
  if (Absolute) {
    outs() << "*ABS*";
  } else if (Common) {
    outs() << "*COM*";
  } else if (Section == O->section_end()) {
    outs() << "*UND*";
  } else {
    if (MachO) {
      DataRefImpl DR = Section->getRawDataRefImpl();
      StringRef SegmentName = MachO->getSectionFinalSegmentName(DR);
      outs() << SegmentName << ",";
    }
    StringRef SectionName = unwrapOrError(Section->getName(), FileName);
    outs() << SectionName;
  }

  if (Common || O->isELF()) {
    uint64_t Val =
        Common ? Symbol.getAlignment() : ELFSymbolRef(Symbol).getSize();
    outs() << '\t' << format(Fmt, Val);
  }

  if (O->isELF()) {
    uint8_t Other = ELFSymbolRef(Symbol).getOther();
    switch (Other) {
    case ELF::STV_DEFAULT:
      break;
    case ELF::STV_INTERNAL:
      outs() << " .internal";
      break;
    case ELF::STV_HIDDEN:
      outs() << " .hidden";
      break;
    case ELF::STV_PROTECTED:
      outs() << " .protected";
      break;
    default:
      outs() << format(" 0x%02x", Other);
      break;
    }
  } else if (Hidden) {
    outs() << " .hidden";
  }

  if (Demangle)
    outs() << ' ' << demangle(std::string(Name)) << '\n';
  else
    outs() << ' ' << Name << '\n';
}

static void printUnwindInfo(const ObjectFile *O) {
  outs() << "Unwind info:\n\n";

  if (const COFFObjectFile *Coff = dyn_cast<COFFObjectFile>(O))
    printCOFFUnwindInfo(Coff);
  else if (const MachOObjectFile *MachO = dyn_cast<MachOObjectFile>(O))
    printMachOUnwindInfo(MachO);
  else
    // TODO: Extract DWARF dump tool to objdump.
    WithColor::error(errs(), ToolName)
        << "This operation is only currently supported "
           "for COFF and MachO object files.\n";
}

/// Dump the raw contents of the __clangast section so the output can be piped
/// into llvm-bcanalyzer.
static void printRawClangAST(const ObjectFile *Obj) {
  if (outs().is_displayed()) {
    WithColor::error(errs(), ToolName)
        << "The -raw-clang-ast option will dump the raw binary contents of "
           "the clang ast section.\n"
           "Please redirect the output to a file or another program such as "
           "llvm-bcanalyzer.\n";
    return;
  }

  StringRef ClangASTSectionName("__clangast");
  if (Obj->isCOFF()) {
    ClangASTSectionName = "clangast";
  }

  Optional<object::SectionRef> ClangASTSection;
  for (auto Sec : ToolSectionFilter(*Obj)) {
    StringRef Name;
    if (Expected<StringRef> NameOrErr = Sec.getName())
      Name = *NameOrErr;
    else
      consumeError(NameOrErr.takeError());

    if (Name == ClangASTSectionName) {
      ClangASTSection = Sec;
      break;
    }
  }
  if (!ClangASTSection)
    return;

  StringRef ClangASTContents = unwrapOrError(
      ClangASTSection.getValue().getContents(), Obj->getFileName());
  outs().write(ClangASTContents.data(), ClangASTContents.size());
}

static void printFaultMaps(const ObjectFile *Obj) {
  StringRef FaultMapSectionName;

  if (Obj->isELF()) {
    FaultMapSectionName = ".llvm_faultmaps";
  } else if (Obj->isMachO()) {
    FaultMapSectionName = "__llvm_faultmaps";
  } else {
    WithColor::error(errs(), ToolName)
        << "This operation is only currently supported "
           "for ELF and Mach-O executable files.\n";
    return;
  }

  Optional<object::SectionRef> FaultMapSection;

  for (auto Sec : ToolSectionFilter(*Obj)) {
    StringRef Name;
    if (Expected<StringRef> NameOrErr = Sec.getName())
      Name = *NameOrErr;
    else
      consumeError(NameOrErr.takeError());

    if (Name == FaultMapSectionName) {
      FaultMapSection = Sec;
      break;
    }
  }

  outs() << "FaultMap table:\n";

  if (!FaultMapSection.hasValue()) {
    outs() << "<not found>\n";
    return;
  }

  StringRef FaultMapContents =
      unwrapOrError(FaultMapSection.getValue().getContents(), Obj->getFileName());
  FaultMapParser FMP(FaultMapContents.bytes_begin(),
                     FaultMapContents.bytes_end());

  outs() << FMP;
}

static void printPrivateFileHeaders(const ObjectFile *O, bool OnlyFirst) {
  if (O->isELF()) {
    printELFFileHeader(O);
    printELFDynamicSection(O);
    printELFSymbolVersionInfo(O);
    return;
  }
  if (O->isCOFF())
    return printCOFFFileHeader(O);
  if (O->isWasm())
    return printWasmFileHeader(O);
  if (O->isMachO()) {
    printMachOFileHeader(O);
    if (!OnlyFirst)
      printMachOLoadCommands(O);
    return;
  }
  reportError(O->getFileName(), "Invalid/Unsupported object file format");
}

static void printFileHeaders(const ObjectFile *O) {
  if (!O->isELF() && !O->isCOFF())
    reportError(O->getFileName(), "Invalid/Unsupported object file format");

  Triple::ArchType AT = O->getArch();
  outs() << "architecture: " << Triple::getArchTypeName(AT) << "\n";
  uint64_t Address = unwrapOrError(O->getStartAddress(), O->getFileName());

  StringRef Fmt = O->getBytesInAddress() > 4 ? "%016" PRIx64 : "%08" PRIx64;
  outs() << "start address: "
         << "0x" << format(Fmt.data(), Address) << "\n\n";
}

static void printArchiveChild(StringRef Filename, const Archive::Child &C) {
  Expected<sys::fs::perms> ModeOrErr = C.getAccessMode();
  if (!ModeOrErr) {
    WithColor::error(errs(), ToolName) << "ill-formed archive entry.\n";
    consumeError(ModeOrErr.takeError());
    return;
  }
  sys::fs::perms Mode = ModeOrErr.get();
  outs() << ((Mode & sys::fs::owner_read) ? "r" : "-");
  outs() << ((Mode & sys::fs::owner_write) ? "w" : "-");
  outs() << ((Mode & sys::fs::owner_exe) ? "x" : "-");
  outs() << ((Mode & sys::fs::group_read) ? "r" : "-");
  outs() << ((Mode & sys::fs::group_write) ? "w" : "-");
  outs() << ((Mode & sys::fs::group_exe) ? "x" : "-");
  outs() << ((Mode & sys::fs::others_read) ? "r" : "-");
  outs() << ((Mode & sys::fs::others_write) ? "w" : "-");
  outs() << ((Mode & sys::fs::others_exe) ? "x" : "-");

  outs() << " "; //degistirdim

  outs() << format("%d/%d %6" PRId64 " ", unwrapOrError(C.getUID(), Filename),
                   unwrapOrError(C.getGID(), Filename),
                   unwrapOrError(C.getRawSize(), Filename));

  StringRef RawLastModified = C.getRawLastModified();
  unsigned Seconds;
  if (RawLastModified.getAsInteger(10, Seconds))
    outs() << "(date: \"" << RawLastModified
           << "\" contains non-decimal chars) ";
  else {
    // Since ctime(3) returns a 26 character string of the form:
    // "Sun Sep 16 01:03:52 1973\n\0"
    // just print 24 characters.
    time_t t = Seconds;
    outs() << format("%.24s ", ctime(&t));
  }

  StringRef Name = "";
  Expected<StringRef> NameOrErr = C.getName();
  if (!NameOrErr) {
    consumeError(NameOrErr.takeError());
    Name = unwrapOrError(C.getRawName(), Filename);
  } else {
    Name = NameOrErr.get();
  }
  outs() << Name << "\n";
}

// For ELF only now.
static bool shouldWarnForInvalidStartStopAddress(ObjectFile *Obj) {
  if (const auto *Elf = dyn_cast<ELFObjectFileBase>(Obj)) {
    if (Elf->getEType() != ELF::ET_REL)
      return true;
  }
  return false;
}

static void checkForInvalidStartStopAddress(ObjectFile *Obj,
                                            uint64_t Start, uint64_t Stop) {
  if (!shouldWarnForInvalidStartStopAddress(Obj))
    return;

  for (const SectionRef &Section : Obj->sections())
    if (ELFSectionRef(Section).getFlags() & ELF::SHF_ALLOC) {
      uint64_t BaseAddr = Section.getAddress();
      uint64_t Size = Section.getSize();
      if ((Start < BaseAddr + Size) && Stop > BaseAddr)
        return;
    }

  if (StartAddress.getNumOccurrences() == 0)
    reportWarning("no section has address less than 0x" +
                      Twine::utohexstr(Stop) + " specified by --stop-address",
                  Obj->getFileName());
  else if (StopAddress.getNumOccurrences() == 0)
    reportWarning("no section has address greater than or equal to 0x" +
                      Twine::utohexstr(Start) + " specified by --start-address",
                  Obj->getFileName());
  else
    reportWarning("no section overlaps the range [0x" +
                      Twine::utohexstr(Start) + ",0x" + Twine::utohexstr(Stop) +
                      ") specified by --start-address/--stop-address",
                  Obj->getFileName());
}

static void dumpObject(ObjectFile *O, const Archive *A = nullptr,
                       const Archive::Child *C = nullptr) {
  // Avoid other output when using a raw option.
  /*if (!RawClangAST) {
    outs() << '\n';
    if (A)
      outs() << A->getFileName() << "(" << O->getFileName() << ")";
    else
      outs() << O->getFileName();
    outs() << ":\tfile format " << O->getFileFormatName().lower() << "\n\n";
  }*/

  if (StartAddress.getNumOccurrences() || StopAddress.getNumOccurrences())
    checkForInvalidStartStopAddress(O, StartAddress, StopAddress);

  // Note: the order here matches GNU objdump for compatability.
  StringRef ArchiveName = A ? A->getFileName() : "";
  if (ArchiveHeaders && !MachOOpt && C)
    printArchiveChild(ArchiveName, *C);
  if (FileHeaders)
    printFileHeaders(O);
  if (PrivateHeaders || FirstPrivateHeader)
    printPrivateFileHeaders(O, FirstPrivateHeader);
  if (SectionHeaders)
    printSectionHeaders(O);
  if (SymbolTable)
    printSymbolTable(O, ArchiveName);
  if (DynamicSymbolTable)
    printSymbolTable(O, ArchiveName, /*ArchitectureName=*/"",
                     /*DumpDynamic=*/true);
  if (DwarfDumpType != DIDT_Null) {
    std::unique_ptr<DIContext> DICtx = DWARFContext::create(*O);
    // Dump the complete DWARF structure.
    DIDumpOptions DumpOpts;
    DumpOpts.DumpType = DwarfDumpType;
    DICtx->dump(outs(), DumpOpts);
  }
  if (Relocations && !Disassemble)
    printRelocations(O);
  if (DynamicRelocations)
    printDynamicRelocations(O);
  if (SectionContents)
    printSectionContents(O);
  if (Disassemble)
    disassembleObject(O, Relocations);
  if (UnwindInfo)
    printUnwindInfo(O);

  // Mach-O specific options:
  if (ExportsTrie)
    printExportsTrie(O);
  if (Rebase)
    printRebaseTable(O);
  if (Bind)
    printBindTable(O);
  if (LazyBind)
    printLazyBindTable(O);
  if (WeakBind)
    printWeakBindTable(O);

  // Other special sections:
  if (RawClangAST)
    printRawClangAST(O);
  if (FaultMapSection)
    printFaultMaps(O);
}

static void dumpObject(const COFFImportFile *I, const Archive *A,
                       const Archive::Child *C = nullptr) {
  StringRef ArchiveName = A ? A->getFileName() : "";

  // Avoid other output when using a raw option.
  /*if (!RawClangAST)
    outs() << '\n'
           << ArchiveName << "(" << I->getFileName() << ")"
           << ":\tfile format COFF-import-file"
           << "\n\n";*/

  if (ArchiveHeaders && !MachOOpt && C)
    printArchiveChild(ArchiveName, *C);
  if (SymbolTable)
    printCOFFSymbolTable(I);
}

/// Dump each object file in \a a;
static void dumpArchive(const Archive *A) {
  Error Err = Error::success();
  unsigned I = -1;
  for (auto &C : A->children(Err)) {
    ++I;
    Expected<std::unique_ptr<Binary>> ChildOrErr = C.getAsBinary();
    if (!ChildOrErr) {
      if (auto E = isNotObjectErrorInvalidFileType(ChildOrErr.takeError()))
        reportError(std::move(E), getFileNameForError(C, I), A->getFileName());
      continue;
    }
    if (ObjectFile *O = dyn_cast<ObjectFile>(&*ChildOrErr.get()))
      dumpObject(O, A, &C);
    else if (COFFImportFile *I = dyn_cast<COFFImportFile>(&*ChildOrErr.get()))
      dumpObject(I, A, &C);
    else
      reportError(errorCodeToError(object_error::invalid_file_type),
                  A->getFileName());
  }
  if (Err)
    reportError(std::move(Err), A->getFileName());
}

/// Open file and figure out how to dump it.
static void dumpInput(StringRef file) {
  // If we are using the Mach-O specific object file parser, then let it parse
  // the file and process the command line options.  So the -arch flags can
  // be used to select specific slices, etc.
  if (MachOOpt) {
    parseInputMachO(file);
    return;
  }

  // Attempt to open the binary.
  OwningBinary<Binary> OBinary = unwrapOrError(createBinary(file), file);
  Binary &Binary = *OBinary.getBinary();

  if (Archive *A = dyn_cast<Archive>(&Binary))
    dumpArchive(A);
  else if (ObjectFile *O = dyn_cast<ObjectFile>(&Binary))
    dumpObject(O);
  else if (MachOUniversalBinary *UB = dyn_cast<MachOUniversalBinary>(&Binary))
    parseInputMachO(UB);
  else
    reportError(errorCodeToError(object_error::invalid_file_type), file);
}
/*
void getdosyadan(){

  if(!dosya.empty()){
    ifstream dos(dosya, fstream::in | fstream::out | fstream::trunc);
    string option = "";
    char character;
    if (dos.is_open()){
      while(dos >> std::noskipws >> character){
        
        if(character != ' ' && character != '\n')
          option += string(1,character);
        else {
          if( option == "add"                ){ add = true; option = ""; }
          if( option == "sub"                ){ sub = true; option = ""; }
        }
      }
    }
  }

}
*/



int main(int argc, char **argv) {

g_argc = argc;
g_argv = argv;



  using namespace llvm;
  InitLLVM X(argc, argv);
  const cl::OptionCategory *OptionFilters[] = {&ObjdumpCat, &MachOCat};
  cl::HideUnrelatedOptions(OptionFilters);
//ekleme
//  if(!dosya.empty()){
//    ifstream dos(dosya);
//    string option = "";
//    char character;
//    if (dos.is_open()){
//      while(dos >> character){
//        if(character='a') add = true;
//        if(character != ' ' || character != '\n')
//          option += string(1,character);
//        else {
//          if( option == "add"                ){ add = true; option = ""; } //else sub=true;
//          if( option == "sub"                ){ sub = true; option = ""; }
//        }
//      }
//    }
//  }
//getdosyadan();
//add = true;
  // Initialize targets and assembly printers/parsers.
  InitializeAllTargetInfos();
  InitializeAllTargetMCs();
  InitializeAllDisassemblers();

  // Register the target printer for --version.
  cl::AddExtraVersionPrinter(TargetRegistry::printRegisteredTargetsForVersion);

  cl::ParseCommandLineOptions(argc, argv, "llvm object file dumper\n", nullptr,
                              /*EnvVar=*/nullptr,
                              /*LongOptionsUseDoubleDash=*/true);

  if (StartAddress >= StopAddress)
    reportCmdLineError("start address should be less than stop address");

  ToolName = argv[0];

int bitArr[90];
for(int i=89;i>=0;i--){
  bitArr[i] = 0;
}

for(int i=0;i<bits.length();i++){
  bitArr[i] = bits[i] - '0';
}

// hepsini baslangicta falsea esitlemeyi dene ya da else kullan.

if(bitArr[0 ] == 1) beq                = true;
if(bitArr[1 ] == 1) bne                = true;
if(bitArr[2 ] == 1) blt                = true;
if(bitArr[3 ] == 1) bge                = true;
if(bitArr[4 ] == 1) bltu               = true;
if(bitArr[5 ] == 1) bgeu               = true;
if(bitArr[6 ] == 1) jalr               = true;
if(bitArr[7 ] == 1) jal                = true;
if(bitArr[8 ] == 1) lui                = true;
if(bitArr[9 ] == 1) auipc              = true;
if(bitArr[10] == 1) addi               = true;
if(bitArr[11] == 1) slli               = true;
if(bitArr[12] == 1) slti               = true;
if(bitArr[13] == 1) sltiu              = true;
if(bitArr[14] == 1) xori               = true;
if(bitArr[15] == 1) srli               = true;
if(bitArr[16] == 1) srai               = true;
if(bitArr[17] == 1) ori                = true;
if(bitArr[18] == 1) andi               = true;
if(bitArr[19] == 1) add                = true;
if(bitArr[20] == 1) sub                = true;
if(bitArr[21] == 1) sll                = true;
if(bitArr[22] == 1) slt                = true;
if(bitArr[23] == 1) sltu               = true;
if(bitArr[24] == 1) xor_               = true; 
if(bitArr[25] == 1) srl                = true;
if(bitArr[26] == 1) sra                = true;
if(bitArr[27] == 1) or_                = true; 
if(bitArr[28] == 1) and_               = true; 
if(bitArr[29] == 1) addiw              = true;
if(bitArr[30] == 1) slliw              = true;
if(bitArr[31] == 1) srliw              = true;
if(bitArr[32] == 1) sraiw              = true;
if(bitArr[33] == 1) addw               = true;
if(bitArr[34] == 1) subw               = true;
if(bitArr[35] == 1) sllw               = true;
if(bitArr[36] == 1) srlw               = true;
if(bitArr[37] == 1) sraw               = true;
if(bitArr[38] == 1) lb                 = true;
if(bitArr[39] == 1) lh                 = true;
if(bitArr[40] == 1) lw                 = true;
if(bitArr[41] == 1) ld                 = true;
if(bitArr[42] == 1) lbu                = true;
if(bitArr[43] == 1) lhu                = true;
if(bitArr[44] == 1) lwu                = true;
if(bitArr[45] == 1) sb                 = true;
if(bitArr[46] == 1) sh                 = true;
if(bitArr[47] == 1) sw                 = true;
if(bitArr[48] == 1) sd                 = true;
if(bitArr[49] == 1) fence              = true;
if(bitArr[50] == 1) fence_i            = true;
if(bitArr[51] == 1) mul                = true;
if(bitArr[52] == 1) mulh               = true;
if(bitArr[53] == 1) mulhsu             = true;
if(bitArr[54] == 1) mulhu              = true;
if(bitArr[55] == 1) objdump::div       = true; // div is ambigious hatasindan dolayi
if(bitArr[56] == 1) divu               = true;
if(bitArr[57] == 1) rem                = true;
if(bitArr[58] == 1) remu               = true;
if(bitArr[59] == 1) mulw               = true;
if(bitArr[60] == 1) divw               = true;
if(bitArr[61] == 1) divuw              = true;
if(bitArr[62] == 1) remw               = true;
if(bitArr[63] == 1) remuw              = true;
if(bitArr[64] == 1) lr_w               = true;
if(bitArr[65] == 1) sc_w               = true;
if(bitArr[66] == 1) lr_d               = true;
if(bitArr[67] == 1) sc_d               = true;
if(bitArr[68] == 1) ecall              = true;
if(bitArr[69] == 1) ebreak             = true;
if(bitArr[70] == 1) uret               = true;
if(bitArr[71] == 1) mret               = true;
if(bitArr[72] == 1) dret               = true;
if(bitArr[73] == 1) sfence_vma         = true;
if(bitArr[74] == 1) wfi                = true;
if(bitArr[75] == 1) csrrw              = true;
if(bitArr[76] == 1) csrrs              = true;
if(bitArr[77] == 1) csrrc              = true;
if(bitArr[78] == 1) csrrwi             = true;
if(bitArr[79] == 1) csrrsi             = true;
if(bitArr[80] == 1) csrrci             = true;
if(bitArr[81] == 1) slli_rv32          = true;
if(bitArr[82] == 1) srli_rv32          = true;
if(bitArr[83] == 1) srai_rv32          = true;
if(bitArr[84] == 1) rdcycle            = true;
if(bitArr[85] == 1) rdtime             = true;
if(bitArr[86] == 1) rdinstret          = true;
if(bitArr[87] == 1) rdcycleh           = true;
if(bitArr[88] == 1) rdtimeh            = true;
if(bitArr[89] == 1) rdinstreth         = true;

  // Defaults to a.out if no filenames specified.
  if (InputFilenames.empty())
    InputFilenames.push_back("a.out");

  if (AllHeaders)
    ArchiveHeaders = FileHeaders = PrivateHeaders = Relocations =
        SectionHeaders = SymbolTable = true;

  if (DisassembleAll || PrintSource || PrintLines ||
      !DisassembleSymbols.empty())
    Disassemble = true;

  if (!ArchiveHeaders && !Disassemble && DwarfDumpType == DIDT_Null &&
      !DynamicRelocations && !FileHeaders && !PrivateHeaders && !RawClangAST &&
      !Relocations && !SectionHeaders && !SectionContents && !SymbolTable &&
      !DynamicSymbolTable && !UnwindInfo && !FaultMapSection &&
      !(MachOOpt &&
        (Bind || DataInCode || DylibId || DylibsUsed || ExportsTrie ||
         FirstPrivateHeader || IndirectSymbols || InfoPlist || LazyBind ||
         LinkOptHints || ObjcMetaData || Rebase || UniversalHeaders ||
         WeakBind || !FilterSections.empty()))) {
    cl::PrintHelpMessage();
    return 2;
  }

  DisasmSymbolSet.insert(DisassembleSymbols.begin(), DisassembleSymbols.end());

  llvm::for_each(InputFilenames, dumpInput);

  warnOnNoMatchForSections();
OSS<<""; // mainde boş da olsa bişey yazmazsam yapmadı.
  //OSS<<"afasdasd"; OSS<<" ";
OSS.close();

  return EXIT_SUCCESS;
}
// Inputfilenames e vector olarak davran ismi almaya calis
// digerlerini kaptmayip onlari da dosya yazdirmasına dahil etmek gerek

// printrelocations da %08 ve %016 7 ve 15 boşluk basıyor hexten önce
