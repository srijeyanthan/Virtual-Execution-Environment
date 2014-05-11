//===------------- Main.cpp - Simple execution of J3 ----------------------===//
//
//                          The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "VmkitGC.h"
#include "vmkit/JIT.h"
#include "vmkit/MethodInfo.h"
#include "vmkit/VirtualMachine.h"
#include "vmkit/Thread.h"

#include "j3/JavaJITCompiler.h"
#include "../../lib/j3/VMCore/JnjvmClassLoader.h"
#include "../../lib/j3/VMCore/Jnjvm.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"

using namespace j3;
using namespace vmkit;
int main(int argc, char **argv, char **envp) {
	llvm::llvm_shutdown_obj X;

	std::string commandstring = "java -jar JavaTransform.jar classpath/ ";
	std::string stringclassname(argv[1]);
	commandstring += stringclassname + " .";
	std::cout<<"[J3] Byte code modification is taking place "<<commandstring<<std::endl;
	system(commandstring.c_str());

	// Initialize base components.

	// modification in java byte code -- forefullt we can add dummy
	VmkitModule::initialise(argc, argv);
	Collector::initialise(argc, argv);

	// Create the allocator that will allocate the bootstrap loader and the JVM.
	vmkit::BumpPtrAllocator Allocator;
	JavaJITCompiler* Comp = JavaJITCompiler::CreateCompiler("JITModule",1);
	Comp->StartProfilerThread();
	JnjvmBootstrapLoader* loader =
			new (Allocator, "Bootstrap loader") JnjvmBootstrapLoader(Allocator,
					Comp, true);
	Jnjvm* vm = new (Allocator, "VM") Jnjvm(Allocator, initialFrametables,
			loader);

	// Run the application.
	vm->runApplication(argc, argv);
	vm->waitForExit();
	System::Exit(0);

	// Destroy everyone.
	// vm->~Jnjvm();
	// loader->~JnjvmBootstrapLoader();

	return 0;
}
