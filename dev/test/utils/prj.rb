require 'mxx_ru/cpp'

require 'shrimp/magickpp_helper.rb'

MxxRu::Cpp::exe_target {

	required_prj 'shrimp/prj.rb'
	ShrimpMagickppHelper.attach_imagemagickpp( self )

	target( "_unit.test.utils" )

	cpp_source( "catch_main.cpp" )
	cpp_source( "main.cpp" )
}

