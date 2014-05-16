//===-------- InlineMethods.cpp - Initialize the inline methods -----------===//
//
//                      The VMKit project
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Type.h>

using namespace llvm;

namespace mmtk {

namespace mmtk_vt_malloc {
  #include "MMTkVTMallocInline.inc"
}

namespace mmtk_vmkit_malloc {
  #include "MMTkVMKitMallocInline.inc"
}

namespace mmtk_array_write {
  #include "MMTkArrayWriteInline.inc"
}

namespace mmtk_field_write {
  #include "MMTkFieldWriteInline.inc"
}

namespace mmtk_non_heap_write {
  #include "MMTkNonHeapWriteInline.inc"
}

}

extern "C" void MMTk_InlineMethods(llvm::Module* module) {
  mmtk::mmtk_vt_malloc::makeLLVMFunction(module);
  mmtk::mmtk_vmkit_malloc::makeLLVMFunction(module);
  mmtk::mmtk_field_write::makeLLVMFunction(module);
  mmtk::mmtk_array_write::makeLLVMFunction(module);
  mmtk::mmtk_non_heap_write::makeLLVMFunction(module);
}

namespace vmkit {
	extern "C" void vmkit::makeLLVMFunctions_FinalMMTk(llvm::Module* module) {
		MMTk_InlineMethods(module);
	}
}
