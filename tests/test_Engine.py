#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import unittest
import logging

from naga import JSContext, JSEngine, JSScript, JSClass, JSFunction, JSUndefined, JSIsolate
import naga.toolkit as toolkit


class TestEngine(unittest.TestCase):
    def testClassProperties(self):
        with JSContext():
            self.assertTrue(str(JSEngine.version).startswith("8."))
            self.assertFalse(JSEngine.dead)

    def testCompile(self):
        with JSContext():
            with JSEngine() as engine:
                s = engine.compile("1+2")

                self.assertTrue(isinstance(s, JSScript))

                self.assertEqual("1+2", s.source)
                self.assertEqual(3, int(s.run()))

                self.assertRaises(SyntaxError, engine.compile, "1+")

    def testUnicodeSource(self):
        class Global(JSClass):
            var = u'测试'

            def __getattr__(self, name):
                if name:
                    return self.var

                return JSClass.__getattr__(self, name)

        g = Global()

        with JSContext(g) as ctxt:
            with JSEngine() as engine:
                src = u"""
                function 函数() { return 变量.length; }

                函数();

                var func = function () {};
                """

                s = engine.compile(src)

                self.assertTrue(isinstance(s, JSScript))

                self.assertEqual(src, s.source)
                self.assertEqual(2, s.run())

                func_name = u'函数'

                self.assertTrue(hasattr(ctxt.locals, func_name))

                func = getattr(ctxt.locals, func_name)

                self.assertTrue(isinstance(func, JSFunction))

                self.assertEqual(func_name, toolkit.get_name(func))
                self.assertEqual("", toolkit.resource_name(func))
                self.assertEqual(1, toolkit.line_number(func))
                self.assertEqual(0, toolkit.line_offset(func))
                self.assertEqual(0, toolkit.column_offset(func))

                var_name = u'变量'

                setattr(ctxt.locals, var_name, u'测试长字符串')

                self.assertEqual(6, func())

                self.assertEqual("func", toolkit.inferred_name(ctxt.locals.func))

    def testEval(self):
        with JSContext() as ctxt:
            self.assertEqual(3, int(ctxt.eval("1+2")))

    def testGlobal(self):
        class Global(JSClass):
            version = "1.0"

        with JSContext(Global()) as ctxt:
            _vars = ctxt.locals

            # getter
            self.assertEqual(Global.version, str(_vars.version))
            self.assertEqual(Global.version, str(ctxt.eval("version")))

            self.assertRaises(ReferenceError, ctxt.eval, "nonexists")

            # setter
            self.assertEqual(2.0, float(ctxt.eval("version = 2.0")))

            self.assertEqual(2.0, float(_vars.version))

    def testThis(self):
        class Global(JSClass):
            version = 1.0

        with JSContext(Global()) as ctxt:
            self.assertEqual("[object Global]", str(ctxt.eval("this")))
            self.assertEqual(1.0, float(ctxt.eval("this.version")))

    def testObjectBuiltInMethods(self):
        class Global(JSClass):
            version = 1.0

        with JSContext(Global()) as ctxt:
            self.assertEqual("[object Global]", str(ctxt.eval("this.toString()")))
            self.assertEqual("[object Global]", str(ctxt.eval("this.toLocaleString()")))
            self.assertEqual(Global.version, float(ctxt.eval("this.valueOf()").version))

            self.assertTrue(bool(ctxt.eval("this.hasOwnProperty(\"version\")")))

            self.assertFalse(ctxt.eval("this.hasOwnProperty(\"nonexistent\")"))

    def testPythonWrapper(self):
        class Global(JSClass):
            s = [1, 2, 3]
            d = {'a': {'b': 'c'}, 'd': ['e', 'f']}

        g = Global()

        with JSContext(g) as ctxt:
            ctxt.eval("""
                s[2] = s[1] + 2;
                s[0] = s[1];
                delete s[1];
            """)
            self.assertEqual([2, 4], g.s)
            self.assertEqual('c', ctxt.eval("d.a.b"))
            self.assertEqual(['e', 'f'], ctxt.eval("d.d"))
            ctxt.eval("""
                d.a.q = 4
                delete d.d
            """)
            self.assertEqual(4, g.d['a']['q'])
            self.assertEqual(JSUndefined, ctxt.eval("d.d"))

    def _testMemoryAllocationCallback(self):
        alloc = {}

        def callback(space, action, size):
            alloc[(space, action)] = alloc.setdefault((space, action), 0) + size

        JSEngine.setMemoryAllocationCallback(callback)

        with JSContext() as ctxt:
            # noinspection PyUnresolvedReferences
            self.assertFalse((JSObjectSpace.Code, JSAllocationAction.alloc) in alloc)

            ctxt.eval("var o = new Array(1000);")

            # noinspection PyUnresolvedReferences
            self.assertTrue((JSObjectSpace.Code, JSAllocationAction.alloc) in alloc)

        JSEngine.setMemoryAllocationCallback(None)

    def _testOutOfMemory(self):
        with JSIsolate():
            JSEngine.setMemoryLimit(max_young_space_size=16 * 1024, max_old_space_size=4 * 1024 * 1024)

            with JSContext() as ctxt:
                JSEngine.ignoreOutOfMemoryException()

                ctxt.eval("var a = new Array(); while(true) a.push(a);")

                self.assertTrue(ctxt.hasOutOfMemoryException)

                JSEngine.setMemoryLimit()

                JSEngine.collect()

    def testStackLimit(self):
        with JSIsolate():
            JSEngine.set_stack_limit(256 * 1024)

            with JSContext() as ctxt:
                old_stack_size = ctxt.eval(
                    "var maxStackSize = function(i){try{(function m(){++i&&m()}())}catch(e){return i}}(0); maxStackSize")

        with JSIsolate():
            JSEngine.set_stack_limit(512 * 1024)

            with JSContext() as ctxt:
                new_stack_size = ctxt.eval(
                    "var maxStackSize = function(i){try{(function m(){++i&&m()}())}catch(e){return i}}(0); maxStackSize")

        self.assertTrue(new_stack_size > old_stack_size * 2)


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level=level, format='%(asctime)s %(levelname)s %(message)s')
    unittest.main()
