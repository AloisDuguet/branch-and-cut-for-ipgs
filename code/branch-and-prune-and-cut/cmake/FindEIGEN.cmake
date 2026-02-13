if (NOT DEFINED EIGEN_DIR)
    set(EIGEN_DIR $ENV{EIGEN_PATH})
endif()

find_path(
        EIGEN_INCLUDE_DIRS
        NAMES Eigen/Dense
        HINTS /usr/local/include/ ${EIGEN_DIR} $ENV{EIGEN_PATH} $ENV{EIGEN_PATH}/Eigen
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EIGEN DEFAULT_MSG EIGEN_INCLUDE_DIRS)

if (EIGEN_FOUND)
    add_library(EIGEN INTERFACE IMPORTED)
    target_include_directories(EIGEN INTERFACE ${EIGEN_INCLUDE_DIRS})
endif()
