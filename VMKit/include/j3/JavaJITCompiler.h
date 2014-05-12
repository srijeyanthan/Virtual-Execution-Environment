//===------- JavaJITCompiler.h - The J3 just in time compiler -------------===//
//
//                            The VMKit project
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef J3_JIT_COMPILER_H
#define J3_JIT_COMPILER_H

#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITEventListener.h"
#include "j3/JavaLLVMCompiler.h"
#include "llvm/IR/Function.h"
#include <queue>
using namespace llvm;
//#include "j3/ProfilerThread.h"
namespace j3 {

class JavaJITCompiler;
typedef struct Patch {
	void *llvmGlobalVariableMachinePointer;
	void *llvmOptimzedFunctionPointer;
	void *llvmGlobalVariableTailPointer;
	Function *llvmFunction;
	JavaMethod *meth;
	void *codePointer;
	size_t Size;
	Class *cusotmizedFor;
	unsigned long long previousexetime;
	int numberofTime;
	double avgexetime;
	int samplingcount;
	int previousglobalvalue;
	bool isOptimizedDone;
	bool isFunctionReplaced;
	int  functionindex;
	Patch()
	{
		    llvmGlobalVariableMachinePointer=NULL;
			llvmOptimzedFunctionPointer=NULL;
			llvmGlobalVariableTailPointer=NULL;
			llvmFunction=NULL;
			codePointer=NULL;
			meth=NULL;
			Size=0;
			cusotmizedFor=NULL;
			previousexetime=0.0;
			numberofTime=0;
			avgexetime=0.0;
			samplingcount=0;
			previousglobalvalue=0;
			isOptimizedDone=false;
			isFunctionReplaced=false;
			functionindex =0;
	}

}Patch;

typedef struct Gnuplotmsg
{
    int functionindex;
    int msgtype;
    int value;
    Gnuplotmsg()
    {
    	functionindex =0;
    	msgtype=0;
    	value=0;
    }

}Gnuplotmsg;
class JavaJITListener : public llvm::JITEventListener {
  JavaJITCompiler* TheCompiler;
public:
  JavaJITListener(JavaJITCompiler* Compiler) {
    TheCompiler = Compiler;
  }
  virtual void NotifyFunctionEmitted(
      const llvm::Function &F,
      void *Code,
      size_t Size,
      const llvm::JITEventListener::EmittedFunctionDetails &Details);
};

class ProfilerThread {
public:
	ProfilerThread(std::map<std::string, Patch *> &patchMap);

	~ProfilerThread() ;
	void start();
	void join();
    void setJavaJITCompilerPtr(void *ptr);
    void run();
    static void* helper(void* arg);

private:

    unsigned long long rdtsc();

	std::map<std::string, Patch *> &m_patchMap;
	pthread_t tid;
	pthread_mutexattr_t mta;
	pthread_mutex_t m_mutex;
	unsigned long long m_profilerstarttime;
	void *m_JavaJITCompilerPtr;
};
class GnuSocketThread {
public:
	GnuSocketThread(/*std::queue<Gnuplotmsg *> &plotq*/);
	~GnuSocketThread();
	void start();
	void join();
	static void* helper(void* arg);
	void startSocket();

private:
	int connFd;
	pthread_t tid;
	pthread_mutexattr_t mta;
	pthread_mutex_t m_socketmutex;

};
class JavaJITCompiler : public JavaLLVMCompiler {
public:

  bool EmitFunctionName;
  JavaJITListener listener;
  llvm::ExecutionEngine* executionEngine;
  llvm::GCModuleInfo* GCInfo;
  ProfilerThread *m_ProfilerThread;
  GnuSocketThread *m_GnuSocketThread;

  virtual void StartProfilerAndGnuPlotThread();
  JavaJITCompiler(
	const std::string &ModuleID, bool compiling_garbage_collector = false);
  ~JavaJITCompiler();
  
  virtual bool isStaticCompiling() {
    return false;
  }

  void Test();
  virtual void* GenerateStub(llvm::Function* F);  
 
  virtual bool emitFunctionName() {
    return EmitFunctionName;
  }


  virtual void makeVT(Class* cl);
  virtual void makeIMT(Class* cl);
  virtual void *patchmaterializeFunction(void *codePointer,JavaMethod* meth,
			Class* customizeFor);
  virtual void* materializeFunction(JavaMethod* meth, Class* customizeFor);
  virtual llvm::Constant* getFinalObject(JavaObject* obj, CommonClass* cl);
  virtual JavaObject* getFinalObject(llvm::Value* C);
  virtual llvm::Constant* getNativeClass(CommonClass* cl);
  virtual llvm::Constant* getJavaClass(CommonClass* cl);
  virtual llvm::Constant* getJavaClassPtr(CommonClass* cl);
  virtual llvm::Constant* getStaticInstance(Class* cl);
  virtual llvm::Constant* getVirtualTable(JavaVirtualTable*);
  virtual llvm::Constant* getMethodInClass(JavaMethod* meth);
  
  virtual llvm::Constant* getString(JavaString* str);
  virtual llvm::Constant* getStringPtr(JavaString** str);
  virtual llvm::Constant* getResolvedConstantPool(JavaConstantPool* ctp);
  virtual llvm::Constant* getNativeFunction(JavaMethod* meth, void* natPtr);
  
  virtual void setMethod(llvm::Function* func, void* ptr, const char* name);


  virtual llvm::Value* addCallback(Class* cl, uint16 index, Signdef* sign,
                                   bool stat, llvm::BasicBlock* insert) = 0;
  virtual word_t getPointerOrStub(JavaMethod& meth, int type) = 0;

  static JavaJITCompiler* CreateCompiler(
	const std::string& ModuleID, bool compiling_garbage_collector = false);
  static JavaJITCompiler* CreateCompiler(
 	const std::string& ModuleID, int isThisVMkit ,bool compiling_garbage_collector = false);
};

class JavaJ3LazyJITCompiler : public JavaJITCompiler {
public:
  virtual bool needsCallback(JavaMethod* meth, Class* customizeFor, bool* needsInit);
  virtual llvm::Value* addCallback(Class* cl, uint16 index, Signdef* sign,
                                   bool stat, llvm::BasicBlock* insert);
  virtual word_t getPointerOrStub(JavaMethod& meth, int side);
  
  virtual JavaCompiler* Create(
	const std::string& ModuleID, bool compiling_garbage_collector = false)
  {
    return new JavaJ3LazyJITCompiler(ModuleID, compiling_garbage_collector);
  }

  JavaJ3LazyJITCompiler(
	const std::string& ModuleID, bool compiling_garbage_collector = false);
};

} // end namespace j3

#endif
