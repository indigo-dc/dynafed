


# Build the Doxygen cfg
configure_file (
               "${PROJECT_SOURCE_DIR}/doc/Doxyfile.in"
               "${PROJECT_BINARY_DIR}/doc/Doxyfile.cfg"
)




# creat doc dir, add doc target, add install doc
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc/html")
ADD_CUSTOM_TARGET(doc
  doxygen "${PROJECT_BINARY_DIR}/doc/Doxyfile.cfg")
SET( l_doc "${CMAKE_CURRENT_BINARY_DIR}/doc/html/")
install(DIRECTORY ${l_doc} DESTINATION ${DOC_INSTALL_DIR}/html) # install examples


file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doc/whitepaper")

install(FILES ${CMAKE_SOURCE_DIR}/doc/whitepaper/Doc_DynaFeds.pdf
  DESTINATION ${DOC_INSTALL_DIR}/whitepaper/
  PERMISSIONS  OWNER_READ GROUP_READ WORLD_READ ) # install the whitepaper