#!/usr/bin/ruby
require 'mxx_ru/cpp'
require 'restinio/boost_helper.rb'

MxxRu::Cpp::composite_target( MxxRu::BUILD_ROOT ) {

	toolset.force_cpp17
	global_include_path "."

	if not $sanitizer_build
		if 'gcc' == toolset.name || 'clang' == toolset.name
			global_linker_option '-pthread'
			global_linker_option '-static-libstdc++'
			global_linker_option "-Wl,-rpath='$ORIGIN'"
		end

		# If there is local options file then use it.
		if FileTest.exist?( "local-build.rb" )
			required_prj "local-build.rb"
		else
			default_runtime_mode( MxxRu::Cpp::RUNTIME_RELEASE )
			MxxRu::enable_show_brief

			global_obj_placement MxxRu::Cpp::RuntimeSubdirObjPlacement.new( 'target' )
		end
	end

	if "mswin" == toolset.tag( "target_os" ) && 'vc' == toolset.name && "" != RestinioBoostHelper.detect_boost_root
		RestinioBoostHelper.add_boost_root_path_msvc( self )
	end

	required_prj 'test/build_tests.rb'
	required_prj 'shrimp/app/prj.rb'
}

