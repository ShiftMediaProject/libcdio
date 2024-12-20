# This PowerShell script creates 'version.h' if not present
if (!(Test-Path ..\include\cdio\version.h)) {
  $VERSION = Select-String -Path ..\configure.ac "^define\(RELEASE_NUM,\s*(.*)\)" | Foreach-Object {$_.Matches.Groups[1].Value}
  $LIBCDIO_VERSION_NUM = Select-String -Path ..\configure.ac "^LIBCDIO_VERSION_NUM=(.*)" | Foreach-Object {$_.Matches.Groups[1].Value}
  (Get-Content ..\include\cdio\version.h.in) -replace "@LIBCDIO_VERSION_NUM@", "$LIBCDIO_VERSION_NUM" -replace "@VERSION@", "$VERSION" -replace "@build@", "Windows" | Out-File -FilePath ..\include\cdio\version.h -NoClobber
}
