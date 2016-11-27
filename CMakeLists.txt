cmake_minimum_required (VERSION 2.6)
project (SSD1306)

set(CMAKE_CURRENT_SOURCE_DIR .)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_PREFIX /usr/)
include_directories(.)

set(SSD1306_SOURCE
  ssd1306.cpp
)

set(SSD1306_HEADERS
  ssd1306.h
)

add_library(ssd1306 SHARED ${SSD1306_SOURCE} ${SSD1306_HEADERS})
target_link_libraries(ssd1306 firmware_i2c)

set_target_properties(ssd1306 PROPERTIES
  VERSION 1.0.0
  SOVERSION 1
)

install(TARGETS ssd1306 DESTINATION lib)
install(FILES ${SSD1306_HEADERS} DESTINATION include)
