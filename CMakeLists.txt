cmake_minimum_required (VERSION 3.20)

project(AudioMan
	VERSION 0.1
)

# Library source
add_subdirectory(src)

# Demo apps
option(AUDIOMAN_BUILD_DEMO_APPS
	"Build demo applications for AudioMan"
	ON
)

if (AUDIOMAN_BUILD_DEMO_APPS)
	add_subdirectory(demo)
endif()
