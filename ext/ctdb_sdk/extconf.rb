require 'mkmf'

def osx?
  !!(RUBY_PLATFORM =~ /darwin/)
end

def linux?
  !!(RUBY_PLATFORM =~ /linux/)
end

def fail(msg)
  msg << <<-EOS
  
Try using the following options:

  --with-faicom-dir=/path/to/faircom
EOS
  printf("extconf.rb failure: %s\n", msg)
  exit
end

if osx?
  api_dir = "ctree.ctdb/multithreaded/static"
  lib     = "mtclient"
elsif linux?
  api_dir = "ctree.ctdb/singlethreaded/static"
  lib     = "ctclient"
end

base_dir = with_config('faircom-dir')
inc_dirs = [ "#{base_dir}/include", "#{base_dir}/include/sdk/#{api_dir}" ]
# cflags   = "-g -m64"
lib_dir  = "#{base_dir}/lib/#{api_dir}"

inc_dirs.each do |path|
  $INCFLAGS << " -I#{path}"
end
$LIBPATH << "#{lib_dir}"

errors = []
errors << "'ctdbsdk'" unless have_header('ctdbsdk.hpp')
errors << "'#{lib}'"  unless have_library(lib)
fail("missing dependencies: #{errors.join(',')}") unless errors.empty?

create_makefile("ctree_sdk")