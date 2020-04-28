#!/usr/bin/env python

# console.py - run javascript using a custom console

from logging import getLogger, basicConfig, INFO
from naga import JSClass, JSContext

basicConfig(format='%(message)s')
log = getLogger('myapp')
log.setLevel(INFO)


# noinspection PyMethodMayBeStatic
class Console(object):
    def log(self, message):
        log.info(message)

    def error(self, message):
        log.error(message)


class Global(JSClass):
    custom_console = Console()


def load(js_file):
    with open(js_file, 'r') as f:
        return f.read()


def run(js_file):
    with JSContext(Global()) as ctxt:
        ctxt.eval('this.console = custom_console;')
        ctxt.eval(load(js_file))


run('simple.js')
