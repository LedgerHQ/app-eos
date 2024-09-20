cmake_minimum_required(VERSION 3.10)

project(eos_fuzzer C)

set(CMAKE_C_STANDARD 11)

set (SDK_PATH $ENV{BOLOS_SDK})

include_directories(. ../src)
include_directories(
	${SDK_PATH}/include/
	${SDK_PATH}/lib_cxng/include/
	${SDK_PATH}/lib_cxng/src/
	${SDK_PATH}/lib_ux/include/
	)

add_compile_options(-g -ggdb2)

if (FUZZ)
add_compile_options(-fsanitize=fuzzer,address)
add_link_options(-fsanitize=fuzzer,address)
# add_compile_definitions(HAVE_PRINTF PRINTF=printf)
endif()

add_compile_definitions(
		FUZZING
        OS_IO_SEPROXYHAL
        IO_SEPROXYHAL_BUFFER_SIZE_B=300
        IO_HID_EP_LENGTH=64
        HAVE_UX_FLOW
        HAVE_BAGL
        APPVERSION="0.0.0"
        BAGL_WIDTH=128
        BAGL_HEIGHT=64
)

# hack so that core_sc000.h compiles
if (WIN32)
add_compile_definitions(__GNUC__)
endif()

add_compile_definitions(
    HAVE_ECC
    HAVE_ECC_WEIERSTRASS
    HAVE_SECP256K1_CURVE
    HAVE_ECDSA
    HAVE_HASH
    HAVE_SHA256
    HAVE_RIPEMD160
	HAVE_HMAC
)

set(LIBUX_PATH ${SDK_PATH}/lib_ux)

set (LIBUX_SRCS
	${LIBUX_PATH}/src/ux_flow_engine.c
	${LIBUX_PATH}/src/ux_layout_bb.c
	${LIBUX_PATH}/src/ux_layout_bn.c
	${LIBUX_PATH}/src/ux_layout_bnn.c
	${LIBUX_PATH}/src/ux_layout_bnnn.c
	${LIBUX_PATH}/src/ux_layout_nn.c
	${LIBUX_PATH}/src/ux_layout_paging.c
	${LIBUX_PATH}/src/ux_layout_paging_compute.c
	${LIBUX_PATH}/src/ux_layout_pbb.c
	${LIBUX_PATH}/src/ux_layout_pn.c
	${LIBUX_PATH}/src/ux_layout_pnn.c
	${LIBUX_PATH}/src/ux_layout_utils.c
	${LIBUX_PATH}/src/ux_stack.c
)

set(SOURCES
        fuzztest.c
		os_mocks.c
		glyphs.c

		../src/eos_parse.c
		../src/eos_parse_eosio.c
		../src/eos_parse_token.c
		../src/eos_parse_unknown.c
		../src/eos_stream.c
		../src/eos_types.c
		../src/eos_utils.c
		${LIBUX_SRCS})

add_executable(fuzzer ${SOURCES})
add_executable(fuzzer_coverage ${SOURCES})

target_compile_options(fuzzer_coverage PRIVATE -fprofile-instr-generate -fcoverage-mapping)

target_link_options(fuzzer PRIVATE)
target_link_options(fuzzer_coverage PRIVATE)
