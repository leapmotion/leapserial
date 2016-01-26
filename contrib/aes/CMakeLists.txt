set(
  aes_srcs
  aes256.h
  aes256.c
)

add_library(aes STATIC ${aes_srcs})

install(TARGETS aes EXPORT LeapSerialTargets
  DESTINATION lib
  COMPONENT LeapSerial
  CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)
