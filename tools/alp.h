//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVM_OBJDUMP_LLVM_OBJDUMP_H
#define LLVM_TOOLS_LLVM_OBJDUMP_LLVM_OBJDUMP_H

#include "llvm/ADT/StringSet.h"
#include "llvm/DebugInfo/DIContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/Object/Archive.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/DataTypes.h"

namespace llvm {
class StringRef;

namespace object {
class ELFObjectFileBase;
class ELFSectionRef;
class MachOObjectFile;
class MachOUniversalBinary;
class RelocationRef;
} // namespace object

namespace objdump {

extern cl::opt<bool> ArchiveHeaders;
extern cl::opt<bool> Demangle;
extern cl::opt<bool> Disassemble;
extern cl::opt<bool> DisassembleAll;
extern cl::opt<DIDumpType> DwarfDumpType;
extern cl::list<std::string> FilterSections;
extern cl::list<std::string> MAttrs;
extern cl::opt<std::string> MCPU;
extern cl::opt<bool> NoShowRawInsn;
extern cl::opt<bool> NoLeadingAddr;
extern cl::opt<bool> PrintImmHex;
extern cl::opt<bool> PrivateHeaders;
extern cl::opt<bool> Relocations;
extern cl::opt<bool> SectionHeaders;
extern cl::opt<bool> SectionContents;
extern cl::opt<bool> SymbolDescription;
extern cl::opt<bool> SymbolTable;
extern cl::opt<std::string> TripleName;
extern cl::opt<bool> UnwindInfo;

/////////////ekleme
//degistirdim
// xor, or ve and icin _ eklemek zorundaydım.
extern cl::opt<bool> beq               ;
extern cl::opt<bool> bne               ;
extern cl::opt<bool> blt               ;
extern cl::opt<bool> bge               ;
extern cl::opt<bool> bltu              ;
extern cl::opt<bool> bgeu              ;
extern cl::opt<bool> jalr              ;
extern cl::opt<bool> jal               ;
extern cl::opt<bool> lui               ;
extern cl::opt<bool> auipc             ;
extern cl::opt<bool> addi              ;
extern cl::opt<bool> slli              ;
extern cl::opt<bool> slti              ;
extern cl::opt<bool> sltiu             ;
extern cl::opt<bool> xori              ;
extern cl::opt<bool> srli              ;
extern cl::opt<bool> srai              ;
extern cl::opt<bool> ori               ;
extern cl::opt<bool> andi              ;
extern cl::opt<bool> add               ;
extern cl::opt<bool> sub               ;
extern cl::opt<bool> sll               ;
extern cl::opt<bool> slt               ;
extern cl::opt<bool> sltu              ;
extern cl::opt<bool> xor_               ;
extern cl::opt<bool> srl               ;
extern cl::opt<bool> sra               ;
extern cl::opt<bool> or_                ;
extern cl::opt<bool> and_               ;
extern cl::opt<bool> addiw             ;
extern cl::opt<bool> slliw             ;
extern cl::opt<bool> srliw             ;
extern cl::opt<bool> sraiw             ;
extern cl::opt<bool> addw              ;
extern cl::opt<bool> subw              ;
extern cl::opt<bool> sllw              ;
extern cl::opt<bool> srlw              ;
extern cl::opt<bool> sraw              ;
extern cl::opt<bool> lb                ;
extern cl::opt<bool> lh                ;
extern cl::opt<bool> lw                ;
extern cl::opt<bool> ld                ;
extern cl::opt<bool> lbu               ;
extern cl::opt<bool> lhu               ;
extern cl::opt<bool> lwu               ;
extern cl::opt<bool> sb                ;
extern cl::opt<bool> sh                ;
extern cl::opt<bool> sw                ;
extern cl::opt<bool> sd                ;
extern cl::opt<bool> fence             ;
extern cl::opt<bool> fence_i           ;
extern cl::opt<bool> mul               ;
extern cl::opt<bool> mulh              ;
extern cl::opt<bool> mulhsu            ;
extern cl::opt<bool> mulhu             ;
extern cl::opt<bool> div               ;
extern cl::opt<bool> divu              ;
extern cl::opt<bool> rem               ;
extern cl::opt<bool> remu              ;
extern cl::opt<bool> mulw              ;
extern cl::opt<bool> divw              ;
extern cl::opt<bool> divuw             ;
extern cl::opt<bool> remw              ;
extern cl::opt<bool> remuw             ;
extern cl::opt<bool> lr_w              ;
extern cl::opt<bool> sc_w              ;
extern cl::opt<bool> lr_d              ;
extern cl::opt<bool> sc_d              ;
extern cl::opt<bool> ecall             ;
extern cl::opt<bool> ebreak            ;
extern cl::opt<bool> uret              ;
extern cl::opt<bool> mret              ;
extern cl::opt<bool> dret              ;
extern cl::opt<bool> sfence_vma        ;
extern cl::opt<bool> wfi               ;
extern cl::opt<bool> csrrw             ;
extern cl::opt<bool> csrrs             ;
extern cl::opt<bool> csrrc             ;
extern cl::opt<bool> csrrwi            ;
extern cl::opt<bool> csrrsi            ;
extern cl::opt<bool> csrrci            ;
//extern cl::opt<bool> custom0           ;
//extern cl::opt<bool> custom0_rs1       ;
//extern cl::opt<bool> custom0_rs1_rs2   ;
//extern cl::opt<bool> custom0_rd        ;
//extern cl::opt<bool> custom0_rd_rs1    ;
//extern cl::opt<bool> custom0_rd_rs1_rs2;
//extern cl::opt<bool> custom1           ;
//extern cl::opt<bool> custom1_rs1       ;
//extern cl::opt<bool> custom1_rs1_rs2   ;
//extern cl::opt<bool> custom1_rd        ;
//extern cl::opt<bool> custom1_rd_rs1    ;
//extern cl::opt<bool> custom1_rd_rs1_rs2;
//extern cl::opt<bool> custom2           ;
//extern cl::opt<bool> custom2_rs1       ;
//extern cl::opt<bool> custom2_rs1_rs2   ;
//extern cl::opt<bool> custom2_rd        ;
//extern cl::opt<bool> custom2_rd_rs1    ;
//extern cl::opt<bool> custom2_rd_rs1_rs2;
//extern cl::opt<bool> custom3           ;
//extern cl::opt<bool> custom3_rs1       ;
//extern cl::opt<bool> custom3_rs1_rs2   ;
//extern cl::opt<bool> custom3_rd        ;
//extern cl::opt<bool> custom3_rd_rs1    ;
//extern cl::opt<bool> custom3_rd_rs1_rs2;
extern cl::opt<bool> slli_rv32         ;
extern cl::opt<bool> srli_rv32         ;
extern cl::opt<bool> srai_rv32         ;
extern cl::opt<bool> rdcycle           ;
extern cl::opt<bool> rdtime            ;
extern cl::opt<bool> rdinstret         ;
extern cl::opt<bool> rdcycleh          ;
extern cl::opt<bool> rdtimeh           ;
extern cl::opt<bool> rdinstreth        ;

extern cl::opt<std::string> dosya;
extern cl::opt<std::string> bits;

extern StringSet<> FoundSectionSet;

typedef std::function<bool(llvm::object::SectionRef const &)> FilterPredicate;

/// A filtered iterator for SectionRefs that skips sections based on some given
/// predicate.
class SectionFilterIterator {
public:
  SectionFilterIterator(FilterPredicate P,
                        llvm::object::section_iterator const &I,
                        llvm::object::section_iterator const &E)
      : Predicate(std::move(P)), Iterator(I), End(E) {
    ScanPredicate();
  }
  const llvm::object::SectionRef &operator*() const { return *Iterator; }
  SectionFilterIterator &operator++() {
    ++Iterator;
    ScanPredicate();
    return *this;
  }
  bool operator!=(SectionFilterIterator const &Other) const {
    return Iterator != Other.Iterator;
  }

private:
  void ScanPredicate() {
    while (Iterator != End && !Predicate(*Iterator)) {
      ++Iterator;
    }
  }
  FilterPredicate Predicate;
  llvm::object::section_iterator Iterator;
  llvm::object::section_iterator End;
};

/// Creates an iterator range of SectionFilterIterators for a given Object and
/// predicate.
class SectionFilter {
public:
  SectionFilter(FilterPredicate P, llvm::object::ObjectFile const &O)
      : Predicate(std::move(P)), Object(O) {}
  SectionFilterIterator begin() {
    return SectionFilterIterator(Predicate, Object.section_begin(),
                                 Object.section_end());
  }
  SectionFilterIterator end() {
    return SectionFilterIterator(Predicate, Object.section_end(),
                                 Object.section_end());
  }

private:
  FilterPredicate Predicate;
  llvm::object::ObjectFile const &Object;
};

// Various helper functions.

/// Creates a SectionFilter with a standard predicate that conditionally skips
/// sections when the --section objdump flag is provided.
///
/// Idx is an optional output parameter that keeps track of which section index
/// this is. This may be different than the actual section number, as some
/// sections may be filtered (e.g. symbol tables).
SectionFilter ToolSectionFilter(llvm::object::ObjectFile const &O,
                                uint64_t *Idx = nullptr);

bool isRelocAddressLess(object::RelocationRef A, object::RelocationRef B);
void printRelocations(const object::ObjectFile *O);
void printDynamicRelocations(const object::ObjectFile *O);
void printSectionHeaders(const object::ObjectFile *O);
void printSectionContents(const object::ObjectFile *O);
void printSymbolTable(const object::ObjectFile *O, StringRef ArchiveName,
                      StringRef ArchitectureName = StringRef(),
                      bool DumpDynamic = false);
void printSymbol(const object::ObjectFile *O, const object::SymbolRef &Symbol,
                 StringRef FileName, StringRef ArchiveName,
                 StringRef ArchitectureName, bool DumpDynamic);
LLVM_ATTRIBUTE_NORETURN void reportError(StringRef File, Twine Message);
LLVM_ATTRIBUTE_NORETURN void reportError(Error E, StringRef FileName,
                                         StringRef ArchiveName = "",
                                         StringRef ArchitectureName = "");
void reportWarning(Twine Message, StringRef File);

template <typename T, typename... Ts>
T unwrapOrError(Expected<T> EO, Ts &&... Args) {
  if (EO)
    return std::move(*EO);
  reportError(EO.takeError(), std::forward<Ts>(Args)...);
}

std::string getFileNameForError(const object::Archive::Child &C,
                                unsigned Index);
SymbolInfoTy createSymbolInfo(const object::ObjectFile *Obj,
                              const object::SymbolRef &Symbol);

} // namespace objdump
} // end namespace llvm

#endif
