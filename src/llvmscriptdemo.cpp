/*
 * llvmscriptdemp.cpp - Example LLVM 'script'. Integrates LLVM bitcode with
 * native C++.
 *
 * Written in 2011 by David C. Bishop <david@davidbishop.org>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

#include <string>
#include <memory>
#include <iostream>
#include <limits.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Support/MemoryBuffer.h>
#include "llvm/ExecutionEngine/Interpreter.h"
//#include <llvm/ExecutionEngine/MCJIT.h>
#include "llvm/ExecutionEngine/GenericValue.h"
#include <llvm/Support/DynamicLibrary.h>

#include "TestClass.hpp"

using namespace std;
using namespace llvm;

void print (const char *message)
{
	cout << message << endl;
}

GenericValue print_W (FunctionType *FT, ArrayRef<GenericValue> Args)
{
	print((const char *)GVTOP(Args[0]));
	return GenericValue();
}

void printhi()
{
	print("hi");
}

GenericValue printhi_W (FunctionType *FT, ArrayRef<GenericValue> Args)
{
	printhi();
	return GenericValue();
}

int main()
{
   InitializeNativeTarget();
   LLVMContext context;
   string error;
	
   cout << "starting" << endl;

   auto mb = MemoryBuffer::getFile("bitcode/damage.bc");
   if (!mb) {
      cout << "ERROR: Failed to getFile" << endl;
      return 0;
   }
	
   cout << "got file" << endl;
	
   auto mbr = (*mb)->getMemBufferRef();

   cout << "got mem buffer ref" << endl;

   auto m = parseBitcodeFile(mbr, context);
   if(!m) {
      cout << "ERROR: Failed to load script file." << endl;
      return 0;
   }
   auto mo = m->release();
	
   cout << "got bit code " << mo << endl;
	
   auto ee = EngineBuilder(std::unique_ptr<Module>(mo))
		.setEngineKind(EngineKind::Interpreter)
		.create();

   sys::DynamicLibrary::AddSymbol("lle_X__Z5printPKc", (void*)&print_W);
   sys::DynamicLibrary::AddSymbol("lle_X__Z7printhiv", (void*)&printhi_W);

   cout << "built engine " << ee << endl;

   // NOTE: Function names are mangled by the compiler.
   Function* init_func = ee->FindFunctionNamed("_Z9initilizev");
   if(!init_func) {
      cout << "ERROR: Failed to find 'initialize' function." << endl;
      return 0;
   }
	
   cout << "got function" << init_func << endl;

   ee->runFunction(init_func, ArrayRef<GenericValue>());

   Function* attack_func = ee->FindFunctionNamed("_Z6attackP9TestClass");
   if(!attack_func) {
      cout << "ERROR: Failed to find 'attack' function." << endl;
      return 0;
   }

   cout << "got class" << attack_func << endl;
   TestClass c;

   ee->runFunction(attack_func, ArrayRef<GenericValue>(GenericValue(&c)));


   delete ee;
}
