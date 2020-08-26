import sys
import subprocess
import os
import os.path
from setuptools import setup, Extension


def build_judy():
	print('INFO: building judy static library...')

	CC = os.environ.get('CC', 'cc')

	# adding last two flags because of compiler and/or code bugs
	# see http://sourceforge.net/p/judy/mailman/message/32417284/
	assert(sys.maxsize == 2**63 - 1)

	CFLAGS = '-DJU_64BIT -O0 -fPIC -fno-strict-aliasing -fno-aggressive-loop-optimizations'
	exitcode, output = subprocess.getstatusoutput(f'(cd judy-1.0.5/src; CC=\'{CC}\' COPT=\'{CFLAGS}\' sh ./sh_build)')

	if exitcode != 0:
		sys.exit(output)

	print(output)


if not os.path.isfile('./judy-1.0.5/src/libJudy.a'):
	build_judy()

extra_compile_args = [
	'-I./include',
	'-I./judy-1.0.5/src',
	'-Wall',
	'-Wno-strict-prototypes',
	'-g',
	'-D_GNU_SOURCE',
	'-O2',
	'-DNDEBUG',
	'-std=c99'
]

extra_link_args = ['-L./judy-1.0.5/src', '-Bstatic', '-lJudy', '-Bdynamic', '-lm']

setup(
	name='pointless',
	version='1.0.5',
	maintainer='Arni Mar Jonsson',
	maintainer_email='arnimarj@gmail.com',
	url='https://github.com/arnimarj/py-pointless',

	classifiers=[
		'Development Status :: 5 - Production/Stable',
		'Environment :: Other Environment',
		'Intended Audience :: Developers',
		'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
		'Operating System :: POSIX',
		'Programming Language :: C',
		'Programming Language :: Python',
		'Programming Language :: Python :: 3',
		'Programming Language :: Python :: 3.5',
		'Programming Language :: Python :: 3.6',
		'Programming Language :: Python :: 3.7',
		'Programming Language :: Python :: 3.8',
		'Topic :: Database',
		'Topic :: Software Development :: Libraries'
	],

	description='A read-only relocatable data structure for JSON like data, with C and Python APIs',

	packages=['pointless'],
	package_dir={'pointless': ''},

	ext_modules=[
		Extension(
			'pointless',
			include_dirs=[
				'./include',
			],
			sources=[
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
				'src/pointless_cycle_marker_wrappers.c',
				'src/pointless_validate.c',
				'src/pointless_validate_heap_ref.c',
				'src/pointless_validate_heap.c',
				'src/pointless_validate_hash_table.c',
				'src/pointless_malloc.c',
				'src/pointless_int_ops.c',
				'src/pointless_recreate.c',
				'src/pointless_eval.c'
			],

			extra_compile_args=extra_compile_args,
			extra_link_args=extra_link_args
		)
	]
)
