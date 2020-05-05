import sys
import unittest
import logging

from naga import JSIsolate, JSContext, JSNull
# noinspection PyUnresolvedReferences
import naga.aux as aux


class TestIsolate(unittest.TestCase):
    def testBase(self):
        with JSIsolate() as isolate:
            self.assertIsNotNone(isolate.current)
            self.assertFalse(isolate.locked)

    def testEnterLeave(self):
        with JSIsolate() as isolate:
            self.assertIsNotNone(isolate.current)

    def testIsolateContexts(self):
        with JSIsolate() as isolate:
            ctx1 = JSContext()
            ctx2 = JSContext()

            self.assertFalse(isolate.in_context)

            with ctx1:
                self.assertEqual(ctx1, isolate.entered_or_microtask_context)
                self.assertEqual(ctx1, isolate.current_context)
                self.assertTrue(isolate.in_context)

                with ctx2:
                    self.assertEqual(ctx2, isolate.entered_or_microtask_context)
                    self.assertEqual(ctx2, isolate.current_context)
                    self.assertTrue(isolate.in_context)

                self.assertEqual(ctx1, isolate.entered_or_microtask_context)
                self.assertEqual(ctx1, isolate.current_context)
                self.assertTrue(isolate.in_context)

            self.assertFalse(isolate.in_context)
            self.assertEqual(JSNull, isolate.entered_or_microtask_context)
            self.assertEqual(JSNull, isolate.current_context)

    def testEncounteringForeignIsolate(self):
        self.assertRaises(RuntimeError, aux.test_encountering_foreign_isolate)


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level=level, format='%(asctime)s %(levelname)s %(message)s')
    unittest.main()
