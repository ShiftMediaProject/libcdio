#!/usr/bin/env ruby
# Alternative configuration for Visual C written in Ruby.


ABS_TOP_SRCDIR = File.dirname(__FILE__)

# Text substitutions performed on files
SUBS = {

  # Put in what you want for the build string.
  :build => ARGV[0] || 'pc-windows-visualstudio',

  :abs_top_srcdir => ABS_TOP_SRCDIR.dup,
  :abs_top_builddir => ABS_TOP_SRCDIR.dup,
  :native_abs_top_srcdir => File.expand_path(ABS_TOP_SRCDIR)
}

# Get version number from configure.ac
def get_version_num
  config_file = File.dirname(__FILE__) + '/configure.ac'
  lines = File.readlines(config_file)
  relnum_regexp = Regexp.new('^define\(RELEASE_NUM, (\d+)')
  relnum_line = lines.grep(relnum_regexp)
  if relnum_line.size != 1
    STDERR.puts("Can't find define(RELEASE_NUM, ...) in #{config_file}")
    exit 1
  end
  if relnum_line[0] !~ relnum_regexp
    STDERR.puts("Something went wrong in matching release number in #{config_file}")
    exit 2
  end
  SUBS[:LIBCDIO_VERSION_NUM] = $1
  SUBS[:VERSION] = "0.#{$1}"
end

# Write #{filename} from #{filename}.in and SUBS
def perform_substitutions(filename)

  builddir = SUBS[:abs_top_builddir]
  version_file = builddir + '/' + filename
  version_file_template = version_file + '.in'

  text = File.read(version_file_template)
  SUBS.each do |key, val|
    from_str = "@#{key.to_s}@"
    text.gsub!(from_str, val)
  end
  File.open(version_file, 'w') { |f| f.write(text) }
end

get_version_num
%w(include/cdio/version.h
   test/testgetdevices.c test/testisocd2.c   		test/testisocd_joliet.c
   test/driver/bincue.c  test/driver/track.c		test/test/testisocd_joliet.c
   test/driver/cdrdao.c  test/test/testgetdevices.c	test/test/testpregap.c
   test/driver/nrg.c	 test/test/testisocd2.c
   test/testpregap.c
).each do |file|
  perform_substitutions(file)
end
