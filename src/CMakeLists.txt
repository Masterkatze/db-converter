FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

set (CMAKE_CXX_STANDARD 17)

add_library(db_tools SHARED
	"db_tools.cxx"
	"db_tools.hxx"
	"crc32/crc32.cxx"
	"crc32/crc32.hxx"
	"lzo/lzoconf.h"
	"lzo/lzodefs.h"
	"lzo/minilzo.c"
	"lzo/minilzo.h"
	"xray_re/xr_types.hxx"
	"xray_re/xr_scrambler.cxx"
	"xray_re/xr_scrambler.hxx"
	"xray_re/xr_lzhuf.cxx"
	"xray_re/xr_lzhuf.hxx"
	"xray_re/xr_file_system.cxx"
	"xray_re/xr_file_system.hxx"
	"xray_re/xr_reader.cxx"
	"xray_re/xr_reader.hxx"
	"xray_re/xr_reader_scrambler.cxx"
	"xray_re/xr_utils.hxx"
	"xray_re/xr_writer.cxx"
	"xray_re/xr_writer.hxx"
	"xray_re/xr_packet.cxx"
	"xray_re/xr_packet.hxx"
	"xray_re/xr_limits.hxx"
	"xray_re/xr_log.cxx"
	"xray_re/xr_log.hxx")

add_executable(${PROJECT_NAME}
	"main.cxx")

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${Boost_LIBRARIES} db_tools)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
