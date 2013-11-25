add_executable(contourpp contourpp.cpp contourpp_driver.cpp hid_commands.cpp)
target_link_libraries(contourpp ${LIBS})
install (TARGETS contourpp DESTINATION bin)
