

# create the specfile
configure_file (
               "${CMAKE_CURRENT_SOURCE_DIR}/rpm/specs/dynafed.spec.in"
               "${CMAKE_CURRENT_SOURCE_DIR}/rpm/specs/dynafed.spec"
               @ONLY
)
