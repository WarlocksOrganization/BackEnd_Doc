cmake_minimum_required(VERSION 3.12)
project(SocketServer)

# C++17 표준 사용
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 중요: fmt와 spdlog를 헤더 전용으로 사용
add_definitions(
    -DFMT_HEADER_ONLY      # fmt를 헤더 전용으로 사용
    -DSPDLOG_FMT_EXTERNAL  # spdlog가 외부 fmt를 사용하도록 설정
    -DSPDLOG_HEADER_ONLY   # spdlog도 헤더 전용으로 사용
    -D_WIN32_WINNT=0x0601  # Windows 7 이상 대상
)

# Visual Studio 관련 컴파일러 옵션 설정
if(MSVC)
    # UTF-8 지원 추가
    add_compile_options(/utf-8)
endif()

# 소스 디렉토리 설정
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
message(STATUS "Source directory: ${SRC_DIR}")

# vcpkg 설치 디렉토리 설정
set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows")

# 소스 파일과 헤더 파일 명시적으로 나열
set(SOURCE_FILES
    "${SRC_DIR}/main.cpp"
    "${SRC_DIR}/core/server.cpp"
    "${SRC_DIR}/core/session.cpp"
    "${SRC_DIR}/controller/auth_controller.cpp"
    "${SRC_DIR}/service/auth_service.cpp"
    "${SRC_DIR}/repository/user_repository.cpp"
    "${SRC_DIR}/util/db_pool.cpp"
    "${SRC_DIR}/util/password_util.cpp"  # 추가된 부분
)

# 모든 헤더 디렉토리를 명시적으로 추가
set(INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${SRC_DIR}"
    "${SRC_DIR}/core"
    "${SRC_DIR}/controller"
    "${SRC_DIR}/service"
    "${SRC_DIR}/repository"
    "${SRC_DIR}/dto"
    "${SRC_DIR}/dto/request"
    "${SRC_DIR}/dto/response"
    "${SRC_DIR}/entity"
    "${SRC_DIR}/util"
    "${VCPKG_INSTALLED_DIR}/include"
)

# 실행 파일 추가
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# 인클루드 디렉토리 설정
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

# 라이브러리 디렉토리 설정
target_link_directories(${PROJECT_NAME} PRIVATE
    "${VCPKG_INSTALLED_DIR}/lib"
)

# 라이브러리 파일 검색
file(GLOB BOOST_SYSTEM_LIBS "${VCPKG_INSTALLED_DIR}/lib/boost_system*.lib")
file(GLOB PQXX_LIBS "${VCPKG_INSTALLED_DIR}/lib/pqxx*.lib")
file(GLOB SSL_LIBS "${VCPKG_INSTALLED_DIR}/lib/*ssl*.lib")
file(GLOB CRYPTO_LIBS "${VCPKG_INSTALLED_DIR}/lib/*crypto*.lib")

message(STATUS "Found Boost system libraries: ${BOOST_SYSTEM_LIBS}")
message(STATUS "Found PQXX libraries: ${PQXX_LIBS}")
message(STATUS "Found SSL libraries: ${SSL_LIBS}")
message(STATUS "Found CRYPTO libraries: ${CRYPTO_LIBS}")

# 라이브러리 링크
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${BOOST_SYSTEM_LIBS}
    ${PQXX_LIBS}
    ${SSL_LIBS}
    ${CRYPTO_LIBS}
)

# 빌드 후 모든 DLL 파일 복사
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${VCPKG_INSTALLED_DIR}/bin/pqxx.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/pqxx.dll
    COMMENT "Copying pqxx.dll to output directory"
)

# 디버깅 정보 출력 - 파일 존재 확인
message(STATUS "Checking source files existence:")
foreach(SOURCE_FILE ${SOURCE_FILES})
    if(EXISTS "${SOURCE_FILE}")
        message(STATUS "  ${SOURCE_FILE} (exists)")
    else()
        message(STATUS "  ${SOURCE_FILE} (NOT FOUND)")
    endif()
endforeach()