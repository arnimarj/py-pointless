import sys, commands, os
from setuptools import setup, Extension

def build_judy():
	print('INFO: building judy static library...')

	CC = os.environ.get('CC', 'cc')

	is_clang = False

	# test if CC is clang
	exitcode, output = commands.getstatusoutput('%s --version' % (CC,))

	if exitcode != 0:
		sys.exit(output)

	if 'clang' in output:
		is_clang = True

	# adding last two flags because of compiler and/or code bugs
	# see http://sourceforge.net/p/judy/mailman/message/32417284/
	assert(sys.maxint in (2**63-1, 2**31-1))

	if is_clang:
		if sys.maxint == 2**63-1:
			CFLAGS = '-DJU_64BIT -O0 -fPIC -fno-strict-aliasing'
		else:
			CFLAGS = '           -O0 -fPIC -fno-strict-aliasing'
	else:
		if sys.maxint == 2**63-1:
			CFLAGS = '-DJU_64BIT -O0 -fPIC -fno-strict-aliasing -fno-aggressive-loop-optimizations'
		else:
			CFLAGS = '           -O0 -fPIC -fno-strict-aliasing -fno-aggressive-loop-optimizations'

	exitcode, output = commands.getstatusoutput('(cd judy-1.0.5/src; CC=\'%s\' COPT=\'%s\' sh ./sh_build)' % (CC, CFLAGS))

	if exitcode != 0:
		sys.exit(output)

	print output

build_judy()

extra_compile_args = [
	'-I./include',
	'-I./judy-1.0.5/src',
	'-Wall',
	'-Wno-strict-prototypes',
	'-g',
	'-D_GNU_SOURCE',
	'-O2',
	'-DNDEBUG'
]

extra_link_args = ['-L./judy-1.0.5/src', '-Bstatic', '-lJudy', '-Bdynamic', '-lm']

setup(
	name = 'pointless',
	version = '0.2.7',
	maintainer = 'Arni Mar Jonsson',
	maintainer_email = 'arnimarj@gmail.com',
	url = 'https://github.com/arnimarj/py-pointless',

	classifiers = [
		'Development Status :: 5 - Production/Stable',
		'Environment :: Other Environment',
		'Intended Audience :: Developers',
		'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
		'Operating System :: POSIX',
		'Programming Language :: C',
		'Programming Language :: Python',
		'Programming Language :: Python :: 2.6',
		'Programming Language :: Python :: 2.7',
		'Topic :: Database',
		'Topic :: Software Development :: Libraries'
	],

	download_url = 'https://github.com/arnimarj/py-pointless/archive/v0.2.7.tar.gz',
	description = 'A read-only relocatable data structure for JSON like data, with C and Python APIs',

	packages = ['pointless'],
	package_dir = {'pointless': ''},

	ext_modules = [
		Extension('pointless',
			sources = [
				# python stuff
				'pointless_ext.c',

				# pypointless
				'python/pointless_create.c',
				'python/pointless_vector.c',
				'python/pointless_bitvector.c',
				'python/pointless_set.c',
				'python/pointless_map.c',
				'python/pointless_object.c',
				'python/pointless_instance_dispatch.c',
				'python/pointless_pyobject_hash.c',
				'python/pointless_pyobject_cmp.c',
				'python/pointless_print.c',
				'python/pointless_prim_vector.c',

				# libpointless
				'src/custom_sort.c',
				'src/bitutils.c',
				'src/pointless_reader.c',
				'src/pointless_reader_helpers.c',
				'src/pointless_create.c',
				'src/pointless_create_cache.c',
				'src/pointless_dynarray.c',
				'src/pointless_value.c',
				'src/pointless_unicode_utils.c',
				'src/pointless_hash.c',
				'src/pointless_cmp.c',
				'src/pointless_hash_table.c',
				'src/pointless_bitvector.c',
				'src/pointless_walk.c',
				'src/pointless_cycle_marker.c',
				'src/pointless_validate.c',
				'src/pointless_validate_heap_ref.c',
				'src/pointless_validate_heap.c',
				'src/pointless_validate_hash_table.c',
				'src/pointless_malloc.c',
				'src/pointless_int_ops.c',
				'src/pointless_recreate.c',
				'src/pointless_eval.c'
			],

			extra_compile_args = extra_compile_args,
			extra_link_args = extra_link_args
		)
	]
)
