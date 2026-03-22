vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO kkEngine/NirnLabUIPlatform
  REF 21c1adaa719245e039566f2426bc7cd546005677  # ver3.1PR + a few commits, will likely be tagged as ver3.1
  SHA512 7acc44e6af661b10a1082fe5a82ddbe0276912d04622c2295c2c3cd61be27e24b14764d4c0c2b767b9ad8bc567a0d57ccba7712067601a9b1595383cd7e2420e
  HEAD_REF main
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(COPY "${SOURCE_PATH}/src/UIPlatform/NirnLabUIPlatformAPI" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
