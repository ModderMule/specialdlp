add_executable(sdlp_test_vectors test_vectors.cpp vectors.h)
target_link_libraries(sdlp_test_vectors PRIVATE specialdlp)
if(MSVC)
    target_compile_options(sdlp_test_vectors PRIVATE /utf-8)
endif()
add_test(NAME sdlp_vectors COMMAND sdlp_test_vectors ${CMAKE_CURRENT_SOURCE_DIR}/vectors.tsv)
if(WIN32)
    # find specialdlp.dll next to the test
    set_target_properties(sdlp_test_vectors PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:specialdlp>)
endif()

# Windows: compare against an upstream antiLeech*.dll built from the original sources
if(WIN32 AND SDLP_DIFF_UPSTREAM_DIR)
    add_executable(sdlp_diff_upstream diff_upstream.cpp vectors.h)
    target_link_libraries(sdlp_diff_upstream PRIVATE specialdlp)
    target_compile_options(sdlp_diff_upstream PRIVATE /utf-8)
    set_target_properties(sdlp_diff_upstream PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:specialdlp>)
    foreach(variant lite allverycd easymule nickname tag)
        add_test(NAME sdlp_diff_${variant}
                 COMMAND sdlp_diff_upstream ${SDLP_DIFF_UPSTREAM_DIR}/${variant}/antiLeechx64.dll ${variant}
                         ${CMAKE_CURRENT_SOURCE_DIR}/vectors.tsv)
    endforeach()
endif()
