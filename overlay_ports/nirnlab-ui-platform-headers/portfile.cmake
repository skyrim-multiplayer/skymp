vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO kkEngine/NirnLabUIPlatform
  REF 302a2b6686bd1ae517bf51a1e20e6b034d77a8fd  # ver3.0PR + 1 (version was not updated yet)
  SHA512 7d5d898fa1bf7caacb881252a6f2c44c283715a5303c5c063e0fb1ba10a33d45b42eb3a0a911394dea988229da3f280cb146b075373c01b0939e9f66ee1c5c02
  HEAD_REF main
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(COPY "${SOURCE_PATH}/src/UIPlatform/NirnLabUIPlatformAPI" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
