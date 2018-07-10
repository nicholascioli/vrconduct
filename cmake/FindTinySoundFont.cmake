# Additional modules
include(FindPackageHandleStandardArgs)

find_path(TSF_INCLUDE_DIR
	NAMES TinySoundFont/tml.h TinySoundFont/tsf.h
	PATHS ${CMAKE_SOURCE_DIR}/lib/
	DOC "The directory where TinySoundFont headers reside"
)

# Handle REQUIRD argument, define *_FOUND variable
find_package_handle_standard_args(TinySoundFont DEFAULT_MSG TSF_INCLUDE_DIR)

# Hide some variables
mark_as_advanced(TSF_INCLUDE_DIR)
