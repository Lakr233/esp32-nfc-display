file(GLOB SRC_FILES "*.c")
idf_component_register(SRCS ${SRC_FILES}
                    INCLUDE_DIRS ".")