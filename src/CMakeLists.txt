FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

set (CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME}
	"main.cxx"

	"lzo/lzoconf.h"
	"lzo/lzodefs.h"
	"lzo/minilzo.c"
	"lzo/minilzo.h"

	"crc32/crc32.cxx"
	"crc32/crc32.hxx"

	"db_tools.cxx"
	"db_tools.hxx"

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
	"xray_re/xr_vector2.hxx"
	"xray_re/xr_vector3.cxx"
	"xray_re/xr_vector3.hxx"
	"xray_re/xr_vector4.hxx"
	"xray_re/xr_math.hxx"
	"xray_re/xr_color.hxx"
	"xray_re/xr_utils.hxx"
	"xray_re/xr_writer.cxx"
	"xray_re/xr_writer.hxx"
	"xray_re/xr_packet.cxx"
	"xray_re/xr_packet.hxx"
	"xray_re/xr_matrix.cxx"
	"xray_re/xr_matrix.hxx"
	"xray_re/xr_quaternion.cxx"
	"xray_re/xr_quaternion.hxx"
	"xray_re/xr_string_utils.hxx"
	"xray_re/xr_limits.hxx"
	"xray_re/xr_log.cxx"
	"xray_re/xr_log.hxx")

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${Boost_LIBRARIES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
