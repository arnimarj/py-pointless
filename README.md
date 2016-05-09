[![Build Status](https://travis-ci.org/dohopmas/py-pointless.svg)](https://travis-ci.org/dohopmas/py-pointless)

# py-pointless

A fast and efficient read-only relocatable data structure for JSON like data, with C and Python APIs.

## Installing / Getting started

To start using the extension from within Python, simply install this module via pip:

```shell
> pip install pointless
```

### Using pointless from within another C extension

The pointless module exposes a C API for use in other Python extensions written in C. For this scenario, the Pointless Python module conveniently installs required header files along with the module.

If you installed this module using `pip` in a virtual environment then you can find it's C header files in:

```shell
<path-to-venv>/include/site/pythonX.Y/
```

X.Y refers to the python version you are using, f.e. for a python 2.7 virtual environment called 'foo' installed in a user's home directory the location would be:

```shell
~/foo/include/site/python2.7/
```

## Developing

```shell
> sudo apt-get install libjudy-dev
> git clone https://github.com/dohopmas/py-pointless.git
> cd py-pointless/
```

To get started with py-pointless development, you'll need to be able to statically link against [the Judy library](http://judy.sourceforge.net/). If you happen to be on Ubuntu 12.04 or later, this dependency can be satisfied by simply installing the `libjudy-dev` package.

After that you'll simply need to clone this repository and you're good to go!

### Building

To build the module you can simply [use setuptools](http://pythonhosted.org/setuptools/index.html):

```shell
> python setup.py build_ext
```

Or, better yet, just run the tests as well:

```shell
> python setup.py test
```
