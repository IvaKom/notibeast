list(APPEND BEAST_SRC
  websocket_session.cpp
  service.cpp
  listener.cpp
  http_session.cpp
  shared_state.cpp
)
get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME) 
list(TRANSFORM BEAST_SRC PREPEND ${DIR_NAME}/)
set(BEAST_SRC ${BEAST_SRC} PARENT_SCOPE)
