configure_file(
    "${SCHIP_HDR_DIR}/config.h.in"
    "${SCHIP_HDR_DIR}/config.h"
)

set(SCHIP_SOURCES
    chip.cpp
    display.cpp
    memory.cpp
    main.cpp
)

set(SCHIP_HEADERS
    chip.h
    config.h
    display.h
    keypad.h
    memory.h
)

list(TRANSFORM SCHIP_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
list(TRANSFORM SCHIP_HEADERS PREPEND "${SCHIP_HDR_DIR}/")

set(SCHIP_SOURCES ${SCHIP_SOURCES} PARENT_SCOPE)
set(SCHIP_HEADERS ${SCHIP_HEADERS} PARENT_SCOPE)
