vcpkg_download_distfile(ARCHIVE
    URLS "https://skse.silverlock.org/beta/sksevr_2_00_12.7z"
    FILENAME "sksevr_2_00_12.7z"
    SHA512 9042a86d19a28c4a8cc3803e8153ecd7c27f2c311ab92863c28a2e550d6653728dc8df24fa4dcb3797f118bffbd38f199e3fb169fcfd4cd4a2820c7ce4d49a3d
)

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${ARCHIVE}
    NO_REMOVE_ONE_LEVEL
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
)
vcpkg_build_cmake()
vcpkg_install_cmake()
vcpkg_copy_pdbs()

file(GLOB sksevr_include "${SOURCE_PATH}/sksevr_2_00_12/src/sksevr/skse64/*.h")
file(INSTALL ${sksevr_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/skse64")

file(GLOB sksevr_common_include "${SOURCE_PATH}/sksevr_2_00_12/src/sksevr/skse64_common/*.h")
file(INSTALL ${sksevr_common_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/skse64_common")

file(GLOB commonvr_include "${SOURCE_PATH}/sksevr_2_00_12/src/common/*.h")
file(INSTALL ${commonvr_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/common")

file(GLOB xbyak_include "${SOURCE_PATH}/sksevr_2_00_12/src/sksevr/xbyak/*.h")
file(INSTALL ${xbyak_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/xbyak")

file(WRITE ${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\nThank you MIT license for providing a standard boilerplate legal disclaimer. This reference does not mean SKSE is released under the MIT license.")
