MxxRu::arch_externals :so5 do |e|
  e.url 'https://sourceforge.net/projects/sobjectizer/files/sobjectizer/SObjectizer%20Core%20v.5.5/so-5.5.22.1.zip'
  e.map_dir 'dev/so_5' => 'dev'
  e.map_dir 'dev/timertt' => 'dev'
end

MxxRu::arch_externals :restinio do |e|
  e.url 'https://bitbucket.org/sobjectizerteam/restinio-0.4/downloads/restinio-0.4.6-full.tar.bz2'

  # RESTinio and its dependencies.
  e.map_dir 'dev/asio' => 'dev'
  e.map_dir 'dev/asio_mxxru' => 'dev'
  # Use fmt of version 4.1.0 as spd log doesn't support 5.0.0 for now.
  # e.map_dir 'dev/fmt' => 'dev'
  # e.map_dir 'dev/fmt_mxxru' => 'dev'
  e.map_dir 'dev/nodejs' => 'dev'
  e.map_dir 'dev/restinio' => 'dev'
end

MxxRu::arch_externals :fmt do |e|
  e.url 'https://github.com/fmtlib/fmt/archive/4.1.0.zip'

  e.map_dir 'fmt' => 'dev/fmt'
  e.map_dir 'support' => 'dev/fmt'
  e.map_file 'CMakeLists.txt' => 'dev/fmt/*'
  e.map_file 'README.rst' => 'dev/fmt/*'
  e.map_file 'ChangeLog.rst' => 'dev/fmt/*'
end

MxxRu::arch_externals :fmtlib_mxxru do |e|
  e.url 'https://bitbucket.org/sobjectizerteam/fmtlib_mxxru/get/fmt-4.1.0.tar.bz2'

  e.map_dir 'dev/fmt_mxxru' => 'dev'
end

MxxRu::arch_externals :clara do |e|
  e.url 'https://github.com/catchorg/Clara/archive/v1.1.4.tar.gz'

  e.map_file 'single_include/clara.hpp' => 'dev/clara/*'
end

MxxRu::arch_externals :catch do |e|
  e.url 'https://github.com/catchorg/Catch2/archive/v2.1.0.zip'

  e.map_file 'single_include/catch.hpp' => 'dev/catch/*'
end
