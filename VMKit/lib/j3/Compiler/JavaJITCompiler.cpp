//===--------- JavaJITCompiler.cpp - Support for JIT compiling -------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/DebugInfo.h"
#include "llvm/CodeGen/GCStrategy.h"
#include <llvm/CodeGen/JITCodeEmitter.h>
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DataLayout.h"
#include <lib/ExecutionEngine/JIT/JIT.h>
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include "VmkitGC.h"
#include "vmkit/VirtualMachine.h"

#include "JavaClass.h"
#include "JavaConstantPool.h"
#include "JavaThread.h"
#include "JavaTypes.h"
#include "Jnjvm.h"

#include "j3/JavaJITCompiler.h"
#include "j3/J3Intrinsics.h"
#include "j3/GlobalHeader.h"
using namespace j3;
using namespace llvm;

Function *globalFptr;
bool isThisJ3Compilation =false;
void * globalEmittedFunctionPointer;
// create global map
size_t globalcodesize=0;
std::map < std::string , Patch * >  patchMap ;
void JavaJITListener::NotifyFunctionEmitted(const Function &F, void *Code,
		size_t Size, const EmittedFunctionDetails &Details) {

	assert(F.hasGC());
	if (TheCompiler->GCInfo == NULL) {
		TheCompiler->GCInfo = Details.MF->getGMI();

	}
	if(isThisJ3Compilation)
	{
		globalcodesize =Size;
		globalEmittedFunctionPointer= Code;
		//Details.MF->dump();
	}
	assert(TheCompiler->GCInfo == Details.MF->getGMI());

}
ProfilerThread::ProfilerThread(std::map<std::string, Patch *> &patchMap) :
		m_patchMap(patchMap) {
	pthread_mutexattr_init(&mta);
	pthread_mutex_init(&m_mutex, &mta);
}

ProfilerThread::~ProfilerThread() {
	pthread_mutexattr_destroy(&mta);
	pthread_mutex_destroy(&m_mutex);
}
void ProfilerThread::start() {

	m_profilerstarttime = rdtsc();
	pthread_create(&tid, 0, helper, this);
}
void ProfilerThread::join() {
	int* status = 0;
	pthread_join(tid, (void**) &status);
}

void ProfilerThread::setJavaJITCompilerPtr(void *ptr) {
	m_JavaJITCompilerPtr = ptr;
}
void ProfilerThread::run() {
	while (true) {
		for (std::map<std::string, Patch*>::iterator itr = m_patchMap.begin();
				itr != m_patchMap.end(); ++itr) {

			pthread_mutex_lock(&m_mutex);
			void *FPtr = itr->second->llvmGlobalVariableMachinePointer;
			// Cast it to the right type (takes no arguments, returns a double) so we
			// can call it as a native function.
			int (*FP)() = (int (*)())(intptr_t)FPtr;
			int globalval = FP();
			if (globalval > 0
					&& globalval != itr->second->previousglobalvalue) {
				itr->second->previousglobalvalue = globalval;
				double avgtimeSec = 1.0
						* (rdtsc() - itr->second->previousexetime) / 3199987.0
						/ 1000.0;  // this is second
				itr->second->avgexetime += avgtimeSec;
				itr->second->previousexetime = rdtsc();
				itr->second->samplingcount += 1;
				std::cout<<"[VMKIT] INFO Gathering - Function name - "<<itr->first<<" |Global value - "<<FP() <<" Avg Time - "<<itr->second->avgexetime<<std::endl;
				/*fprintf(stderr,
						"___Global variable reached _____  function name - %s - %d - %.5f - %d\n",
						itr->first.c_str(),FP(), itr->second->avgexetime,
						itr->second->samplingcount);*/
			}
			pthread_mutex_unlock(&m_mutex);

		}

		int l_timegap = 1.0 * (rdtsc() - m_profilerstarttime) / 3199987.0
				/ 1000.0;
		if (l_timegap > 20) {
			m_profilerstarttime = rdtsc();
			std::cout << "******************** Functions Statistics **************************************"
					<< std::endl;
			for (std::map<std::string, Patch*>::iterator itr =
					m_patchMap.begin(); itr != m_patchMap.end(); ++itr) {
				pthread_mutex_lock(&m_mutex);
				int frequency = itr->second->avgexetime
						/ itr->second->samplingcount;
				std::cout <<"Function name - "<<itr->first<<"| Avg exe time - " << itr->second->avgexetime
						<< "|sampling count - " << itr->second->samplingcount<< "|Frequency - "<< frequency << " call/sec" << std::endl;
				if(!itr->second->isOptimizedDone)
				{
				((JavaJITCompiler*) m_JavaJITCompilerPtr)->patchmaterializeFunction(
						itr->second->codePointer, itr->second->meth,
						itr->second->cusotmizedFor);
				}
				void *FPtr = itr->second->llvmGlobalVariableMachinePointer;
				// Cast it to the right type (takes no arguments, returns a double) so we
				// can call it as a native function.
				int (*FP)() = (int (*)())(intptr_t)FPtr;
				void *pTempPtr = itr->second->llvmGlobalVariableTailPointer;
				int (*FPTeil)() = (int (*)())(intptr_t)pTempPtr;
				int tailvalue =FPTeil();
				int previousglobalvalue = itr->second->previousglobalvalue;
				if(FP() ==tailvalue && (itr->second->isOptimizedDone) && !(itr->second->isFunctionReplaced))
				{
				  itr->second->isFunctionReplaced=true;
				  memcpy(itr->second->codePointer, itr->second->llvmOptimzedFunctionPointer,itr->second->Size);
                  std::cout<<"[VMKIT] New optimized function has placed in the memory "<<itr->first<<std::endl;
				}else
				{
					std::cout<<"____ Function is busy/replaced before , can not replace the optimized function _______"<<itr->first<<std::endl;
				}
				pthread_mutex_unlock(&m_mutex);
			}
		}
		//dont let the loop sleep now .ask him to work now .
		sleep(5);
	}
}
unsigned long long ProfilerThread::rdtsc() {
	unsigned a, d;

	__asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

	return ((unsigned long long) a) | (((unsigned long long) d) << 32);
}

void* ProfilerThread::helper(void* arg) {
	ProfilerThread* l_profThread = reinterpret_cast<ProfilerThread*>(arg);
	l_profThread->run();
	pthread_exit(0);
}

Constant* JavaJITCompiler::getNativeClass(CommonClass* classDef) {
	Type* Ty =
			classDef->isClass() ?
					JavaIntrinsics.JavaClassType :
					JavaIntrinsics.JavaCommonClassType;

	ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
			uint64_t(classDef));
	return ConstantExpr::getIntToPtr(CI, Ty);
}

Constant* JavaJITCompiler::getResolvedConstantPool(JavaConstantPool* ctp) {
	void* ptr = ctp->ctpRes;
	assert(ptr && "No constant pool found");
	ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
			uint64_t(ptr));
	return ConstantExpr::getIntToPtr(CI,
			JavaIntrinsics.ResolvedConstantPoolType);
}

Constant* JavaJITCompiler::getMethodInClass(JavaMethod* meth) {
	ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
			(int64_t) meth);
	return ConstantExpr::getIntToPtr(CI, JavaIntrinsics.JavaMethodType);
}

Constant* JavaJITCompiler::getString(JavaString* str) {
	llvm_gcroot(str, 0);
	fprintf(stderr, "Should not be here\n");
	abort();
}

Constant* JavaJITCompiler::getStringPtr(JavaString** str) {
	assert(str && "No string given");
	Type* Ty = PointerType::getUnqual(JavaIntrinsics.JavaObjectType);
	ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
			uint64(str));
	return ConstantExpr::getIntToPtr(CI, Ty);
}

Constant* JavaJITCompiler::getJavaClass(CommonClass* cl) {
	fprintf(stderr, "Should not be here\n");
	abort();
}

Constant* JavaJITCompiler::getJavaClassPtr(CommonClass* cl) {
	Jnjvm* vm = JavaThread::get()->getJVM();
	JavaObject* const * obj = cl->getClassDelegateePtr(vm);
	assert(obj && "Delegatee not created");
	Constant* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
			uint64(obj));
	Type* Ty = PointerType::getUnqual(JavaIntrinsics.JavaObjectType);
	return ConstantExpr::getIntToPtr(CI, Ty);
}

JavaObject* JavaJITCompiler::getFinalObject(llvm::Value* obj) {
	// obj can not encode direclty an object.
	return NULL;
}

Constant* JavaJITCompiler::getFinalObject(JavaObject* obj, CommonClass* cl) {
	llvm_gcroot(obj, 0);
	return NULL;
}

Constant* JavaJITCompiler::getStaticInstance(Class* classDef) {
	void* obj = classDef->getStaticInstance();
	if (!obj) {
		classDef->acquire();
		obj = classDef->getStaticInstance();
		if (!obj) {
			// Allocate now so that compiled code can reference it.
			obj = classDef->allocateStaticInstance(JavaThread::get()->getJVM());
		}
		classDef->release();
	}
	Constant* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
			(uint64_t(obj)));
	return ConstantExpr::getIntToPtr(CI, JavaIntrinsics.ptrType);
}

Constant* JavaJITCompiler::getVirtualTable(JavaVirtualTable* VT) {
	if (VT->cl->isClass()) {
		LLVMClassInfo* LCI = getClassInfo(VT->cl->asClass());
		LCI->getVirtualType();
	}

	ConstantInt* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
			uint64_t(VT));
	return ConstantExpr::getIntToPtr(CI, JavaIntrinsics.VTType);
}

Constant* JavaJITCompiler::getNativeFunction(JavaMethod* meth, void* ptr) {
	LLVMSignatureInfo* LSI = getSignatureInfo(meth->getSignature());
	Type* valPtrType = LSI->getNativePtrType();

	assert(ptr && "No native function given");

	Constant* CI = ConstantInt::get(Type::getInt64Ty(getLLVMContext()),
			uint64_t(ptr));
	return ConstantExpr::getIntToPtr(CI, valPtrType);
}

void JavaJITCompiler::Test()
{
	// Create some module to put our function into it.

	  // Create the add1 function entry and insert this entry into module M.  The
	  // function will have a return type of "int" and take an argument of "int".
	  // The '0' terminates the list of argument types.
	  Function *Add1F =
	    cast<Function>(TheModule->getOrInsertFunction("add1", Type::getInt32Ty(getLLVMContext()),
	                                          Type::getInt32Ty(getLLVMContext()),
	                                          (Type *)0));


	  // Add a basic block to the function. As before, it automatically inserts
	  // because of the last argument.
	  BasicBlock *BB = BasicBlock::Create(getLLVMContext(), "EntryBlock", Add1F);

	  // Create a basic block builder with default parameters.  The builder will
	  // automatically append instructions to the basic block `BB'.
	  IRBuilder<> builder(BB);

	  // Get pointers to the constant `1'.
	  Value *One = builder.getInt32(1);

	  // Get pointers to the integer argument of the add1 function...
	  assert(Add1F->arg_begin() != Add1F->arg_end()); // Make sure there's an arg
	  Argument *ArgX = Add1F->arg_begin();  // Get the arg
	  ArgX->setName("AnArg");            // Give it a nice symbolic name for fun.

	  // Create the add instruction, inserting it into the end of BB.
	  Value *Add = builder.CreateAdd(One, ArgX);

	  // Create the return instruction and add it to the basic block
	  builder.CreateRet(Add);

	  // Now, function add1 is ready.


	  // Now we're going to create function `foo', which returns an int and takes no
	  // arguments.
	  Function *FooF =
	    cast<Function>(TheModule->getOrInsertFunction("foo", Type::getInt32Ty(getLLVMContext()),
	                                          (Type *)0));

	  // Add a basic block to the FooF function.
	  BB = BasicBlock::Create(getLLVMContext(), "EntryBlock", FooF);

	  // Tell the basic block builder to attach itself to the new basic block
	  builder.SetInsertPoint(BB);

	  // Get pointer to the constant `10'.
	  Value *Ten = builder.getInt32(10);

	  // Pass Ten to the call to Add1F
	  CallInst *Add1CallRes = builder.CreateCall(Add1F, Ten);
	  Add1CallRes->setTailCall(true);

	  // Create the return instruction and add it to the basic block.
	  builder.CreateRet(Add1CallRes);


	  // Now we create the JIT.
	 ExecutionEngine* EE = EngineBuilder(TheModule).create();

	  outs() << "We just constructed this LLVM module:\n\n" << *TheModule;
	  outs() << "\n\nRunning foo: ";
	  outs().flush();

	  // Call the `foo' function with no arguments:
	  std::vector<GenericValue> noargs;
	  GenericValue gv = executionEngine->runFunction(FooF, noargs);

	  // Import result of execution:
	  outs() << "________________RESULT_______________________________: " << gv.IntVal << "\n";
	  executionEngine->freeMachineCodeForFunction(FooF);
	 // delete EE;
	  //llvm_shutdown();
	 // return 0;


}

JavaJITCompiler::JavaJITCompiler(const std::string &ModuleID,
		bool compiling_garbage_collector) :
		JavaLLVMCompiler(ModuleID, compiling_garbage_collector), listener(this) {

	EmitFunctionName = false;
	GCInfo = NULL;

	EngineBuilder engine(TheModule);
	TargetOptions options;
	options.NoFramePointerElim = true;
	//options.PrintMachineCode = true;
	//options.JITEmitDebugInfo = true;
	engine.setTargetOptions(options);
	engine.setEngineKind(EngineKind::JIT);
	executionEngine = engine.create();

	executionEngine->RegisterJITEventListener(&listener);
	TheDataLayout = executionEngine->getDataLayout();
	TheModule->setDataLayout(TheDataLayout->getStringRepresentation());
	TheModule->setTargetTriple(vmkit::VmkitModule::getHostTriple());
	JavaIntrinsics.init(TheModule);
	initialiseAssessorInfo();

	addJavaPasses();

	// Set the pointer to methods that will be inlined, so that these methods
	// do not get compiled by the JIT.
	executionEngine->updateGlobalMapping(JavaIntrinsics.AllocateFunction,
			(void*) (word_t) vmkitgcmalloc);
	executionEngine->updateGlobalMapping(JavaIntrinsics.VTAllocateFunction,
			(void*) (word_t) VTgcmalloc);
	executionEngine->updateGlobalMapping(
			JavaIntrinsics.ArrayWriteBarrierFunction,
			(void*) (word_t) arrayWriteBarrier);
	executionEngine->updateGlobalMapping(
			JavaIntrinsics.FieldWriteBarrierFunction,
			(void*) (word_t) fieldWriteBarrier);
	executionEngine->updateGlobalMapping(
			JavaIntrinsics.NonHeapWriteBarrierFunction,
			(void*) (word_t) nonHeapWriteBarrier);

	executionEngine->updateGlobalMapping(
			JavaIntrinsics.IsSecondaryClassFunctionInner,
			(void*) (word_t) IsSubtypeIntrinsic);
	executionEngine->updateGlobalMapping(
			JavaIntrinsics.IsSubclassOfFunctionInner,
			(void*) (word_t) IsSubtypeIntrinsic);
	executionEngine->updateGlobalMapping(JavaIntrinsics.CheckIfAssignable,
			(void*) (word_t) CheckIfObjectIsAssignableToArrayPosition);
}

JavaJITCompiler::~JavaJITCompiler() {
	executionEngine->removeModule(TheModule);
	delete executionEngine;
	// ~JavaLLVMCompiler will delete the module.
}

void JavaJITCompiler::StartProfilerThread()
{
	m_ProfilerThread = new ProfilerThread(patchMap);
	m_ProfilerThread->setJavaJITCompilerPtr(this);
	m_ProfilerThread->start();
}
void JavaJITCompiler::makeVT(Class* cl) {
	JavaVirtualTable* VT = cl->virtualVT;
	assert(VT && "No VT was allocated!");

	if (VT->init) {
		// The VT has already been filled by the AOT compiler so there
		// is nothing left to do!
		return;
	}

	Class* current = cl;
	word_t* functions = VT->getFunctions();
	while (current != NULL) {
		// Fill the virtual table with function pointers.
		for (uint32 i = 0; i < current->nbVirtualMethods; ++i) {
			JavaMethod& meth = current->virtualMethods[i];
			if (meth.offset != 0 || current->super != NULL) {
				functions[meth.offset] = getPointerOrStub(meth,
						JavaMethod::Virtual);
			}
		}
		current = current->super;
	}
}

extern "C" void ThrowUnfoundInterface() {
	JavaThread *th = JavaThread::get();

	BEGIN_NATIVE_EXCEPTION(1);

	// Lookup the caller of this class.
	vmkit::StackWalker Walker(th);
	vmkit::FrameInfo* FI = Walker.get();
	assert(FI->Metadata != NULL && "Wrong stack trace");
	JavaMethod* meth = (JavaMethod*) FI->Metadata;

	// Lookup the method info in the constant pool of the caller.
	uint16 ctpIndex = meth->lookupCtpIndex(FI);
	assert(ctpIndex && "No constant pool index");
	JavaConstantPool* ctpInfo = meth->classDef->getConstantPool();
	CommonClass* ctpCl = 0;
	const UTF8* utf8 = 0;
	Signdef* sign = 0;
	ctpInfo->resolveMethod(ctpIndex, ctpCl, utf8, sign);

	JavaThread::get()->getJVM()->abstractMethodError(ctpCl, utf8);

END_NATIVE_EXCEPTION
}

void JavaJITCompiler::makeIMT(Class* cl) {
InterfaceMethodTable* IMT = cl->virtualVT->IMT;
if (!IMT)
	return;

std::set<JavaMethod*> contents[InterfaceMethodTable::NumIndexes];
cl->fillIMT(contents);

for (uint32_t i = 0; i < InterfaceMethodTable::NumIndexes; ++i) {
	std::set<JavaMethod*>& atIndex = contents[i];
	uint32_t size = atIndex.size();
	if (size == 1) {
		JavaMethod* Imeth = *(atIndex.begin());
		JavaMethod* meth = cl->lookupMethodDontThrow(Imeth->name, Imeth->type,
				false, true, 0);
		if (meth) {
			IMT->contents[i] = getPointerOrStub(*meth, JavaMethod::Interface);
		} else {
			IMT->contents[i] = (word_t) ThrowUnfoundInterface;
		}
	} else if (size > 1) {
		std::vector<JavaMethod*> methods;
		bool SameMethod = true;
		JavaMethod* OldMethod = 0;

		for (std::set<JavaMethod*>::iterator it = atIndex.begin(), et =
				atIndex.end(); it != et; ++it) {
			JavaMethod* Imeth = *it;
			JavaMethod* Cmeth = cl->lookupMethodDontThrow(Imeth->name,
					Imeth->type, false, true, 0);

			if (OldMethod && OldMethod != Cmeth)
				SameMethod = false;
			else
				OldMethod = Cmeth;

			methods.push_back(Cmeth);
		}

		if (SameMethod) {
			if (methods[0]) {
				IMT->contents[i] = getPointerOrStub(*(methods[0]),
						JavaMethod::Interface);
			} else {
				IMT->contents[i] = (word_t) ThrowUnfoundInterface;
			}
		} else {

			// Add one to have a NULL-terminated table.
			uint32_t length = (2 * size + 1) * sizeof(word_t);

			word_t* table = (word_t*) cl->classLoader->allocator.Allocate(
					length, "IMT");

			IMT->contents[i] = (word_t) table | 1;

			uint32_t j = 0;
			std::set<JavaMethod*>::iterator Interf = atIndex.begin();
			for (std::vector<JavaMethod*>::iterator it = methods.begin(), et =
					methods.end(); it != et; ++it, j += 2, ++Interf) {
				JavaMethod* Imeth = *Interf;
				JavaMethod* Cmeth = *it;
				assert(Imeth != NULL);
				assert(j < 2 * size - 1);
				table[j] = (word_t) Imeth;
				if (Cmeth) {
					table[j + 1] = getPointerOrStub(*Cmeth,
							JavaMethod::Interface);
				} else {
					table[j + 1] = (word_t) ThrowUnfoundInterface;
				}
			}
			assert(Interf == atIndex.end());
		}
	}
}
}

void JavaJITCompiler::setMethod(Function* func, void* ptr, const char* name) {
func->setLinkage(GlobalValue::ExternalLinkage);
func->setName(name);
assert(ptr && "No value given");
executionEngine->updateGlobalMapping(func, ptr);
}

void * JavaJITCompiler::patchmaterializeFunction(void *codePointer,JavaMethod* meth,
		Class* customizeFor)
{


	//vmkit::VmkitModule::protectIR();


	Function* func = parseModifiedFunction(meth, customizeFor,2);


	const  vmkit::UTF8 * methodname = meth->getMehtodName();
	void* res = executionEngine->getPointerToGlobal(func);
	void *resFunc = executionEngine->getPointerToFunction(func);
	std::cout << "__________ optimization ____ new size - "<< globalcodesize << "|new place -"
								<< globalEmittedFunctionPointer << std::endl;

	//func->dump();

	std::string methodnameinstring;
	methodname->toString(methodnameinstring);
	std::string classname;
	meth->classDef->name->toString(classname);
	std::string key = classname + methodnameinstring;
	patchMap[key.c_str()]->llvmOptimzedFunctionPointer =globalEmittedFunctionPointer;
	patchMap[key.c_str()]->Size = globalcodesize;
	patchMap[key.c_str()]->isOptimizedDone= true;

	return NULL ;
	//memcpy(codePointer, globalEmittedFunctionPointer,globalcodesize);

	///vmkit::VmkitModule::unprotectIR();
}
void* JavaJITCompiler::materializeFunction(JavaMethod* meth,
	Class* customizeFor) {
vmkit::VmkitModule::protectIR();


Function* func = parseFunction(meth, customizeFor);
const  vmkit::UTF8 * methodname = meth->getMehtodName();
void* res = executionEngine->getPointerToGlobal(func);




if(isThisJ3Compilation)
{

	std::string methodnameinstring;
	methodname->toString(methodnameinstring);

	char buffer[30];
	std::size_t foundglobalVariable = methodnameinstring.find("group2S");
    std::size_t tailglobalVariable = methodnameinstring.find("group2E");
	if (tailglobalVariable != std::string::npos) {

		//we need to add with start pointer;
		char tempbuffer[30];
		methodnameinstring.copy(tempbuffer, methodnameinstring.size(), 0);
		//extract the class name and method .
		int size = methodnameinstring.size() - 6;
		memmove(tempbuffer, tempbuffer + 8, size);
		tempbuffer[size - 2] = '\0';
		void * pTailPointer = executionEngine->getPointerToFunction(func);

		std::cout<<"[VMKIT] Insert tail global counter function  -  "<<tempbuffer <<"|pointer - "<<pTailPointer<<std::endl;
		patchMap[tempbuffer]->llvmGlobalVariableTailPointer = pTailPointer;

	}else if (foundglobalVariable != std::string::npos) {
		// don't ruin the original string,  just copy

		methodnameinstring.copy(buffer, methodnameinstring.size(), 0);
		int size = methodnameinstring.size() - 6;
		memmove(buffer, buffer + 8, size);
		buffer[size - 2] = '\0';

		// we couldn't able to found any dummy functions.


			Patch *g_patch = new Patch();
			void *pStartPointer  =executionEngine->getPointerToFunction(func);
			g_patch->llvmGlobalVariableMachinePointer = pStartPointer;
			patchMap.insert(
					std::pair<std::string, Patch*>(buffer, g_patch));

			std::cout << "[VMKIT] The start global function : " << buffer <<"| Pointer - "<<pStartPointer<< std::endl;

	} else {

		std::string classname;
		meth->classDef->name->toString(classname);
		std::string key = classname + methodnameinstring;
		std::map<std::string, Patch*>::iterator itFind = patchMap.find(key);
		if (itFind != patchMap.end()) //we found the key
				{

			itFind->second->llvmFunction = func;
			itFind->second->Size = globalcodesize;
			itFind->second->codePointer = globalEmittedFunctionPointer;
			itFind->second->meth = meth;
			itFind->second->cusotmizedFor = customizeFor;
			itFind->second->previousexetime=0.0;
			itFind->second->numberofTime=0;
			itFind->second->avgexetime =0.0;
			itFind->second->samplingcount=0;
			itFind->second->previousglobalvalue=0;
			itFind->second->isOptimizedDone=false;
			itFind->second->isFunctionReplaced=false;
			std::cout << "__Stroing the function - Name -" << itFind->first
					<< "|Size - " << itFind->second->Size << "|location - "
					<< itFind->second->codePointer <<"meth - "<<meth << std::endl;

		}
	}

}

if (!func->isDeclaration()) {
	/*std::cout << "adding this function in to cache - " << *meth->getMehtodName()
			<< std::endl;*/
	llvm::GCFunctionInfo& GFI = GCInfo->getFunctionInfo(*func);

	Jnjvm* vm = JavaThread::get()->getJVM();
	vmkit::VmkitModule::addToVM(vm, &GFI, (JIT*) executionEngine, allocator,
			meth);

//	if(diff!=0)
  	////func->deleteBody();
	// Now that it's compiled, we don't need the IR anymore

	/*if(diff !=0)
	 func->deleteBody();*/

}

vmkit::VmkitModule::unprotectIR();
if (customizeFor == NULL || !getMethodInfo(meth)->isCustomizable) {
	/*std::cout << "compiled code is seting here , which method  	 "
			<< *meth->getMehtodName() << std::endl;*/

		meth->code = res;
}
return res;
}

void* JavaJITCompiler::GenerateStub(llvm::Function* F) {
vmkit::VmkitModule::protectIR();
void* res = executionEngine->getPointerToGlobal(F);

// If the stub was already generated through an equivalent signature,
// The body has been deleted, so the function just becomes a declaration.
if (!F->isDeclaration()) {
	llvm::GCFunctionInfo& GFI = GCInfo->getFunctionInfo(*F);

	Jnjvm* vm = JavaThread::get()->getJVM();
	vmkit::VmkitModule::addToVM(vm, &GFI, (JIT*) executionEngine, allocator,
	NULL);

// Now that it's compiled, we don't need the IR anymore
	F->deleteBody();
}
vmkit::VmkitModule::unprotectIR();
return res;
}

// Helper function to run an executable with a JIT
extern "C" int StartJnjvmWithJIT(int argc, char** argv, char* mainClass) {
llvm::llvm_shutdown_obj X;

vmkit::VmkitModule::initialise(argc, argv);
vmkit::Collector::initialise(argc, argv);

vmkit::ThreadAllocator allocator;
char** newArgv = (char**) allocator.Allocate((argc + 1) * sizeof(char*));
memcpy(newArgv + 1, argv, argc * sizeof(void*));
newArgv[0] = newArgv[1];
newArgv[1] = mainClass;

vmkit::BumpPtrAllocator Allocator;
JavaJITCompiler* Comp = JavaJITCompiler::CreateCompiler("JITModule");
JnjvmBootstrapLoader* loader =
		new (Allocator, "Bootstrap loader") JnjvmBootstrapLoader(Allocator,
				Comp, true);
Jnjvm* vm = new (Allocator, "VM") Jnjvm(Allocator, NULL, loader);
vm->runApplication(argc + 1, newArgv);
vm->waitForExit();

return 0;
}

word_t JavaJ3LazyJITCompiler::getPointerOrStub(JavaMethod& meth, int side) {
return meth.getSignature()->getVirtualCallStub();
}

Value* JavaJ3LazyJITCompiler::addCallback(Class* cl, uint16 index,
	Signdef* sign, bool stat, llvm::BasicBlock* insert) {
LLVMSignatureInfo* LSI = getSignatureInfo(sign);
// Set the stub in the constant pool.
JavaConstantPool* ctpInfo = cl->ctpInfo;
word_t stub = stat ? sign->getStaticCallStub() : sign->getSpecialCallStub();
if (!ctpInfo->ctpRes[index]) {
// Do a compare and swap, so that we do not overwrite what a stub might
// have just updated.
	word_t val = (word_t) __sync_val_compare_and_swap(&(ctpInfo->ctpRes[index]),
	NULL, (void*) stub);
// If there is something in the the constant pool that is not NULL nor
// the stub, then it's the method.
	if (val != 0 && val != stub) {
		return ConstantExpr::getIntToPtr(
				ConstantInt::get(Type::getInt64Ty(insert->getContext()), val),
				stat ? LSI->getStaticPtrType() : LSI->getVirtualPtrType());
	}
}
// Load the constant pool.
Value* CTP = getResolvedConstantPool(ctpInfo);
Value* Index = ConstantInt::get(Type::getInt32Ty(insert->getContext()), index);
Value* func = GetElementPtrInst::Create(CTP, Index, "", insert);
func = new LoadInst(func, "", false, insert);
// Bitcast it to the LLVM function.
func = new BitCastInst(func,
		stat ? LSI->getStaticPtrType() : LSI->getVirtualPtrType(), "", insert);
return func;
}

bool JavaJ3LazyJITCompiler::needsCallback(JavaMethod* meth, Class* customizeFor,
	bool* needsInit) {
*needsInit = true;
return (meth == NULL || getMethod(meth, customizeFor)->hasExternalWeakLinkage());
}

JavaJ3LazyJITCompiler::JavaJ3LazyJITCompiler(const std::string& ModuleID,
	bool compiling_garbage_collector) :
	JavaJITCompiler(ModuleID, compiling_garbage_collector) {
}

JavaJITCompiler* JavaJITCompiler::CreateCompiler(const std::string& ModuleID,
	bool compiling_garbage_collector) {
return new JavaJ3LazyJITCompiler(ModuleID, compiling_garbage_collector);
}
JavaJITCompiler* JavaJITCompiler::CreateCompiler(const std::string& ModuleID,
	int isThisJ3, bool compiling_garbage_collector) {
	if(isThisJ3==1)
	isThisJ3Compilation = isThisJ3;
return new JavaJ3LazyJITCompiler(ModuleID, compiling_garbage_collector);
}
