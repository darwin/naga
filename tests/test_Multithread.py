#!/usr/bin/env python
# -*- coding: utf-8 -*-

import logging
import sys
import unittest

import naga


class TestMultithread(unittest.TestCase):
    def testLocker(self):
        with naga.JSIsolate():
            self.assertFalse(naga.JSLocker.active)
            self.assertFalse(naga.JSLocker.locked)

            with naga.JSLocker() as outer_locker:
                self.assertTrue(naga.JSLocker.active)
                self.assertTrue(naga.JSLocker.locked)

                self.assertTrue(outer_locker)

                with naga.JSLocker() as inner_locker:
                    self.assertTrue(naga.JSLocker.locked)

                    self.assertTrue(outer_locker)
                    self.assertTrue(inner_locker)

                    with naga.JSUnlocker():
                        self.assertFalse(naga.JSLocker.locked)

                        self.assertTrue(outer_locker)
                        self.assertTrue(inner_locker)

                    self.assertTrue(naga.JSLocker.locked)

            self.assertTrue(naga.JSLocker.active)
            self.assertFalse(naga.JSLocker.locked)

            locker = naga.JSLocker()

        with naga.JSContext():
            self.assertRaises(RuntimeError, locker.__enter__)
            self.assertRaises(RuntimeError, locker.__exit__, None, None, None)

        del locker

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
            with naga.JSIsolate():
                with naga.JSContext(g) as ctxt:
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

    def _testMultiJavascriptThread(self):
        import time
        import threading

        class Global:
            result = []

            def add(self, value):
                with naga.JSUnlocker():
                    time.sleep(0.1)

                    self.result.append(value)

        g = Global()

        def run():
            with naga.JSContext(g) as ctxt:
                ctxt.eval("""
                    for (i=0; i<10; i++)
                        add(i);
                """)

        threads = [threading.Thread(target=run), threading.Thread(target=run)]

        with naga.JSLocker():
            for t in threads:
                t.start()

        for t in threads:
            t.join()

        self.assertEqual(20, len(g.result))


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level=level, format='%(asctime)s %(levelname)s %(message)s')
    unittest.main()
