# AGKTier2onAppleSilicon

Port of AGKTier2 on Apple Silicon

The file libAGKMac.a was compiled and tested on Apple M1, the only modifications made were to the dependencies, glfw was compiled as universal binary, DDHidLib as arm64 only. After switching dependencies, the program compiles properly. 

A bit more work is required to compile DDHidLib as a universal binary, then you should be able to compile libAGKMac to universal binary by just adding back x86_64 in the list of archs to compile to.
