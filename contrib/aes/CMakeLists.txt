set(
  aes_srcs
  rijndael-alg-fst.h
  rijndael-alg-fst.c
)

add_library(aes ${aes_srcs})

install(TARGETS aes EXPORT LeapSerialTargets
  DESTINATION lib
  COMPONENT LeapSerial
  CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)
