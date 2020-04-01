#!/usr/bin/env python

import os
import shutil

from distutils.command.build_ext import build_ext
from distutils.command.install import install

try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension

V8_GIT_TAG_STABLE = "8.3.104"
V8_GIT_TAG_MASTER = "master"

STPYV8_V8_GIT_TAG = os.environ.get('STPYV8_V8_GIT_TAG', V8_GIT_TAG_STABLE)
STPYV8_VERSION = STPYV8_V8_GIT_TAG


# here we override standard Extension build,
# to simply check for existence and copy existing pre-built binary
class BuildExtCmd(build_ext):

    def get_input_path(self):
        build_config = "debug" if self.debug else "release"
        return os.path.join("..", "gn", "_out", STPYV8_VERSION, build_config, "libstpyv8.so")

    def get_output_path(self):
        name = self.extensions[0].name
        return self.get_ext_fullpath(name)

    def build_extension(self, ext):
        # this is intentionally a no-op
        return

    def run(self):
        # the extension has only name and should be empty so nothing should be happening here
        # anyways, we let normal build_ext run and then finish the work ourselves
        build_ext.run(self)

        input_binary = self.get_input_path()
        output_binary = self.get_output_path()

        if not os.path.isfile(input_binary):
            msg = "Expected pre-compiled file '{}' does not exists. Follow build steps.".format(input_binary)
            raise Exception(msg)

        output_parent_dir = os.path.abspath(os.path.join(output_binary, os.pardir))
        print(output_parent_dir)
        os.makedirs(output_parent_dir, exist_ok=True)
        assert (not os.path.isdir(output_binary))
        shutil.copy2(input_binary, output_binary)
        print("copied '{}' to '{}'\n".format(input_binary, output_binary))


class InstallCmd(install):
    skip_build = False

    def run(self):
        # TODO: why is this here? explain
        self.skip_build = True
        install.run(self)


stpyv8_ext = Extension(name="_STPyV8", sources=[])

setup(name="stpyv8",
      version=STPYV8_VERSION,
      description="Python Wrapper for Google V8 Engine",
      platforms="x86",
      author="Philip Syme, Angelo Dell'Aera",
      url="https://github.com/area1/stpyv8",
      license="Apache License 2.0",
      py_modules=["STPyV8"],
      ext_modules=[stpyv8_ext],
      classifiers=[
          "Development Status :: 4 - Beta",
          "Environment :: Plugins",
          "Intended Audience :: Developers",
          "Intended Audience :: System Administrators",
          "License :: OSI Approved :: Apache Software License",
          "Natural Language :: English",
          "Operating System :: POSIX",
          "Programming Language :: C++",
          "Programming Language :: Python",
          "Topic :: Internet",
          "Topic :: Internet :: WWW/HTTP",
          "Topic :: Software Development",
          "Topic :: Software Development :: Libraries :: Python Modules",
          "Topic :: Utilities",
      ],
      cmdclass=dict(
          build_ext=BuildExtCmd,
          install=InstallCmd),
      )
