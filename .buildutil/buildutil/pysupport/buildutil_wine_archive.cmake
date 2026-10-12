# CMAKE_USER_MAKE_RULES_OVERRIDE_<LANG> of the wine-msvc lane: CMake never runs an archive through the
# linker launcher, and Windows-MSVC.cmake sets the archive rule unconditionally, so it is prefixed here.
foreach(_buildutil_lang IN ITEMS C CXX)
  if(DEFINED CMAKE_${_buildutil_lang}_CREATE_STATIC_LIBRARY
     AND CMAKE_${_buildutil_lang}_LINKER_LAUNCHER
     AND NOT _buildutil_archive_launched_${_buildutil_lang})
    set(_buildutil_archive_launched_${_buildutil_lang} ON)
    list(TRANSFORM CMAKE_${_buildutil_lang}_LINKER_LAUNCHER PREPEND "\"" OUTPUT_VARIABLE _buildutil_words)
    list(TRANSFORM _buildutil_words APPEND "\"")
    list(JOIN _buildutil_words " " _buildutil_launcher)
    string(PREPEND CMAKE_${_buildutil_lang}_CREATE_STATIC_LIBRARY "${_buildutil_launcher} ")
  endif()
endforeach()
