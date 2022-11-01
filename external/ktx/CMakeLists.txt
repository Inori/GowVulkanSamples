
set( KTX_FEATURE_VULKAN ON )
set( KTX_FEATURE_KTX1 ON )
set( KTX_FEATURE_KTX2 ON )

set(KTX_MAIN_SRC
    include/ktx.h
    lib/basis_sgd.h
    lib/basis_transcode.cpp
    lib/basisu/transcoder/basisu_containers.h
    lib/basisu/transcoder/basisu_containers_impl.h
    lib/basisu/transcoder/basisu_file_headers.h
    lib/basisu/transcoder/basisu_transcoder_internal.h
    lib/basisu/transcoder/basisu_transcoder_uastc.h
    lib/basisu/transcoder/basisu_transcoder.cpp
    lib/basisu/transcoder/basisu_transcoder.h
    lib/basisu/transcoder/basisu.h
    lib/basisu/zstd/zstd.c
    lib/checkheader.c
    lib/dfdutils/createdfd.c
    lib/dfdutils/colourspaces.c
    lib/dfdutils/dfd.h
    lib/dfdutils/interpretdfd.c
    lib/dfdutils/printdfd.c
    lib/dfdutils/queries.c
    lib/dfdutils/vk2dfd.c
    lib/dfdutils/vk2dfd.inl
    lib/dfdutils/vulkan/vk_platform.h
    lib/dfdutils/vulkan/vulkan_core.h
    lib/etcdec.cxx
    lib/etcunpack.cxx
    lib/filestream.c
    lib/filestream.h
    lib/formatsize.h
    lib/gl_format.h
    lib/hashlist.c
    lib/info.c
    lib/ktxint.h
    lib/memstream.c
    lib/memstream.h
    lib/strings.c
    lib/swap.c
    lib/texture.c
    lib/texture.h
    lib/texture2.c
    lib/texture2.h
    lib/uthash.h
    lib/vk_format.h
    lib/vkformat_check.c
    lib/vkformat_enum.h
    lib/vkformat_str.c
    )


# Main library
add_library( ktx STATIC ${KTX_MAIN_SRC} )

target_include_directories(
    ktx
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/transcoder
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/zstd
    ${CMAKE_CURRENT_SOURCE_DIR}/other_include
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_compile_definitions(ktx PUBLIC KHRONOS_STATIC)
target_compile_definitions(ktx PRIVATE BASISD_SUPPORT_FXT1=0)
target_compile_definitions(
    ktx
PUBLIC
    "$<$<CONFIG:Debug>:_DEBUG;DEBUG>"
PRIVATE
    LIBKTX
)

target_compile_features(ktx PUBLIC c_std_99 cxx_std_11)

set_target_properties(ktx PROPERTIES
    PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/ktx.h)

if(KTX_FEATURE_KTX1)
    target_compile_definitions(ktx PUBLIC KTX_FEATURE_KTX1)
    target_sources(
        ktx
    PRIVATE
        lib/texture1.c
        lib/texture1.h
    )
endif()

if(KTX_FEATURE_KTX2)
    target_compile_definitions(ktx PUBLIC KTX_FEATURE_KTX2)
endif()

if(KTX_FEATURE_VULKAN)
    target_sources(
        ktx
    PRIVATE
        include/ktxvulkan.h
        lib/vk_funcs.c
        lib/vk_funcs.h
        lib/vkloader.c
    )
    target_include_directories(
        ktx
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/dfdutils
    )

    get_target_property( KTX_PUBLIC_HEADER ktx PUBLIC_HEADER )
    list(APPEND KTX_PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/ktxvulkan.h)
    set_target_properties(ktx PROPERTIES PUBLIC_HEADER "${KTX_PUBLIC_HEADER}")
endif()