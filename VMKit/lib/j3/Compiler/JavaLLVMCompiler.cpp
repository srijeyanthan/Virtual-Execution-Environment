//===-------- JavaLLVMCompiler.cpp - A LLVM Compiler for J3 ---------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/PassManager.h"
#include "llvm/DIBuilder.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/DataLayout.h"

#include "vmkit/JIT.h"

#include "JavaClass.h"
#include "JavaJIT.h"

#include "j3/JavaLLVMCompiler.h"

using namespace llvm;

namespace j3 {

JavaLLVMCompiler::JavaLLVMCompiler(const std::string& str,
		bool compiling_garbage_collector) :
		JavaCompiler(compiling_garbage_collector), TheModule(
				new llvm::Module(str, *(new LLVMContext()))), DebugFactory(
				new DIBuilder(*TheModule)) {

	enabledException = true;
	cooperativeGC = true;
}

void JavaLLVMCompiler::resolveVirtualClass(Class* cl) {
	// Lock here because we may be called by a class resolver
	vmkit::VmkitModule::protectIR();
	LLVMClassInfo* LCI = (LLVMClassInfo*) getClassInfo(cl);
	LCI->getVirtualType();
	vmkit::VmkitModule::unprotectIR();
}

void JavaLLVMCompiler::resolveStaticClass(Class* cl) {
	// Lock here because we may be called by a class initializer
	vmkit::VmkitModule::protectIR();
	LLVMClassInfo* LCI = (LLVMClassInfo*) getClassInfo(cl);
	LCI->getStaticType();
	vmkit::VmkitModule::unprotectIR();
}

Function* JavaLLVMCompiler::getMethod(JavaMethod* meth, Class* customizeFor) {
	return getMethodInfo(meth)->getMethod(customizeFor);
}

Function* JavaLLVMCompiler::parseModifiedFunction(JavaMethod* meth,
		Class* customizeFor, int Level) {
	assert(!isAbstract(meth->access));
	LLVMMethodInfo* LMI = getMethodInfo(meth);
	const vmkit::UTF8 * methodname = meth->getMehtodName();

    	LMI->isCustomizable =true;
    	LMI->clear();
	Function* func = LMI->getMethod(customizeFor);



	// We are jitting. Take the lock.
	vmkit::VmkitModule::protectIR();

	if (func->getLinkage() == GlobalValue::ExternalWeakLinkage) {
		JavaJIT jit(this, meth, func,LMI->isCustomizable ? customizeFor : NULL);
		if (isNative(meth->access)) {
			jit.nativeCompile();
			vmkit::VmkitModule::runPasses(func, JavaNativeFunctionPasses);
			vmkit::VmkitModule::runPasses(func, J3FunctionPasses);
		} else {
			jit.javaCompile();
			if (Level == 1){
				vmkit::VmkitModule::runPasses(func, JavaFunctionPassesLevel1);
				std::cout<<"[LLVM Compiler] Compiling with Level 1 passes !!!!!!!!!!!!!! "<<std::endl;
			}
			else if (Level == 2)
			{
				vmkit::VmkitModule::runPasses(func, JavaFunctionPassesLevel2);
				std::cout<<"[LLVM Compiler] Compiling with Level 2 passes  !!!!!!!!!!!!!!"<<std::endl;
			}

			vmkit::VmkitModule::runPasses(func, J3FunctionPasses);

		}

			func->setLinkage(GlobalValue::ExternalWeakLinkage);



		if (!LMI->isCustomizable && jit.isCustomizable) {
			// It's the first time we parsed the method and we just found
			// out it can be customized.
			// TODO(geoffray): return a customized version to this caller.
			meth->isCustomizable = true;
			LMI->isCustomizable = true;
		}
	}
	vmkit::VmkitModule::unprotectIR();

	return func;
}
Function* JavaLLVMCompiler::parseFunction(JavaMethod* meth,
		Class* customizeFor) {
	assert(!isAbstract(meth->access));
	LLVMMethodInfo* LMI = getMethodInfo(meth);
	const vmkit::UTF8 * methodname = meth->getMehtodName();
	int diff = methodname->compare("Calc");
    if(diff==0)
    {
    	LMI->isCustomizable =true;
    	LMI->clear();
    }
	//LMI->clear()
	Function* func = LMI->getMethod(customizeFor);



	// We are jitting. Take the lock.
	vmkit::VmkitModule::protectIR();
	/*if (func->getLinkage() == GlobalValue::ExternalWeakLinkage) {
		std::cout << "method name - " << *meth->getMehtodName()
				<< "|ExternalWeakLinkage" << std::endl;
	} else
		std::cout << "method name - " << *meth->getMehtodName()
				<< "|ExternalLinkage" << std::endl;*/

	if (func->getLinkage() == GlobalValue::ExternalWeakLinkage) {
		JavaJIT jit(this, meth, func,LMI->isCustomizable ? customizeFor : NULL);
		if (isNative(meth->access)) {
			jit.nativeCompile();
			vmkit::VmkitModule::runPasses(func, JavaNativeFunctionPasses);
			vmkit::VmkitModule::runPasses(func, J3FunctionPasses);
		} else {
			jit.javaCompile();
			vmkit::VmkitModule::runPasses(func, JavaFunctionPasses);
			vmkit::VmkitModule::runPasses(func, J3FunctionPasses);

		}

		if (diff == 0) {
			std::cout << "javallvmcompiler - method Calc ---   - "
					<< *meth->getMehtodName() << std::endl;
			func->setLinkage(GlobalValue::ExternalWeakLinkage);


		} else
			func->setLinkage(GlobalValue::ExternalLinkage);

		if (!LMI->isCustomizable && jit.isCustomizable) {
			// It's the first time we parsed the method and we just found
			// out it can be customized.
			// TODO(geoffray): return a customized version to this caller.
			//std::cout << "s______________________Checking is customizalble or not __________"<< *meth->getMehtodName() << std::endl;
			meth->isCustomizable = true;
			LMI->isCustomizable = true;
		}
	}
	vmkit::VmkitModule::unprotectIR();

	return func;
}

JavaMethod* JavaLLVMCompiler::getJavaMethod(const llvm::Function& F) {
	function_iterator E = functions.end();
	function_iterator I = functions.find(&F);
	if (I == E)
		return 0;
	return I->second;
}

JavaLLVMCompiler::~JavaLLVMCompiler() {
	LLVMContext* Context = &(TheModule->getContext());
	delete TheModule;
	delete DebugFactory;
	delete JavaFunctionPasses;
	delete J3FunctionPasses;
	delete JavaNativeFunctionPasses;
	delete Context;
}

llvm::FunctionPass* createLowerConstantCallsPass(JavaLLVMCompiler* I);

void JavaLLVMCompiler::addJavaPasses() {
	JavaNativeFunctionPasses = new FunctionPassManager(TheModule);
	JavaNativeFunctionPasses->add(new DataLayout(TheModule));

	J3FunctionPasses = new FunctionPassManager(TheModule);
	J3FunctionPasses->add(createLowerConstantCallsPass(this));

	JavaFunctionPasses = new FunctionPassManager(TheModule);
	JavaFunctionPasses->add(new DataLayout(TheModule));
	vmkit::VmkitModule::addCommandLinePasses(JavaFunctionPasses, 0);

	JavaFunctionPassesLevel1 = new FunctionPassManager(TheModule);
	JavaFunctionPassesLevel1->add(new DataLayout(TheModule));
	vmkit::VmkitModule::addCommandLinePasses(JavaFunctionPassesLevel1, 1);

	JavaFunctionPassesLevel2 = new FunctionPassManager(TheModule);
	JavaFunctionPassesLevel2->add(new DataLayout(TheModule));
	vmkit::VmkitModule::addCommandLinePasses(JavaFunctionPassesLevel2, 2);
}

} // end namespace j3
