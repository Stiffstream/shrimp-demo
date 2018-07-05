require 'mxx_ru/cpp'

require 'restinio/asio_helper.rb'
require 'shrimp/magickpp_helper.rb'

MxxRu::Cpp::exe_target {

	required_prj 'shrimp/prj.rb'

	target( "_unit.test.cache_alike_container" )

	cpp_source( "catch_main.cpp" )
	cpp_source( "main.cpp" )
}

