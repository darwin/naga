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
git clone --recursive https://github.com/darwin/stpyv8.git
```

Or alternatively:
```bash
git clone https://github.com/darwin/stpyv8.git
git submodule init
```

To update submodules:
```bash
git submodule update
```

##### Install dependencies

```bash
./scripts/install-deps.sh
./scripts/checkout-v8.sh
```

##### Generate the build project

```bash
./scripts/gen-build.sh

# or for debug build run:
# ./scripts/gen-build.sh debug
```

##### Enter the build shell and build it

After entering the shell you should review the printed settings:

```
> ./scripts/enter_depot_shell.sh
STPYV8_BASH_COLORS=yes
STPYV8_BOOST_INCLUDES=-I/usr/local/include
STPYV8_BOOST_LDFLAGS=-L/usr/local/lib -lboost_system -lboost_python37
STPYV8_BUILDTOOLS_PATH=/Users/darwin/lab/v8_ws/v8/buildtools
STPYV8_DEPOT_GIT_URL=https://chromium.googlesource.com/chromium/tools/depot_tools.git
STPYV8_GIT_CACHE_PATH=/Users/darwin/lab/stpyv8/.git_cache
STPYV8_GN_EXTRA_ARGS=
STPYV8_PYTHON_CFLAGS=-I/usr/local/Cellar/python/3.7.7/Frameworks/Python.framework/Versions/3.7/include/python3.7m -I/usr/local/Cellar/python/3.7.7/Frameworks/Python.framework/Versions/3.7/include/python3.7m -Wno-unused-result -Wsign-compare -Wunreachable-code -fno-common -dynamic -DNDEBUG -g -fwrapv -O3 -Wall -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk -I/Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk/usr/include -I/Library/Developer/CommandLineTools/SDKs/MacOSX10.15.sdk/System/Library/Frameworks/Tk.framework/Versions/8.5/Headers
STPYV8_PYTHON_INCLUDES=-I/usr/local/Cellar/python/3.7.7/Frameworks/Python.framework/Versions/3.7/include/python3.7m -I/usr/local/Cellar/python/3.7.7/Frameworks/Python.framework/Versions/3.7/include/python3.7m
STPYV8_PYTHON_LDFLAGS=-L/usr/local/opt/python/Frameworks/Python.framework/Versions/3.7/lib/python3.7/config-3.7m-darwin -lpython3.7m -ldl -framework CoreFoundation
STPYV8_PYTHON_LIBS=-lpython3.7m -ldl -framework CoreFoundation
STPYV8_V8_GIT_TAG=8.3.104
STPYV8_V8_GIT_URL=https://chromium.googlesource.com/v8/v8.git
gn:
```

Compile generated build files from previous step: 
```bash
ninja -C _out/8.3.104/release stpyv8
```

When you are done, exit the sub-shell.
```bash
exit
```

##### Run setup.py

```bash
cd ext
python setup.py build

# or for debug build run:
# python setup.py build --debug
```

##### Run install it into our venv

Install final package into our Python3 environment

```bash
cd ext
python setup.py install --prefix ../.venv
```

##### Run tests

```bash
./scripts/test.sh
```

# FAQ

### Where should I put V8 repo?

> V8 repo directory location is specified by `V8_HOME` env variable. By default it goes to `gn/v8`.
> The build system expects it there, so if you specify some other location via explicit `V8_HOME` in your environment
> the build script will create a symlink from `gn/v8` to your `V8_HOME`. 
 
### How do I change V8 revision?

> `export STPYV8_V8_GIT_TAG=${revision}`
>
> Then do `./scripts/checkout-v8.sh` which will pull and checkout specified revision. 
>
> Please make sure that this env variable is persistent in your env. I personally use [direnv](https://direnv.net) 
> for this kind of setup.