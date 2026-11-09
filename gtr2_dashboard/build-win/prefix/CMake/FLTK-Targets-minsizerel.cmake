#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "fltk_z" for configuration "MinSizeRel"
set_property(TARGET fltk_z APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(fltk_z PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/libfltk_z.a"
  )

list(APPEND _cmake_import_check_targets fltk_z )
list(APPEND _cmake_import_check_files_for_fltk_z "${_IMPORT_PREFIX}/lib/libfltk_z.a" )

# Import target "fltk_jpeg" for configuration "MinSizeRel"
set_property(TARGET fltk_jpeg APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(fltk_jpeg PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/libfltk_jpeg.a"
  )

list(APPEND _cmake_import_check_targets fltk_jpeg )
list(APPEND _cmake_import_check_files_for_fltk_jpeg "${_IMPORT_PREFIX}/lib/libfltk_jpeg.a" )

# Import target "fltk_png" for configuration "MinSizeRel"
set_property(TARGET fltk_png APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(fltk_png PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/libfltk_png.a"
  )

list(APPEND _cmake_import_check_targets fltk_png )
list(APPEND _cmake_import_check_files_for_fltk_png "${_IMPORT_PREFIX}/lib/libfltk_png.a" )

# Import target "fltk" for configuration "MinSizeRel"
set_property(TARGET fltk APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(fltk PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C;CXX"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/libfltk.a"
  )

list(APPEND _cmake_import_check_targets fltk )
list(APPEND _cmake_import_check_files_for_fltk "${_IMPORT_PREFIX}/lib/libfltk.a" )

# Import target "fltk_forms" for configuration "MinSizeRel"
set_property(TARGET fltk_forms APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(fltk_forms PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "CXX"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/libfltk_forms.a"
  )

list(APPEND _cmake_import_check_targets fltk_forms )
list(APPEND _cmake_import_check_files_for_fltk_forms "${_IMPORT_PREFIX}/lib/libfltk_forms.a" )

# Import target "fltk_images" for configuration "MinSizeRel"
set_property(TARGET fltk_images APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(fltk_images PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "CXX"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/libfltk_images.a"
  )

list(APPEND _cmake_import_check_targets fltk_images )
list(APPEND _cmake_import_check_files_for_fltk_images "${_IMPORT_PREFIX}/lib/libfltk_images.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
