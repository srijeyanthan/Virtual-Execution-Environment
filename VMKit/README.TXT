=========================================== 
Group 2 instructions  
===========================================
Major code changes have done on following files 
 - JavaJITCompiler.cpp
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


 - JavaLLVMCompiler.cpp
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


===============
Demo java files
===============
All the jar file and java files are inside demojava folder 
before run the the program ,crete the classpath directory and put 
your class file which we have to execute 
=============
Requirements:
=============

* Install required packages (Debian):
apt-get install ant libgconf2-dev libgtk2.0-dev sharutils

* Install required packages (SUSE):
zypper install ant gconf2-devel gtk2-devel

* If you want to use OpenJDK:
apt-get install default-jre default-jdk

==============
GNU Classpath:
==============

* For Macos users
sudo port install antlr
wget http://ftp.gnu.org/gnu/classpath/classpath-0.99.tar.gz
tar zxf classpath-0.99.tar.gz
patch -p1 < patches/classpath-0.99.macos.patch
cd classpath-0.99
./configure --disable-plugin --disable-examples --disable-Werror --with-antlr-jar=/opt//local/share/java/antlr.jar
make -j12
cd lib
ln -s ../native/jni/gtk-peer/.libs/libgtkpeer.dylib;
ln -s ../native/jni/gconf-peer/.libs/libgconfpeer.dylib;
ln -s ../native/jni/java-io/.libs/libjavaio.dylib;
ln -s ../native/jni/java-lang/.libs/libjavalangreflect.dylib;
ln -s ../native/jni/java-lang/.libs/libjavalang.dylib;
ln -s ../native/jni/java-net/.libs/libjavanet.dylib;
ln -s ../native/jni/java-nio/.libs/libjavanio.dylib;
ln -s ../native/jni/java-util/.libs/libjavautil.dylib;

* For Linux users:
wget http://ftp.gnu.org/gnu/classpath/classpath-0.99.tar.gz
tar zxf classpath-0.99.tar.gz
cd classpath-0.99
./configure --disable-plugin --disable-examples --disable-Werror
make -j12
cd lib
ln -sf ../native/jni/gtk-peer/.libs/libgtkpeer.so
ln -sf ../native/jni/gconf-peer/.libs/libgconfpeer.so
ln -sf ../native/jni/java-io/.libs/libjavaio.so
ln -sf ../native/jni/java-lang/.libs/libjavalangreflect.so
ln -sf ../native/jni/java-lang/.libs/libjavalang.so
ln -sf ../native/jni/java-net/.libs/libjavanet.so
ln -sf ../native/jni/java-nio/.libs/libjavanio.so
ln -sf ../native/jni/java-util/.libs/libjavautil.so

=====
LLVM:
=====

* Get the compatible release LLVM 3.3:

wget http://llvm.org/releases/3.3/llvm-3.3.src.tar.gz
tar zxf llvm-3.3.src.tar.gz
cd llvm-3.3.src/tools
wget http://llvm.org/releases/3.3/cfe-3.3.src.tar.gz
tar zxf cfe-3.3.src.tar.gz
mv cfe-3.3.src clang
cd ../projects
wget http://llvm.org/releases/3.3/compiler-rt-3.3.src.tar.gz
tar zxf compiler-rt-3.3.src.tar.gz
mv compiler-rt-3.3.src compiler-rt
cd ../..

* Patch LLVM if you want to have a better debug support for VMKit:
  You will have to get VMKit (see the next section) to find the patch.

cd llvm-3.3.src
patch -p1 < ../vmkit2/patches/llvm.patch

NOTICE:
* VMKit does not support building LLVM out of source because of a bug
  in "llvm-config", so please build in the source, and keep different source
  trees for different configurations/architectures of builds.

* Make it (release mode):

./configure --enable-optimized --enable-assertions --enable-jit --enable-cxx11 --enable-threads --enable-pthreads --enable-pic --enable-targets=host,cpp --enable-zlib --disable-docs --disable-doxygen
make -j12 BUILD_EXAMPLES=0 ENABLE_DOCS=0

* You can also make it in debug mode, but this is is not necessary to debug compiled programs:

./configure --disable-optimized --enable-debug-runtime --enable-debug-symbols --enable-keep-symbols --enable-assertions --enable-jit --enable-cxx11 --enable-threads --enable-pthreads --enable-pic --enable-targets=host,cpp --enable-zlib --disable-docs --disable-doxygen

* Build it:

make -j12 BUILD_EXAMPLES=0 ENABLE_DOCS=0

======
VMKit:
======

* Get it anonymously:

svn co http://llvm.org/svn/llvm-project/vmkit/trunk

* or get as an official developer:

svn co https://YOUR_USER_NAME@llvm.org/svn/llvm-project/vmkit/trunk

* Make it (release mode):

./configure --with-llvm-config-path=@PATH_TO_LLVM@/Release+Asserts/bin/llvm-config --with-gnu-classpath-libs=@PATH_TO_GNUCLASSPATH@/lib --with-gnu-classpath-glibj=@PATH_TO_GNUCLASSPATH@/lib/glibj.zip

* Or you can make it in debug mode:

./configure --with-llvm-config-path=@PATH_TO_LLVM@/Release+Asserts/bin/llvm-config --with-gnu-classpath-libs=@PATH_TO_GNUCLASSPATH@/lib --with-gnu-classpath-glibj=@PATH_TO_GNUCLASSPATH@/lib/glibj.zip --enable-assert=yes --disable-optimized --enable-debug=yes

* Or you can enable OSGi Incinerator build:

./configure --with-llvm-config-path=@PATH_TO_LLVM@/Release+Asserts/bin/llvm-config --with-gnu-classpath-libs=@PATH_TO_GNUCLASSPATH@/lib --with-gnu-classpath-glibj=@PATH_TO_GNUCLASSPATH@/lib/glibj.zip --enable-incinerator=yes

* Build it:

make -j12
