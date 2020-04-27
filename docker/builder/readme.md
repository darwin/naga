### A quick test drive

```bash
# to build the docker container:
./do.sh build

# to build and install the naga library inside the container:
./do.sh run build

# to test built naga library inside the container:
./do.sh run test
```

By default it builds it in release mode. Here are alternatives:

```bash
# to clear naga work dir inside container (and possibly other caches)
./do.sh clear-caches

# or nuclear option, which will get rid of the docker image as well
./do.sh clear 

# to build the docker container from scratch without docker caching:
./do.sh build --no-cache

# to build naga in debug mode
./do.sh run build debug

# to test only matching unit tests
./do.sh run test -k testCompile
```

### Usage

```
> ./do.sh --help
A convenience wrapper for Naga builder Dockerfile.

Usage:

  ./do.sh command [args]

Commands:

  build [args]  - build the docker image, args will be passed to the docker build
  clear         - clear all generated docker images/volumes
  clear-caches  - clear only docker volumes holding caches
  enter [cmd]   - enter built docker image with custom command,
                  e.g. use `./do.sh enter bash` to use bash shell interactively
  run [args]    - run built docker image with rest arguments passed
                  into builder.sh running inside the container.

Run sub-commands:

  run build [args]   - build and install naga library inside the container
                       args will be passed to the scripts/build.sh script
                       e.g. `./do.sh run build debug` will build it in debug mode (defaults to release)
  run test [args]    - run test script on built naga library inside the container
                       args will be passed to the scripts/test.sh script
                       e.g. `./do.sh run test -k testJavascriptWrapper` will run only particular test

Discussion:

  Environment

    It is possible to customize the docker behaviour with env variables.
    Please note that we take all NAGA_ prefixed env variables and "export" them into the container.
    This is handy for example for setting NAGA_V8_GIT_TAG or tweaking other settings.
    Just be aware that not all NAGA env variables will have effect, some variables are hard-coded
    in the container for its proper function, for example NAGA_WORK_DIR is set internally to have
    control over work dir location inside the container.
```