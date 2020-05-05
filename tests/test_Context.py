#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import unittest
import logging

import naga.toolkit as toolkit
# noinspection PyUnresolvedReferences
import naga.aux as aux
from naga import JSIsolate, JSContext, JSFunction


class TestContext(unittest.TestCase):
    def testEval(self):
        with JSContext() as context:
            self.assertEqual(2, context.eval("1+1"))
            self.assertEqual('Hello world', context.eval("'Hello ' + 'world'"))

    # TODO: move this to isolate tests
    def testMultiNamespace(self):
        isolate = JSIsolate.current
        self.assertTrue(not isolate.in_context)
        self.assertTrue(isolate.current_context is None)

        class Global(object):
            name = "global"

        global_scope = Global()

        with JSContext(global_scope) as global_ctxt:
            self.assertTrue(global_ctxt)
            self.assertTrue(isolate.in_context)
            self.assertTrue(isolate.current_context is global_ctxt)
            self.assertEqual(global_scope.name, isolate.current_context.locals.name)

            class Local(object):
                name = "local"

            local_scope = Local()

            with JSContext(local_scope) as local_ctxt:
                self.assertTrue(isolate.in_context)
                self.assertTrue(isolate.current_context is local_ctxt)
                self.assertEqual(local_scope.name, isolate.current_context.locals.name)

            self.assertTrue(isolate.in_context)
            self.assertEqual(global_scope.name, isolate.current_context.locals.name)

        self.assertTrue(not isolate.in_context)
        self.assertTrue(isolate.current_context is None)

    def testMultiContext(self):
        with JSContext() as ctxt0:
            ctxt0.security_token = "password"

            global0 = ctxt0.locals
            global0.custom = 1234

            self.assertEqual(1234, int(global0.custom))

            with JSContext() as ctxt1:
                ctxt1.security_token = ctxt0.security_token

                global1 = ctxt1.locals
                global1.custom = 1234

                with ctxt0:
                    self.assertEqual(1234, int(global0.custom))

                self.assertEqual(1234, int(global1.custom))

    def testSecurityChecks(self):
        with JSContext() as env1:
            env1.security_token = "foo"

            # Create a function in env1.
            env1.eval("spy = function(){return spy;}")

            spy = env1.locals.spy

            self.assertTrue(isinstance(spy, JSFunction))

            # Create another function accessing global objects.
            env1.eval("spy2 = function(){return 123;}")

            spy2 = env1.locals.spy2

            self.assertTrue(isinstance(spy2, JSFunction))

            # Switch to env2 in the same domain and invoke spy on env2.
            env2 = JSContext()
            env2.security_token = "foo"

            with env2:
                result = toolkit.apply(spy, env2.locals)
                self.assertTrue(isinstance(result, JSFunction))

            env2.security_token = "bar"

            # FIXME
            # Call cross_domain_call, it should throw an exception
            # with env2:
            #    self.assertRaises(JSError, toolkit.apply(spy2), env2.locals)

    def testEncounteringForeignContext(self):
        self.assertRaises(RuntimeError, aux.test_encountering_foreign_context)


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level=level, format='%(asctime)s %(levelname)s %(message)s')
    unittest.main()
