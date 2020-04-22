#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import unittest
import logging

import naga


class TestContext(unittest.TestCase):
    def testEval(self):
        with naga.JSContext() as context:
            self.assertEqual(2, context.eval("1+1"))
            self.assertEqual('Hello world', context.eval("'Hello ' + 'world'"))

    def testMultiNamespace(self):
        self.assertTrue(not bool(naga.JSContext.inContext))
        self.assertTrue(not bool(naga.JSContext.entered))

        class Global(object):
            name = "global"

        g = Global()

        with naga.JSContext(g) as ctxt:
            self.assertTrue(ctxt)
            self.assertTrue(bool(naga.JSContext.inContext))
            self.assertEqual(g.name, str(naga.JSContext.entered.locals.name))

            class Local(object):
                name = "local"

            local = Local()

            with naga.JSContext(local):
                self.assertTrue(bool(naga.JSContext.inContext))
                self.assertEqual(local.name, str(naga.JSContext.entered.locals.name))

            self.assertTrue(bool(naga.JSContext.inContext))
            self.assertEqual(g.name, str(naga.JSContext.current.locals.name))

        self.assertTrue(not bool(naga.JSContext.entered))
        self.assertTrue(not bool(naga.JSContext.inContext))

    def testMultiContext(self):
        with naga.JSContext() as ctxt0:
            ctxt0.securityToken = "password"

            global0 = ctxt0.locals
            global0.custom = 1234

            self.assertEqual(1234, int(global0.custom))

            with naga.JSContext() as ctxt1:
                ctxt1.securityToken = ctxt0.securityToken

                global1 = ctxt1.locals
                global1.custom = 1234

                with ctxt0:
                    self.assertEqual(1234, int(global0.custom))

                self.assertEqual(1234, int(global1.custom))

    def testSecurityChecks(self):
        with naga.JSContext() as env1:
            env1.securityToken = "foo"

            # Create a function in env1.
            env1.eval("spy = function(){return spy;}")

            spy = env1.locals.spy

            self.assertTrue(isinstance(spy, naga.JSFunction))

            # Create another function accessing global objects.
            env1.eval("spy2 = function(){return 123;}")

            spy2 = env1.locals.spy2

            self.assertTrue(isinstance(spy2, naga.JSFunction))

            # Switch to env2 in the same domain and invoke spy on env2.
            env2 = naga.JSContext()
            env2.securityToken = "foo"

            with env2:
                result = naga.toolkit.apply(spy, env2.locals)
                self.assertTrue(isinstance(result, naga.JSFunction))

            env2.securityToken = "bar"

            # FIXME
            # Call cross_domain_call, it should throw an exception
            # with env2:
            #    self.assertRaises(naga.JSError, naga.toolkit.apply(spy2), env2.locals)


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level=level, format='%(asctime)s %(levelname)s %(message)s')
    unittest.main()
