﻿# NifSkope 2.0.dev9

NifSkope is a tool for opening and editing the NetImmerse file format (NIF). NIF is used by video games such as Morrowind, Oblivion, Skyrim, Fallout 3/NV/4/76, Starfield, Civilization IV, and more.

This is an experimental fork of 2.0.dev9 with partial support for Starfield materials, and improvements and fixes to Fallout 76 rendering.

### Download

Binary packages for Windows and Linux can be downloaded from [Releases](https://github.com/fo76utils/nifskope/releases). The most up to date builds are automatically generated on any change to the source code, and are available as artifacts from GitHub workflows under [Actions](https://github.com/fo76utils/nifskope/actions). Note that downloading artifacts requires signing in to GitHub. Binaries have been built for CPUs with support for AVX2, or AVX and F16C (NifSkope\_noavx2.exe).

You can also download the latest official release from [niftools/nifskope](https://github.com/niftools/nifskope/releases), or development builds from [hexabits/nifskope](https://github.com/hexabits/nifskope/releases).

**Note:** running NifSkope under Wayland on Linux may require setting the QT\_QPA\_PLATFORM environment variable to "xcb":

    QT_QPA_PLATFORM=xcb ./NifSkope

#### Building from source code

Compiling NifSkope requires Qt 5.15. On Windows, [MSYS2](https://www.msys2.org/) can be used for building. After running the MSYS2 installer, use the following commands in the MSYS2-UCRT64 shell to install required packages:

    pacman -S base-devel mingw-w64-ucrt-x86_64-gcc
    pacman -S mingw-w64-ucrt-x86_64-qt5-base mingw-w64-ucrt-x86_64-qt5-3d mingw-w64-ucrt-x86_64-qt5-imageformats
    pacman -S git

All installed MSYS2 packages can be updated anytime later by running the command '**pacman -Syu**'. To download the complete NifSkope source code, use '**git clone**' as follows:

    git clone --recurse-submodules https://github.com/fo76utils/nifskope.git

Finally, run '**qmake**' and then '**make**' to build the source code (the -j 8 option sets the number of processes to run in parallel). The resulting binaries and required DLL files and resources are placed under '**release**'.

    qmake NifSkope.pro
    make -j 8

By default, code is generated for Intel Haswell or compatible CPUs, including the AMD Zen series or newer. Running qmake with the **noavx2=1** option reduces the requirement to Intel Ivy Bridge or AMD FX CPUs, to build for even older hardware, edit the compiler flags in NifSkope.pro.

### Issues

Anyone can report issues specific to this fork at [GitHub](https://github.com/fo76utils/nifskope/issues).


### Contribute

You can fork the latest source from [GitHub](https://github.com/niftools/nifskope). See [Fork A Repo](https://help.github.com/articles/fork-a-repo) on how to send your contributions upstream. To grab all submodules, make sure to use `--recursive` like so:

```
git clone --recursive git://github.com/<YOUR_USERNAME>/nifskope.git
```

For information about development:

- Refer to our [GitHub wiki](https://github.com/niftools/nifskope/wiki#wiki-development) for information on compilation.


### Miscellaneous

Refer to these other documents in your installation folder or at the links provided:


## [GLTF IMPORT/EXPORT](https://github.com/hexabits/nifskope/blob/develop/README_GLTF.md)

## [TROUBLESHOOTING](https://github.com/niftools/nifskope/blob/develop/TROUBLESHOOTING.md)

## [CHANGELOG](https://github.com/niftools/nifskope/blob/develop/CHANGELOG.md)

## [CONTRIBUTORS](https://github.com/niftools/nifskope/blob/develop/CONTRIBUTORS.md)

## [LICENSE](https://github.com/niftools/nifskope/blob/develop/LICENSE.md)

