vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO kkEngine/NirnLabUIPlatform
  REF 21c1adaa719245e039566f2426bc7cd546005677  # ver3.1PR + a few commits, will likely be tagged as ver3.1
  SHA512 03dd9dff0e43bee963fbe53ceb7407863acfd592b4bc3f87d5aace3687a85cec83f8b37fba150821a0cac16e40a8b5144c0060e507a1a2f94a5fa1bfb69db355
  HEAD_REF main
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(COPY "${SOURCE_PATH}/src/UIPlatform/NirnLabUIPlatformAPI" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
