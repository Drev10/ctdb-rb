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

ct_dir = with_config('faircom-dir')
api_dir  = %w(ctree.ctdb multithreaded dynamic).join(File::SEPARATOR)
lib      = "mtclient"
lib_dir  = File.join(ct_dir, 'lib', api_dir)
inc_dirs = [ File.join(ct_dir, 'include', 'sdk', api_dir),
             File.join(ct_dir, 'include') ]

inc_dirs.each do |path|
  $INCFLAGS << " -I#{path}"
end
$LIBPATH  << "#{lib_dir}"

errors = []
errors << "'ctdbsdk.h'" unless have_header('ctdbsdk.h')
errors << "'#{lib}'"  unless have_library(lib)
fail("missing dependencies: #{errors.join(',')}") unless errors.empty?

create_makefile("ctdb_sdk")