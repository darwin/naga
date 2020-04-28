#!/usr/bin/env python

# meaning.py - call a python method from javascript code

from naga import JSClass, JSContext


# noinspection PyMethodMayBeStatic,PyPep8Naming
class MyClass(JSClass):
    def reallyComplexFunction(self, addme):
        return 10 * 3 + addme


my_class = MyClass()

with JSContext(my_class) as ctxt:
    meaning = ctxt.eval("this.reallyComplexFunction(2) + 10;")
    print("The meaning of life: " + str(meaning))
