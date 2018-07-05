require 'mxx_ru/cpp'

require 'restinio/asio_helper.rb'
require 'shrimp/magickpp_helper.rb'

MxxRu::Cpp::lib_target {

	RestinioAsioHelper.attach_propper_asio( self )
	required_prj 'nodejs/http_parser_mxxru/prj.rb'
	required_prj 'fmt_mxxru/prj.rb'

	required_prj 'restinio/platform_specific_libs.rb'
	required_prj 'restinio/pcre_libs.rb'
	required_prj 'so_5/prj_s.rb'
	ShrimpMagickppHelper.attach_imagemagickpp( self )

	# Define your target name here.
	target 'lib/shrimp'

	cpp_source 'transforms.cpp'
	cpp_source 'response_common.cpp'
	cpp_source 'http_server.cpp'
	cpp_source 'a_transform_manager.cpp'
	cpp_source 'a_transformer.cpp'
}

