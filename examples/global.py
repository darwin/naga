#!/usr/bin/env python

# global.py - pass different global objects to javascript

from naga import JSClass, JSContext


# noinspection PyMethodMayBeStatic
class Global(JSClass):
    version = "1.0"

    def hello(self, name):
        return "Hello " + name


with JSContext(Global()) as ctxt:
    print(ctxt.eval("version"))  # 1.0
    print(ctxt.eval("hello('World')"))  # Hello World
    print(ctxt.eval("hello.toString()"))  # function () { [native code] }


# "simulate" a browser, defining the js 'window' object in python
# see https://github.com/buffer/thug for a robust implementation

# noinspection PyMethodMayBeStatic
class Window(JSClass):

    def alert(self, message):
        print("Popping up an alert with message " + message)


with JSContext(Window()) as browser_ctxt:
    browser_ctxt.eval("alert('Hello')")
