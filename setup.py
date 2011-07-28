import sys, commands
from distutils.core import setup, Extension

print 'INFO: building judy static library...'

if sys.maxint == 2**63-1:
	CFLAGS = '-O3 -fPIC -DJU_64BIT'
elif sys.maxint == 2**31-1:
	CFLAGS = '-O3 -fPIC'
else:
	sys.exit('bad sys.maxint')

a, b = commands.getstatusoutput('(cd judy/src; COPT=\'%s\' ./sh_build)' % (CFLAGS,))

if a != 0:
	sys.exit(b)

extra_compile_args = [
	'-I./judy/src',
	'-I./include',
	'-I/usr/local/include',
	'-pedantic',
	'-std=c99',
	'-Wall',
	'-Wno-strict-prototypes',
	'-g',
	'-D_GNU_SOURCE',
	'-O2',
	'-DNDEBUG',
#	'-fno-omit-frame-pointer',
#	'-pg',
#	'-fno-inline-functions',
#	'-fno-inline-functions-called-once',
#	'-fno-optimize-sibling-calls',
#	'-fno-inline'
]

extra_link_args = ['-L./judy/src', '-Bstatic', '-lJudy', '-Bdynamic', '-lm']

setup(
	name = 'pointless',
	version = '0.2',
	maintainer = 'Arni Mar Jonsson',
	maintainer_email = 'arnimarj@gmail.com',
	url = 'http://code.google.com/p/py-pointless/',

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

	download_url = 'http://py-pointless.googlecode.com/files/pointless-0.2.tar.gz',
	description = 'A read-only relocatable data structure for JSON like data, with C and Python APIs',
	# long_description = 

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
				'python/pointless_db_utils.c',

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
			],

			extra_compile_args = extra_compile_args,
			extra_link_args = extra_link_args
		)
	]
)
