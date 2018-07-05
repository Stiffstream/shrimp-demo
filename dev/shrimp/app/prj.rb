require 'mxx_ru/cpp'

require 'restinio/asio_helper.rb'
require 'shrimp/magickpp_helper.rb'

MxxRu::Cpp::exe_target {

  target 'shrimp.app'

  RestinioAsioHelper.attach_propper_asio( self )
  required_prj 'fmt_mxxru/prj.rb'
  required_prj 'restinio/platform_specific_libs.rb'
  required_prj 'so_5/prj_s.rb'

  ShrimpMagickppHelper.attach_imagemagickpp( self )
  required_prj 'shrimp/prj.rb'

  lib( "stdc++fs" )

  cpp_source 'main.cpp'
}

