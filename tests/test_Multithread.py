#!/usr/bin/env python
# -*- coding: utf-8 -*-

import logging
import sys
import unittest

from naga import JSIsolate, JSContext


class TestMultithread(unittest.TestCase):
    def testIsolateLocking(self):

        isolate = JSIsolate()
        self.assertFalse(isolate.locked)
        self.assertEqual(0, isolate.lock_level)
        isolate.lock()
        self.assertTrue(isolate.locked)
        self.assertEqual(1, isolate.lock_level)
        isolate.unlock()
        self.assertFalse(isolate.locked)
        self.assertEqual(0, isolate.lock_level)
        isolate.lock()
        isolate.lock()
        self.assertTrue(isolate.locked)
        self.assertEqual(2, isolate.lock_level)
        isolate.unlock()
        self.assertTrue(isolate.locked)
        self.assertEqual(1, isolate.lock_level)
        isolate.lock()
        isolate.lock()
        self.assertTrue(isolate.locked)
        self.assertEqual(3, isolate.lock_level)
        isolate.unlock_all()
        self.assertFalse(isolate.locked)
        self.assertEqual(-3, isolate.lock_level)
        isolate.relock_all()
        self.assertTrue(isolate.locked)
        self.assertEqual(3, isolate.lock_level)
        isolate.unlock()
        isolate.unlock()
        isolate.unlock()
        self.assertFalse(isolate.locked)
        self.assertEqual(0, isolate.lock_level)

    def testIsolateWrapperAutoLocking(self):

        isolate1 = JSIsolate()
        with isolate1:
            self.assertTrue(isolate1.locked)
            isolate1.unlock()
            self.assertFalse(isolate1.locked)
            isolate1.lock()
            self.assertTrue(isolate1.locked)
        self.assertFalse(isolate1.locked)

        isolate2 = JSIsolate()
        with isolate2:
            self.assertTrue(isolate2.locked)
            isolate2.lock()
            self.assertTrue(isolate2.locked)
            isolate2.unlock_all()
            self.assertFalse(isolate2.locked)
            isolate2.relock_all()
            self.assertTrue(isolate2.locked)
            isolate2.unlock()
            self.assertTrue(isolate2.locked)
        self.assertFalse(isolate2.locked)

        with JSIsolate() as isolate3:
            self.assertTrue(isolate3.locked)
            with JSIsolate() as isolate4:
                self.assertTrue(isolate4.locked)
                with JSIsolate() as isolate5:
                    self.assertTrue(isolate5.locked)
                self.assertTrue(isolate4.locked)
            self.assertTrue(isolate3.locked)

    def testMultiPythonThread(self):
        import time
        import threading

        class Global:
            count = 0
            started = threading.Event()
            finished = threading.Semaphore(0)

            def sleep(self, ms):
                time.sleep(ms / 1000.0)

                self.count += 1

        g = Global()

        def run():
            with JSIsolate():
                with JSContext(g) as ctxt:
                    ctxt.eval("""
                        started.wait();

                        for (i=0; i<10; i++)
                        {
                            sleep(100);
                        }

                        finished.release();
                    """)

        threading.Thread(target=run).start()

        now = time.time()

        self.assertEqual(0, g.count)

        g.started.set()
        g.finished.acquire()

        self.assertEqual(10, g.count)

        self.assertTrue((time.time() - now) >= 1)

    # def _testMultiJavascriptThread(self):
    #     import time
    #     import threading
    #
    #     class Global:
    #         result = []
    #
    #         def add(self, value):
    #             with JSUnlocker():
    #                 time.sleep(0.1)
    #
    #                 self.result.append(value)
    #
    #     g = Global()
    #
    #     def run():
    #         with JSContext(g) as ctxt:
    #             ctxt.eval("""
    #                 for (i=0; i<10; i++)
    #                     add(i);
    #             """)
    #
    #     threads = [threading.Thread(target=run), threading.Thread(target=run)]
    #
    #     with JSLocker():
    #         for t in threads:
    #             t.start()
    #
    #     for t in threads:
    #         t.join()
    #
    #     self.assertEqual(20, len(g.result))


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level=level, format='%(asctime)s %(levelname)s %(message)s')
    unittest.main()
