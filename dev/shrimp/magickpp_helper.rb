require 'mxx_ru/cpp'

module ShrimpMagickppHelper

  @@extra_compile_flags = nil
  @@libs = nil
  @@lib_dirs = nil

  # Runs `Magick++-config` to get compile flags and libs.
  def self.run_config_if_necessary
    if not @@extra_compile_flags
      IO.popen( "Magick++-config --cppflags", :err => [:child, :out] ) do |io|
        @@extra_compile_flags = io.readlines.join.strip
      end
    end

    if not @@libs
      output = ""
      IO.popen( "Magick++-config --libs", :err => [:child, :out] ) do |io|
        output = io.readlines.join.strip
      end
      @@lib_dirs = []
      output.split.select{|p| p[0..1] == "-L" }.each{|d| @@lib_dirs << d[2..-1] }
      @@libs = []
      output.split.select{|p| p[0..1] == "-l" }.each{|l| @@libs << l[2..-1] }
    end
  end

  # Attach project to imagemagick++.
  def self.attach_imagemagickpp( target_prj )
    run_config_if_necessary
    @@libs.each{|lib| target_prj.lib( lib ) }
    @@lib_dirs.each{|lib| target_prj.lib_path( lib ) }
    target_prj.compiler_option( @@extra_compile_flags )
  end

end # module ShrimpMagickppHelper
