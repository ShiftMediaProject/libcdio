require 'mkmf'
dir_config('rubycdio')
$libs = append_library($libs, "cdio")
create_makefile('rubycdio')
