#!/usr/bin/env python

# simple.py - bind a javascript function to python function

from naga import JSContext

with JSContext() as ctxt:
    upcase = ctxt.eval("""
    ( (lowerString) => {
        return lowerString.toUpperCase();
    })
    """)
    print(upcase("hello world!"))
