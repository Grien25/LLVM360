Based on the Discord chats and the project files, here's what AleLlvm360 is all about:
🎮 Project Overview
AleLlvm360 is an Xbox 360 emulator/recompiler that uses LLVM (the compiler infrastructure) to translate Xbox 360 PowerPC (PPC) code to x86-64 code that can run on modern PCs.
🔧 Key Technical Details
Static Recompilation: The project takes Xbox 360 executables (.xex files) and statically recompiles the PowerPC assembly code into x86-64 code using LLVM's infrastructure.
Main Components:
Naive+: The recompiler component that decodes PowerPC instructions and converts them to LLVM IR (Intermediate Representation)
Emulator: A runtime environment that provides Xbox 360 system functions and kernel calls
XEX Loader: Handles loading and parsing Xbox 360 executable files
Recent Progress (from Discord):
External function call support was recently added
Improved function boundary analyzer for better code detection
Working on embedding .data and .rdata sections in the generated executables
Implementing control flow analysis passes
🛠️ How It Works
The project follows this workflow:
Load Xbox 360 .xex files (the executable format for Xbox 360)
Decode PowerPC instructions from the game
Analyze control flow and function boundaries
Convert PPC instructions to LLVM IR
Compile LLVM IR to native x86-64 code
Runtime provides Xbox kernel functions (XKernel, XAM, etc.)
🎯 Current Status
From the Discord messages, it appears:
The project is actively being developed by AleBlbl
They're working on a "Naive+" rewrite with better control flow detection
External function calls now work, which is crucial for running real games
The developer is improving the instruction analyzer and runtime
💡 The Name
"LLVM360" = LLVM + Xbox 360, though the developer mentioned they might change it as they're not entirely happy with the name.
This is essentially a personal experimental project to create an Xbox 360 emulator using static recompilation techniques with LLVM, which is quite an ambitious and technically impressive undertaking! The approach of using LLVM for static recompilation is interesting as it could potentially produce very efficient native code compared to traditional dynamic recompilation approaches