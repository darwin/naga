# Naga

Naga allows interop between Python3 and JavaScript running Google's V8 engine. You can use Naga to embed JavaScript 
code directly into your Python project, or to call Python code from JavaScript.

Naga is a fork of [STPyV8](https://github.com/area1/stpyv8) which is a fork of the original [PyV8](https://code.google.com/archive/p/pyv8/) project, 
with code changed to work with the latest V8 engine and Python3. Naga links with Google's V8 built as a static library. 

Currently the library is a work in progress.

# Usage Examples

Wrapping a JavaScript function in a Python function:

```Python
# simple.py
from naga import JSContext

with JSContext() as ctxt:
  upcase = ctxt.eval("""
    ( (lowerString) => {
        return lowerString.toUpperCase();
    })
  """)
  print(upcase("hello world!"))
```

```
$ python simple.py
HELLO WORLD!
```

## Using Python in V8

Naga allows you to use Python functions, classes, and objects from within V8.

Exporting a Python class into V8 and using it from JavaScript:

```Python
# meaning.py
from naga import JSClass, JSContext

class MyClass(JSClass):
  def reallyComplexFunction(self, addme):
    return 10 * 3 + addme

my_class = MyClass()

with JSContext(my_class) as ctxt:
  meaning = ctxt.eval("this.reallyComplexFunction(2) + 10;")
  print("The meaning of life: " + str(meaning))
```

```
$ python meaning.py
The meaning of life: 42
```

## Using JavaScript in Python

Naga allows you to use JavaScript functions, classes, object from Python.

Calling methods on a JavaScript class from Python code:

```Python
# circle.py
from naga import JSContext

with JSContext() as ctxt:
  ctxt.eval("""
    class Circle {
      constructor(radius) {
        this.radius = radius;
      }
      get area() {
        return this.calcArea()
      }
      calcArea() {
        return 3.14 * this.radius * this.radius;
      }
  }
  """)
  circle = ctxt.eval("new Circle(10)")
  print("Area of the circle: " + str(circle.area))
```

```
$ python cicle.py
Area of the circle: 314
```

Find more in the [tests](tests) directory.

# Building

Look into [gn/README.md](gn/README.md).

## FAQ

### How does this work?

> Naga is a Python [C++ Extension Module](https://docs.python.org/3/c-api/index.html) that links to an [embedded V8](https://v8.dev/docs/embed) 
library.

### Is Naga fast?

> Depends who is asking. Naga needs to translate Python arguments (and JavaScript arguments) back and forth between 
function and method calls in the two languages. It tries to do the minimum amount of work using native code, but if you are 
interested in the height of performance, make your interface between Python and JavaScript "chunky" ... i.e., make the 
minimum number of transitions between the two.

### What can I use this for?

* [honeynet.org](https://honeynet.org) use this to simulate a [browser](https://github.com/buffer/thug), and then execute sketchy JavaScript in an 
instrumented container. Other kinds of JavaScript sand-boxing (simulating and monitoring the external world to 
JavaScript code) are a natural fit.

* [darwin](https://github.com/darwin) uses it to provide Javascript interface for Blender scripting. 
Actually he is more interested in using [ClojureScript](https://github.com/darwin/blender-clojure).
