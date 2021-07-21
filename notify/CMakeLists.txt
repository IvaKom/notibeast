list(APPEND NOTIFY_SRC
   i_notify.cpp
   recursive_i_notify.cpp
   i_notify_helper.cpp
   recursive_notify_event.cpp
   notify_event.cpp
)
get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
list(TRANSFORM NOTIFY_SRC PREPEND ${DIR_NAME}/)
set(NOTIFY_SRC ${NOTIFY_SRC} PARENT_SCOPE)
