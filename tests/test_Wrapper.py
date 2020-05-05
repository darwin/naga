#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os
import unittest
import logging
import datetime
from io import StringIO

from naga import JSObject, JSContext, JSClass, JSIsolate, JSFunction, JSEngine, JSError, JSUndefined, JSNull, JSArray, \
    JSStackTrace
import naga.aux as aux
import naga.toolkit as toolkit


def convert(obj):
    if isinstance(obj, JSObject):
        if toolkit.has_role_array(obj):
            return [convert(v) for v in obj]
        else:
            return dict([[str(k), convert(obj.__getattr__(str(k)))] for k in obj.__dir__()])

    return obj


class TestWrapper(unittest.TestCase):
    def testObject(self):
        with JSContext() as ctxt:
            o = ctxt.eval("new Object()")

            self.assertTrue(hash(o) > 0)

            o1 = toolkit.clone(o)

            self.assertEqual(hash(o1), hash(o))
            self.assertTrue(o != o1)

    def testAutoConverter(self):
        with JSContext() as ctxt:
            ctxt.eval("""
                var_i = 1;
                var_f = 1.0;
                var_s = "test";
                var_b = true;
                var_s_obj = new String("test");
                var_b_obj = new Boolean(true);
                var_f_obj = new Number(1.5);
            """)

            _vars = ctxt.locals

            var_i = _vars.var_i

            self.assertTrue(var_i)
            self.assertEqual(1, int(var_i))

            var_f = _vars.var_f

            self.assertTrue(var_f)
            self.assertEqual(1.0, float(_vars.var_f))

            var_s = _vars.var_s
            self.assertTrue(var_s)
            self.assertEqual("test", str(_vars.var_s))

            var_b = _vars.var_b
            self.assertTrue(var_b)
            self.assertTrue(bool(var_b))

            self.assertEqual("test", _vars.var_s_obj)
            self.assertTrue(_vars.var_b_obj)
            self.assertEqual(1.5, _vars.var_f_obj)

            attrs = dir(ctxt.locals)

            self.assertTrue(attrs)
            self.assertTrue("var_i" in attrs)
            self.assertTrue("var_f" in attrs)
            self.assertTrue("var_s" in attrs)
            self.assertTrue("var_b" in attrs)
            self.assertTrue("var_s_obj" in attrs)
            self.assertTrue("var_b_obj" in attrs)
            self.assertTrue("var_f_obj" in attrs)

    def testExactConverter(self):
        class MyInteger(int, JSClass):
            pass

        class MyString(str, JSClass):
            pass

        class MyDateTime(datetime.time, JSClass):
            pass

        class Global(JSClass):
            var_bool = True
            var_int = 1
            var_float = 1.0
            var_str = 'str'
            var_datetime = datetime.datetime.now()
            var_date = datetime.date.today()
            var_time = datetime.time()

            var_myint = MyInteger()
            var_mystr = MyString('mystr')
            var_mytime = MyDateTime()

        with JSContext(Global()) as ctxt:
            typename = ctxt.eval("(function (name) { return this[name].constructor.name; })")
            typeof = ctxt.eval("(function (name) { return typeof(this[name]); })")

            self.assertEqual('Boolean', typename('var_bool'))
            self.assertEqual('Number', typename('var_int'))
            self.assertEqual('Number', typename('var_float'))
            self.assertEqual('String', typename('var_str'))
            self.assertEqual('Date', typename('var_datetime'))
            self.assertEqual('Date', typename('var_date'))
            self.assertEqual('Date', typename('var_time'))

            self.assertEqual('MyInteger', typename('var_myint'))
            self.assertEqual('MyString', typename('var_mystr'))
            self.assertEqual('MyDateTime', typename('var_mytime'))

            self.assertEqual('function', typeof('var_myint'))
            self.assertEqual('function', typeof('var_mystr'))
            self.assertEqual('function', typeof('var_mytime'))

    def testJavascriptWrapper(self):
        with JSContext() as ctxt:
            self.assertEqual(type(JSNull), type(ctxt.eval("null")))
            self.assertEqual(type(JSUndefined), type(ctxt.eval("undefined")))
            self.assertEqual(bool, type(ctxt.eval("true")))
            self.assertEqual(str, type(ctxt.eval("'test'")))
            self.assertEqual(int, type(ctxt.eval("123")))
            self.assertEqual(float, type(ctxt.eval("3.14")))
            self.assertEqual(datetime.datetime, type(ctxt.eval("new Date()")))
            self.assertEqual(JSArray, type(ctxt.eval("[1, 2, 3]")))
            self.assertEqual(JSFunction, type(ctxt.eval("(function() {})")))
            self.assertEqual(JSObject, type(ctxt.eval("new Object()")))

    def testPythonWrapper(self):
        with JSContext() as ctxt:
            typeof = ctxt.eval("(function type(value) { return typeof value; })")
            protoof = ctxt.eval("(function protoof(value) { return Object.prototype.toString.apply(value); })")

            self.assertEqual('[object Null]', protoof(None))
            self.assertEqual('boolean', typeof(True))
            self.assertEqual('number', typeof(123))
            self.assertEqual('number', typeof(3.14))
            self.assertEqual('string', typeof('test'))
            self.assertEqual('string', typeof(u'test'))

            self.assertEqual('[object Date]', protoof(datetime.datetime.now()))
            self.assertEqual('[object Date]', protoof(datetime.date.today()))
            self.assertEqual('[object Date]', protoof(datetime.time()))

            def test():
                pass

            self.assertEqual('[object Function]', protoof(abs))
            self.assertEqual('[object Function]', protoof(test))
            self.assertEqual('[object Function]', protoof(self.testPythonWrapper))
            self.assertEqual('[object Function]', protoof(int))

    def testFunction(self):
        with JSContext() as ctxt:
            func = ctxt.eval("""
                (function ()
                {
                    function a()
                    {
                        return "abc";
                    }

                    return a();
                })
                """)

            self.assertEqual("abc", str(func()))
            self.assertTrue(func is not None)
            self.assertFalse(func is None)

            func = ctxt.eval("(function test() {})")

            self.assertEqual("test", toolkit.get_name(func))
            self.assertEqual("", toolkit.resource_name(func))
            self.assertEqual(0, toolkit.line_number(func))
            self.assertEqual(14, toolkit.column_number(func))
            self.assertEqual(0, toolkit.line_offset(func))
            self.assertEqual(0, toolkit.column_offset(func))

            # FIXME
            # Why the setter doesn't work?
            #
            # func.name = "hello"
            # self.assertEqual("hello", func.name)

            # func.setName("hello")
            # self.assertEqual("hello", func.name)

    def testCall(self):
        class Hello(object):
            def __call__(self, name):
                return "hello " + name

        class Global(JSClass):
            hello = Hello()

        with JSContext(Global()) as ctxt:
            self.assertEqual("hello world", ctxt.eval("hello('world')"))

    def testJSFunction(self):
        with JSContext() as ctxt:
            hello = ctxt.eval("(function (name) { return 'Hello ' + name; })")

            self.assertTrue(isinstance(hello, JSFunction))
            self.assertEqual("Hello world", hello('world'))
            self.assertEqual("Hello world", toolkit.invoke(hello, ['world']))

            obj = ctxt.eval(
                "({ 'name': 'world', 'hello': function (name) { return 'Hello ' + name + ' from ' + this.name; }})")
            hello = obj.hello
            self.assertTrue(isinstance(hello, JSFunction))
            self.assertEqual("Hello world from world", hello('world'))

            tester = ctxt.eval("({ 'name': 'tester' })")
            self.assertEqual("Hello world from tester", toolkit.apply(hello, tester, ['world']))
            self.assertEqual("Hello world from json", toolkit.apply(hello, {'name': 'json'}, ['world']))

    def testConstructor(self):
        with JSContext() as ctx:
            ctx.eval("""
                var Test = function() {
                    this.trySomething();
                };
                Test.prototype.trySomething = function() {
                    this.name = 'soirv8';
                };

                var Test2 = function(first_name, last_name) {
                    this.name = first_name + ' ' + last_name;
                };
                """)

            self.assertTrue(isinstance(ctx.locals.Test, JSFunction))

            test = toolkit.create(ctx.locals.Test)

            self.assertTrue(isinstance(ctx.locals.Test, JSObject))
            self.assertEqual("soirv8", test.name)

            test2 = toolkit.create(ctx.locals.Test2, ('John', 'Doe'))

            self.assertEqual("John Doe", test2.name)

            test3 = toolkit.create(ctx.locals.Test2, ('John', 'Doe'), {'email': 'john.doe@randommail.com'})

            self.assertEqual("john.doe@randommail.com", test3.email)

    def testJSError(self):
        with JSContext() as ctxt:
            # noinspection PyBroadException
            try:
                ctxt.eval('throw "test"')
                self.fail()
            except Exception:
                self.assertTrue(JSError, sys.exc_info()[0])

    def testErrorInfo(self):
        with JSContext():
            with JSEngine() as engine:
                try:
                    engine.compile("""
                        function hello()
                        {
                            throw Error("hello world");
                        }

                        hello();""", "test", 10, 10).run()
                    self.fail()
                except JSError as e:
                    self.assertTrue("JSError: Error: hello world ( test @ 14 : 28 )  ->" in str(e))
                    self.assertEqual("Error", e.name)
                    self.assertEqual("hello world", e.message)
                    self.assertEqual("test", e.script_name)
                    self.assertEqual(14, e.line_number)
                    self.assertEqual(96, e.startpos)
                    self.assertEqual(97, e.endpos)
                    self.assertEqual(28, e.startcol)
                    self.assertEqual(29, e.endcol)
                    self.assertEqual('throw Error("hello world");', e.source_line.strip())

                    expected_stack_trace = \
                        'Error: hello world\n' \
                        '    at hello (test:14:35)\n' \
                        '    at test:17:25'
                    self.assertEqual(expected_stack_trace, e.stack_trace)

                    f = StringIO()
                    e.print_stack_trace(f)
                    self.assertEqual(expected_stack_trace, f.getvalue())

    def testParseStack(self):
        self.assertEqual([
            ('Error', 'unknown source', None, None),
            ('test', 'native', None, None),
            ('<anonymous>', 'test0', 3, 5),
            ('f', 'test1', 2, 19),
            ('g', 'test2', 1, 15),
            (None, 'test3', 1, None),
            (None, 'test3', 1, 1),
        ], JSError.parse_stack("""Error: err
            at Error (unknown source)
            at test (native)
            at new <anonymous> (test0:3:5)
            at f (test1:2:19)
            at g (test2:1:15)
            at test3:1
            at test3:1:1"""))

    def testStackTrace(self):
        # noinspection PyPep8Naming,PyMethodMayBeStatic
        class Global(JSClass):
            def getCurrentStackTrace(self, _limit):
                return JSIsolate.current.get_current_stack_trace(4, JSStackTrace.Options.Detailed)

        with JSContext(Global()) as ctxt:
            st = ctxt.eval("""
                function a()
                {
                    return getCurrentStackTrace(10);
                }
                function b()
                {
                    return eval("a()");
                }
                function c()
                {
                    return new b();
                }
            c();""", "test")

            self.assertEqual(4, len(st))
            self.assertEqual("\tat a (test:4:28)\n\tat eval ((eval))\n\tat b (test:8:28)\n\tat c (test:12:28)\n",
                             str(st))
            self.assertEqual("test.a (4:28)\n.eval (1:1) eval\ntest.b (8:28) constructor\ntest.c (12:28)",
                             "\n".join(["%s.%s (%d:%d)%s%s" % (
                                 f.script_name, f.function_name, f.line_number, f.column_number,
                                 ' eval' if f.is_eval else '',
                                 ' constructor' if f.is_constructor else '') for f in st]))

    def testPythonException(self):
        # noinspection PyPep8Naming
        class Global(JSClass):
            def raiseException(self):
                raise RuntimeError("Hello")

        with JSContext(Global()) as ctxt:
            ctxt.eval("""
                msg ="";
                try
                {
                    this.raiseException()
                }
                catch(e)
                {
                    msg += "catch " + e + ";";
                }
                finally
                {
                    msg += "finally";
                }""")
            self.assertEqual("catch Error: Hello;finally", str(ctxt.locals.msg))

    def testExceptionMapping(self):
        class TestException(Exception):
            pass

        # noinspection PyPep8Naming,PyMethodMayBeStatic
        class Global(JSClass):
            def raiseIndexError(self):
                return [1, 2, 3][5]

            def raiseAttributeError(self):
                # noinspection PyUnresolvedReferences
                None.hello()

            def raiseSyntaxError(self):
                eval("???")

            def raiseTypeError(self):
                # noinspection PyTypeChecker
                int(sys)

            def raiseNotImplementedError(self):
                raise NotImplementedError("Not supported")

            def raiseExceptions(self):
                raise TestException()

        with JSContext(Global()) as ctxt:
            ctxt.eval("try { this.raiseIndexError(); } catch (e) { msg = e; }")

            self.assertEqual("RangeError: list index out of range", str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseAttributeError(); } catch (e) { msg = e; }")

            self.assertEqual("ReferenceError: 'NoneType' object has no attribute 'hello'", str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseSyntaxError(); } catch (e) { msg = e; }")

            self.assertEqual("SyntaxError: ('invalid syntax', ('<string>', 1, 1, '???'))", str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseTypeError(); } catch (e) { msg = e; }")

            self.assertEqual(
                "TypeError: int() argument must be a string, a bytes-like object or a number, not 'module'",
                str(ctxt.locals.msg))

            ctxt.eval("try { this.raiseNotImplementedError(); } catch (e) { msg = e; }")

            self.assertEqual("Error: Not supported", str(ctxt.locals.msg))

            self.assertRaises(TestException, ctxt.eval, "this.raiseExceptions();")

    def testArray(self):
        with JSContext() as ctxt:
            array = ctxt.eval("""
                var array = new Array();

                for (i=0; i<10; i++)
                {
                    array[i] = 10-i;
                }

                array;
                """)

            self.assertTrue(isinstance(array, JSArray))
            self.assertEqual(10, len(array))

            self.assertTrue(5 in array)
            self.assertFalse(15 in array)

            self.assertEqual(10, len(array))

            for i in range(10):
                self.assertEqual(10 - i, array[i])

            array[5] = 0

            self.assertEqual(0, array[5])

            del array[5]

            self.assertEqual(None, array[5])

            # array         [10, 9, 8, 7, 6, None, 4, 3, 2, 1]
            # array[4:7]                  4^^^^^^^^^7
            # array[-3:-1]                         -3^^^^^^-1
            # array[0:0]    []

            self.assertEqual([6, JSUndefined, 4], array[4:7])
            self.assertEqual([3, 2], array[-3:-1])
            self.assertEqual([], array[0:0])

            self.assertEqual([10, 9, 8, 7, 6, None, 4, 3, 2, 1], list(array))

            array[1:3] = [9, 9]

            self.assertEqual([10, 9, 9, 7, 6, None, 4, 3, 2, 1], list(array))

            array[5:7] = [8, 8]

            self.assertEqual([10, 9, 9, 7, 6, 8, 8, 3, 2, 1], list(array))

            del array[1]

            self.assertEqual([10, None, 9, 7, 6, 8, 8, 3, 2, 1], list(array))

            # TODO: provide similar functionality in the future
            # ctxt.locals.array1 = JSArray(5)
            # ctxt.locals.array2 = JSArray([1, 2, 3, 4, 5])
            #
            # for i in range(len(ctxt.locals.array2)):
            #     ctxt.locals.array1[i] = ctxt.locals.array2[i] * 10
            #
            # ctxt.eval("""
            #     var sum = 0;
            #
            #     for (i=0; i<array1.length; i++)
            #         sum += array1[i]
            #
            #     for (i=0; i<array2.length; i++)
            #         sum += array2[i]
            #     """)
            #
            # self.assertEqual(165, ctxt.locals.sum)

            ctxt.locals.array3 = [1, 2, 3, 4, 5]
            self.assertTrue(ctxt.eval('array3[1] === 2'))
            self.assertTrue(ctxt.eval('array3[9] === undefined'))

            args = [
                ["a = Array(7); for(i=0; i<a.length; i++) a[i] = i; a[3] = undefined; a[a.length-1]; a", "0,1,2,,4,5,6",
                 [0, 1, 2, JSUndefined, 4, 5, 6]],
                ["a = Array(7); for(i=0; i<a.length - 1; i++) a[i] = i; a[a.length-1]; a", "0,1,2,3,4,5,",
                 [0, 1, 2, 3, 4, 5, None]],
                ["a = Array(7); for(i=1; i<a.length; i++) a[i] = i; a[a.length-1]; a", ",1,2,3,4,5,6",
                 [None, 1, 2, 3, 4, 5, 6]]
            ]

            for arg in args:
                array = ctxt.eval(arg[0])

                self.assertEqual(arg[1], str(array))
                self.assertEqual(arg[2], [array[i] for i in range(len(array))])

            # TODO: provide similar functionality in the future
            # self.assertEqual(3, ctxt.eval("(function (arr) { return arr.length; })")(JSArray([1, 2, 3])))
            # self.assertEqual(2, ctxt.eval("(function (arr, idx) { return arr[idx]; })")(JSArray([1, 2, 3]), 1))
            # self.assertEqual('[object Array]',
            # ctxt.eval("(function (arr) { return Object.prototype.toString.call(arr); })")(JSArray([1, 2, 3])))
            # self.assertEqual('[object Array]',
            # ctxt.eval("(function (arr) { return Object.prototype.toString.call(arr); })")(JSArray((1, 2, 3))))
            # self.assertEqual('[object Array]',
            # ctxt.eval("(function (arr) { return Object.prototype.toString.call(arr); })")
            # (JSArray(list(range(3)))))

    def testArraySlices(self):
        with JSContext() as ctxt:
            array = ctxt.eval("""
                var array = new Array();
                array;
            """)

            array[2:4] = [42, 24]
            # array         [None, None, 42, 24]
            self.assertEqual(len(array), 4)
            self.assertEqual(ctxt.eval('array[0]'), None)
            self.assertEqual(ctxt.eval('array[1]'), None)
            self.assertEqual(ctxt.eval('array[2]'), 42)
            self.assertEqual(ctxt.eval('array[3]'), 24)

            array[2:4] = [1, 2]
            # array         [None, None, 1, 2]
            self.assertEqual(len(array), 4)
            self.assertEqual(ctxt.eval('array[0]'), None)
            self.assertEqual(ctxt.eval('array[1]'), None)
            self.assertEqual(ctxt.eval('array[2]'), 1)
            self.assertEqual(ctxt.eval('array[3]'), 2)

            array[0:4:2] = [7, 8]
            # array         [7, None, 8, 2]
            self.assertEqual(len(array), 4)
            self.assertEqual(ctxt.eval('array[0]'), 7)
            self.assertEqual(ctxt.eval('array[1]'), None)
            self.assertEqual(ctxt.eval('array[2]'), 8)
            self.assertEqual(ctxt.eval('array[3]'), 2)

            array[1:4] = [10]
            # array         [7, 10, None, None]
            self.assertEqual(len(array), 4)
            self.assertEqual(ctxt.eval('array[0]'), 7)
            self.assertEqual(ctxt.eval('array[1]'), 10)
            self.assertEqual(ctxt.eval('array[2]'), JSUndefined)
            self.assertEqual(ctxt.eval('array[3]'), JSUndefined)

            array[0:7] = [0, 1, 2]
            # array         [0, 1, 2, None, None, None, None]
            self.assertEqual(len(array), 7)
            self.assertEqual(ctxt.eval('array[0]'), 0)
            self.assertEqual(ctxt.eval('array[1]'), 1)
            self.assertEqual(ctxt.eval('array[2]'), 2)
            self.assertEqual(ctxt.eval('array[3]'), JSUndefined)
            self.assertEqual(ctxt.eval('array[4]'), JSUndefined)
            self.assertEqual(ctxt.eval('array[5]'), JSUndefined)
            self.assertEqual(ctxt.eval('array[6]'), JSUndefined)

            array[0:7] = [0, 1, 2, 3, 4, 5, 6]
            # array         [0, 1, 2, 3, 4, 5, 6]
            self.assertEqual(len(array), 7)
            self.assertEqual(ctxt.eval('array[0]'), 0)
            self.assertEqual(ctxt.eval('array[1]'), 1)
            self.assertEqual(ctxt.eval('array[2]'), 2)
            self.assertEqual(ctxt.eval('array[3]'), 3)
            self.assertEqual(ctxt.eval('array[4]'), 4)
            self.assertEqual(ctxt.eval('array[5]'), 5)
            self.assertEqual(ctxt.eval('array[6]'), 6)

            del array[0:2]
            # array         [None, None, 2, 3, 4, 5, 6]
            self.assertEqual(len(array), 7)
            self.assertEqual(ctxt.eval('array[0]'), JSUndefined)
            self.assertEqual(ctxt.eval('array[1]'), JSUndefined)
            self.assertEqual(ctxt.eval('array[2]'), 2)
            self.assertEqual(ctxt.eval('array[3]'), 3)
            self.assertEqual(ctxt.eval('array[4]'), 4)
            self.assertEqual(ctxt.eval('array[5]'), 5)
            self.assertEqual(ctxt.eval('array[6]'), 6)

            del array[3:7:2]
            # array         [None, None, 2, None, 4, None, 6]
            self.assertEqual(len(array), 7)
            self.assertEqual(ctxt.eval('array[0]'), JSUndefined)
            self.assertEqual(ctxt.eval('array[1]'), JSUndefined)
            self.assertEqual(ctxt.eval('array[2]'), 2)
            self.assertEqual(ctxt.eval('array[3]'), JSUndefined)
            self.assertEqual(ctxt.eval('array[4]'), 4)
            self.assertEqual(ctxt.eval('array[5]'), JSUndefined)
            self.assertEqual(ctxt.eval('array[6]'), 6)

    def testMultiDimArray(self):
        with JSContext() as ctxt:
            ret = ctxt.eval("""
                ({
                    'test': function(){
                        return  [
                            [ 1, 'abla' ],
                            [ 2, 'ajkss' ],
                        ]
                    }
                })
                """).test()

            self.assertEqual([[1, 'abla'], [2, 'ajkss']], convert(ret))

    def testForEach(self):
        class NamedClass(object):
            foo = 1

            def __init__(self):
                self.bar = 2

            @property
            def foobar(self):
                return self.foo + self.bar

        def gen(x):
            for i in range(x):
                yield i

        with JSContext() as ctxt:
            func = ctxt.eval("""(function (k) {
                var result = [];
                for (var prop in k) {
                  result.push(prop);
                }
                return result;
            })""")

            # noinspection PySetFunctionToLiteral
            self.assertTrue(set(["bar", "foo", "foobar"]).issubset(set(func(NamedClass()))))
            self.assertEqual(["0", "1", "2"], list(func([1, 2, 3])))
            self.assertEqual(["0", "1", "2"], list(func((1, 2, 3))))
            self.assertEqual(["1", "2", "3"], list(func({'1': 1, '2': 2, '3': 3})))

            self.assertEqual(["0", "1", "2"], list(func(list(gen(3)))))

    def testDict(self):
        with JSContext() as ctxt:
            obj = ctxt.eval("var r = { 'a' : 1, 'b' : 2 }; r")

            self.assertEqual(1, obj.a)
            self.assertEqual(2, obj.b)

            # TODO: re-enable after we implement __iter__ support
            # self.assertEqual({ 'a' : 1, 'b' : 2 }, dict(obj))

            self.assertEqual({'a': 1,
                              'b': [1, 2, 3],
                              'c': {'str': 'goofy',
                                    'float': 1.234,
                                    'obj': {'name': 'john doe'}},
                              'd': True,
                              'e': JSNull,
                              'f': JSUndefined},
                             convert(ctxt.eval("""var x =
                             { a: 1,
                               b: [1, 2, 3],
                               c: { str: 'goofy',
                                    float: 1.234,
                                    obj: { name: 'john doe' }},
                               d: true,
                               e: null,
                               f: undefined}; x""")))

    def testDate(self):
        with JSContext() as ctxt:
            now1 = ctxt.eval("new Date();")

            self.assertTrue(now1)

            now2 = datetime.datetime.now()

            delta = now2 - now1 if now2 > now1 else now1 - now2

            self.assertTrue(delta < datetime.timedelta(seconds=1))

            _func = ctxt.eval("(function (d) { return d.toString(); })")

            _now = datetime.datetime.now()

            # this test was flaky on my machine, TODO: revisit
            #
            # self.assertTrue(str(func(now)).startswith(now.strftime("%a %b %d %Y %H:%M:%S")))

            ctxt.eval("function identity(x) { return x; }")

            now3 = now2.replace(microsecond=123000)
            self.assertEqual(now3, ctxt.locals.identity(now3))

    def testUnicode(self):
        with JSContext() as ctxt:
            self.assertEqual(u"人", ctxt.eval(u"\"人\""))
            self.assertEqual(u"é", ctxt.eval(u"\"é\""))

            func = ctxt.eval("(function (msg) { return msg.length; })")

            self.assertEqual(2, func(u"测试"))

    def testClassicStyleObject(self):
        class FileSystemWrapper:
            @property
            def cwd(self):
                return os.getcwd()

        class Global:
            @property
            def fs(self):
                return FileSystemWrapper()

        with JSContext(Global()) as ctxt:
            self.assertEqual(os.getcwd(), ctxt.eval("fs.cwd"))

    def testRefCount(self):
        o = object()

        class Global(JSClass):
            po = o

        g = Global()
        g_refs = sys.getrefcount(g)

        count = sys.getrefcount(o)
        with JSContext(g) as ctxt:
            ctxt.eval("""
                var hold = po;
            """)

            self.assertEqual(count + 1, sys.getrefcount(o))

            ctxt.eval("""
                var hold = po;
            """)

            self.assertEqual(count + 1, sys.getrefcount(o))

            del ctxt

        aux.v8_request_gc_for_testing()
        self.assertEqual(g_refs, sys.getrefcount(g))

    def testProperty(self):
        class Global(JSClass):
            def __init__(self, name):
                self._name = name

            def getname(self):
                return self._name

            def setname(self, name):
                self._name = name

            def delname(self):
                self._name = 'deleted'

            name = property(getname, setname, delname)

        g = Global('world')

        with JSContext(g) as ctxt:
            self.assertEqual('world', ctxt.eval("name"))
            self.assertEqual('foobar', ctxt.eval("this.name = 'foobar';"))
            self.assertEqual('foobar', ctxt.eval("name"))
            self.assertTrue(ctxt.eval("delete name"))

            # FIXME replace the global object with Python object
            #
            # self.assertEqual('deleted', ctxt.eval("name"))
            # ctxt.eval("__defineGetter__('name', function() { return 'fixed'; });")
            # self.assertEqual('fixed', ctxt.eval("name"))

    def testGetterAndSetter(self):
        class Global(JSClass):
            def __init__(self, testval):
                self.testval = testval

        with JSContext(Global("Test Value A")) as ctxt:
            self.assertEqual("Test Value A", ctxt.locals.testval)
            ctxt.eval("""
               this.__defineGetter__("test", function() {
                   return this.testval;
               });
               this.__defineSetter__("test", function(val) {
                   this.testval = val;
               });
           """)
            self.assertEqual("Test Value A", ctxt.locals.test)

            ctxt.eval("test = 'Test Value B';")

            self.assertEqual("Test Value B", ctxt.locals.test)

    def testReferenceCount(self):
        class Hello(object):
            def say(self):
                pass

            # def __del__(self):
            #     owner.deleted = True

        def test():
            with JSContext() as ctxt:
                fn = ctxt.eval("(function (obj) { obj.say(); })")

                obj = Hello()

                self.assertEqual(2, sys.getrefcount(obj))

                fn(obj)

                self.assertEqual(4, sys.getrefcount(obj))

                del obj

        test()

    def testNullInString(self):
        with JSContext() as ctxt:
            fn = ctxt.eval("(function (s) { return s; })")

            self.assertEqual("hello \0 world", fn("hello \0 world"))

    def testLivingObjectCache(self):
        class Global(JSClass):
            i = 1
            b = True
            o = object()

        with JSContext(Global()) as ctxt:
            self.assertTrue(ctxt.eval("i == i"))
            self.assertTrue(ctxt.eval("b == b"))
            self.assertTrue(ctxt.eval("o == o"))

    def testNamedSetter(self):
        class Obj(JSClass):
            def __init__(self):
                self._p = None

            @property
            def p(self):
                return self._p

            @p.setter
            def p(self, value):
                self._p = value

        class Global(JSClass):
            def __init__(self):
                self.obj = Obj()
                self.d = {}
                self.p = None

        with JSContext(Global()) as ctxt:
            ctxt.eval("""
            x = obj;
            x.y = 10;
            x.p = 10;
            d.y = 10;
            """)
            self.assertEqual(10, ctxt.eval("obj.y"))
            self.assertEqual(10, ctxt.eval("obj.p"))
            self.assertEqual(10, ctxt.locals.d['y'])

    def testWatch(self):
        class Obj(JSClass):
            def __init__(self):
                self.p = 1

        class Global(JSClass):
            def __init__(self):
                self.o = Obj()

        with JSContext(Global()) as ctxt:
            ctxt.eval("""
            o.watch("p", function (id, oldval, newval) {
                return oldval + newval;
            });
            """)

            self.assertEqual(1, ctxt.eval("o.p"))

            ctxt.eval("o.p = 2;")

            self.assertEqual(3, ctxt.eval("o.p"))

            ctxt.eval("delete o.p;")

            self.assertEqual(JSUndefined, ctxt.eval("o.p"))

            ctxt.eval("o.p = 2;")

            self.assertEqual(2, ctxt.eval("o.p"))

            ctxt.eval("o.unwatch('p');")

            ctxt.eval("o.p = 1;")

            self.assertEqual(1, ctxt.eval("o.p"))

    def testReferenceError(self):
        class Global(JSClass):
            def __init__(self):
                self.s = self

        with JSContext(Global()) as ctxt:
            self.assertRaises(ReferenceError, ctxt.eval, 'x')

            self.assertTrue(ctxt.eval("typeof(x) === 'undefined'"))

            self.assertTrue(ctxt.eval("typeof(String) === 'function'"))

            self.assertTrue(ctxt.eval("typeof(s.String) === 'undefined'"))

            self.assertTrue(ctxt.eval("typeof(s.z) === 'undefined'"))

    def testRaiseExceptionInGetter(self):
        class Document(JSClass):
            def __getattr__(self, name):
                if name == 'y':
                    raise TypeError()

                return JSClass.__getattr__(self, name)

        class Global(JSClass):
            def __init__(self):
                self.document = Document()

        with JSContext(Global()) as ctxt:
            self.assertEqual(JSUndefined, ctxt.eval('document.x'))
            self.assertRaises(TypeError, ctxt.eval, 'document.y')

    def testNullAndUndefined(self):
        # noinspection PyPep8Naming,PyMethodMayBeStatic
        class Global(JSClass):
            def returnUndefined(self):
                return JSUndefined

            def returnNull(self):
                return JSNull

            def returnNone(self):
                return None

        with JSContext(Global()) as ctxt:
            # JSNull maps to None
            # JSUndefined maps to Py_JSUndefined (None-like)
            self.assertEqual(JSNull, None)

            self.assertEqual(JSNull, ctxt.eval("null"))
            self.assertEqual(JSUndefined, ctxt.eval("undefined"))

            self.assertFalse(bool(JSUndefined))
            self.assertFalse(bool(JSNull))

            self.assertEqual("JSUndefined", str(JSUndefined))
            self.assertEqual("None", str(JSNull))

            self.assertTrue(ctxt.eval('undefined == returnUndefined()'))
            self.assertTrue(ctxt.eval('null == returnNone()'))
            self.assertTrue(ctxt.eval('null == returnNull()'))


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level=level, format='%(asctime)s %(levelname)s %(message)s')
    unittest.main()
