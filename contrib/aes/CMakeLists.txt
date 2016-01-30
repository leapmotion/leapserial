set(
  aes_srcs
  aes256.h
  aes256.c
  rijndael-alg-fst.h
  rijndael-alg-fst.c
)

add_library(aes ${aes_srcs})

install(TARGETS aes EXPORT LeapSerialTargets
  DESTINATION lib
  COMPONENT LeapSerial
  CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)
