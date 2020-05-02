## Compilation

Please note that this workflow was tested only under macOS 10.15. 
But it should possible to use it for other system as well.

This library is bundled as some Python code with a C extension. 
The C extension needs to link against V8 library and Python3 library.

We do not use standard setup.py toolchain for compilation of the C++ code. 
We use it only for bundling the final artifact.
The [main reason](https://github.com/area1/stpyv8/issues/9) for this is that we 
have to stay compatible with [V8 build tools](https://v8.dev/docs/build) and
that is why we adopted [their build system](https://gn.googlesource.com/gn/+/master/docs/reference.md).

Doing compilation outside of Python toolchain brings some challenges.
For example we have to determine paths to compatible Python C API include files and locations of Python libraries.
We have a suite of shell scripts which try to prepare correct environment for compilation.
But don't worry if anything goes wrong on your system, you should be always able to override 
our default settings via env variables. 

### Pesky Python

Unfortunately Python situation is a real mess.
Our library links again Python3.7. Some tools in the toolchain require Python2.x.
Your system might have several versions of Python installed already.
  
Our strategy is to create two clean Python virtual environments (in `.venv` and `.venv2`). 
One with latest Python3 and one with Python2. Our shell wrappers do some switching of Python environments.
We use Python3 by default but force usage of Python2 when needed.
Virualenv [has a lot of issues](https://datagrok.org/python/activate). 
One being that is it was not really designed for building Python C extensions.
Some aspects of it still leak down to system Python. So you are required to have Python3.7 as your system Python.
At least that is how it works on my machine (macOS 10.15).

### Ready?

Our general strategy is:

0. update submodules
1. install build dependencies
2. generate "gn" project files with selected config (release/debug)
3. enter depot shell with correct environment
4. use ninja to build the library => Python C extension
5. use setup.py to bundle the extension with rest of the py files => Python library
6. optionally install the library in Python3 venv

##### Make sure you have updated submodules

The repo has submodules under [/vendor](../vendor). Initially you should have cloned the repo with `--recursive`:
```bash
git clone --recursive https://github.com/darwin/naga.git
```

Or alternatively:
```bash
git clone https://github.com/darwin/naga.git
git submodule init
```

To update submodules:
```bash
git submodule update
```

##### Install dependencies

```bash
./scripts/prepare-deps.sh
./scripts/prepare-v8.sh
```

##### Generate build scripts

```bash
./scripts/gen-build.sh

# or for debug build run:
# ./scripts/gen-build.sh debug
```

```bash
> gn gen --verbose /Users/darwin/lab/naga/.work/gn_out/8.4.100/release --args=import("//args/release.gn") naga_disable_feature_cljs=false
Done. Made 149 targets from 84 files in 1874ms
to enter depot shell:
> ./scripts/enter-depot-shell.sh
in depot shell, you may use following commands:
> ninja -C /Users/darwin/lab/naga/.work/gn_out/8.4.100/release naga
```

##### Enter the build shell and build it

After entering the shell you should review the printed settings:

```
> ./scripts/enter-depot-shell.sh
NAGA_ACTIVE_LOG_LEVEL=
NAGA_BASH_COLORS=yes
NAGA_BUILDTOOLS_PATH=/Users/darwin/lab/naga/.work/v8/buildtools
NAGA_CLANG_FORMAT_PATH=/usr/local/bin/clang-format
NAGA_CLANG_TIDY_EXTRA_ARGS=
NAGA_CLANG_TIDY_PATH=clang-tidy
NAGA_CPYTHON_REPO_DIR=/Users/darwin/lab/naga/.work/cpython
NAGA_DEPOT_GIT_URL=https://chromium.googlesource.com/chromium/tools/depot_tools.git
NAGA_EXT_BUILD_BASE=/Users/darwin/lab/naga/.work/ext_build
NAGA_GIT_CACHE_PATH=/Users/darwin/lab/naga/.work/git_cache
NAGA_GN_EXTRA_ARGS=naga_disable_feature_cljs=false
NAGA_GN_GEN_EXTRA_ARGS=
NAGA_GN_OUT_DIR=/Users/darwin/lab/naga/.work/gn_out
NAGA_LDFLAGS=-L/usr/local/lib
NAGA_PYTHON_ABIFLAGS=dm
NAGA_PYTHON_CFLAGS=-I/usr/local/Cellar/python/3.7.7/Frameworks/Python.framework/Versions/3.7/include/python3.7dm -I/usr/local/Cellar/python/3.7.7/Frameworks/Python.framework/Versions/3.7/include/python3.7dm -Wno-unused-result -Wsign-compare -fno-common -dynamic -g -O0 -Wall
NAGA_PYTHON_INCLUDES=-I/usr/local/Cellar/python/3.7.7/Frameworks/Python.framework/Versions/3.7/include/python3.7dm -I/usr/local/Cellar/python/3.7.7/Frameworks/Python.framework/Versions/3.7/include/python3.7dm
NAGA_PYTHON_LDFLAGS=-L/usr/local/Cellar/python/3.7.7/Frameworks/Python.framework/Versions/3.7/lib/python3.7/config-3.7dm-darwin -lpython3.7dm -ldl -framework CoreFoundation
NAGA_PYTHON_LIBS=-lpython3.7dm -ldl -framework CoreFoundation
NAGA_V8_GIT_TAG=8.4.100
NAGA_V8_GIT_URL=https://chromium.googlesource.com/v8/v8.git
NAGA_WAIT_FOR_DEBUGGER=1
NAGA_WORK_DIR=/Users/darwin/lab/naga/.work
in '/Users/darwin/lab/naga/gn'
gn:
```

Compile generated build files from previous step: 
```bash
ninja -C /Users/darwin/lab/naga/.work/gn_out/8.4.100/release naga
```

When you are done, exit the sub-shell.
```bash
exit
```

##### Run setup.py

```bash
cd ext
./setup.sh build

# or for debug build run:
# ./setup.sh build --debug
```

##### Run install it into our venv

Install final package into our Python3 environment

```bash
cd ext
./setup.sh install --prefix ../.work/venv_python3
```

##### Run tests

```bash
./scripts/test.sh
```

# FAQ

### Where is compiled output?

> By default all generated/downloaded/cached files go to `.work` directory. You can change it via NAGA_WORK_DIR.
> Compilation output from `gn` is configured via `NAGA_GN_OUT_DIR` and defaults to `$NAGA_WORK_DIR/gn_out`. 

### Where should I put V8 repo?

> V8 repo directory location can be specified by `V8_HOME` env variable. By default it goes to `$NAGA_WORK_DIR/v8`.
> The build system expects it in `gn/v8`, so if don't specify explicit `V8_HOME` to point to `gn/v8`
> the build script will create a symlink from `gn/v8` to your `V8_HOME`. 
> Also note that the parent directory of `V8_HOME` is used for gclient config/state files. The directory should
> be under your control and no other gclient checkout should reside there.
 
### How do I change V8 revision?

> `export NAGA_V8_GIT_TAG=${revision}`
>
> Then do `./scripts/prepare-v8.sh` which will pull and checkout the specified revision.