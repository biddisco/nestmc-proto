set(MODCC_SOURCES
    astmanip.cpp
    constantfolder.cpp
    cprinter.cpp
    cudaprinter.cpp
    errorvisitor.cpp
    expression.cpp
    expressionclassifier.cpp
    functionexpander.cpp
    functioninliner.cpp
    lexer.cpp
    module.cpp
    parser.cpp
    textbuffer.cpp
    token.cpp
)

add_library(compiler ${MODCC_SOURCES})

add_executable(modcc modcc.cpp)

target_link_libraries(modcc LINK_PUBLIC compiler)

set_target_properties(modcc
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modcc"
)

