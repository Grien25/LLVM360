# LLVM360

## Info

This is just a little personal experiment i'm doing with llvm, the code is not good :} i took some stuff from the old redex's recompiler project like the XexLoader and some of the instructions decoded.

--------------- 
Join the project [Discord][dis] server! 

### Name
"LLVM360" LLVM + xbox360, but it's just a temporany, i don't really like it so will probably change

## Building
- Install LLVM Libs (I used [these][win-llvm])
- Move the files in something like C:/LLVM19 or wherever you prefer
- Git Clone this repository
- And Run `cmake -B out -DLLVM_USE_CRT_RELEASE=MT -DCMAKE_PREFIX_PATH=<LLVM_BINS_>` (LLVM_BINS is the root folder where you stored your LLVM libs mine is "C:/LLVM19" for example)
- In "out/" folder open the LLVM360.sln 
- If everything is good and i or you didn't messed up something, it should compile fine

## Contributing
if want to contribute, make a fork and a PR :} also join the discord server! 



[win-llvm]: https://github.com/c3lang/win-llvm
[dis]: https://discord.gg/JufwFS9mmf
