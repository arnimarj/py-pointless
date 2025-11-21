from setuptools import Extension, setup


setup(
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

			extra_compile_args=[
				'-I./include',
				'-I./judy-1.0.5/src',
				'-Wall',
				'-Wno-strict-prototypes',
				'-g',
				'-D_GNU_SOURCE',
				'-O2',
				'-DNDEBUG',
				'-std=c99',
				# we get warnings on "state->error = "hash statement fallthrough";"
				'-Wno-incompatible-pointer-types',
			],

			extra_link_args=[
				'-L./judy-1.0.5/src',
				'-Bstatic',
				'-lJudy',
				'-Bdynamic',
				'-lm',
			],
		),
	],
)
