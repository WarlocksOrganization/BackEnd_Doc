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
)

# 플랫폼별 설정
if(WIN32)
    # Windows 특화 설정
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 이상 대상
    
    if(MSVC)
        # Visual Studio 관련 컴파일러 옵션 설정
        # UTF-8 지원 추가
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /source-charset:utf-8 /execution-charset:utf-8")
        add_compile_options(/wd4819)  # 코드 페이지 경고 무시
        
        # 추가 경고 억제
        add_compile_options(/wd4819)  # 코드 페이지 경고 무시
        add_compile_options(/wd4996)  # 보안 경고 억제
        add_compile_options(/wd4267)  # 'size_t'에서 'type'으로의 변환 가능 데이터 손실 경고 억제
        add_compile_options(/wd4828)  # 잘못된 문자 경고(C4828) 무시
        add_compile_options(/wd4005)  # 매크로 재정의 경고(C4005) 무시
        add_compile_options(/wd5030)  # [[assume]] 특성 인식 불가(C5030) 무시
        add_compile_options(/wd4038)  # C++20 이상에서만 사용 가능한 기능 경고(C4038) 무시
        
        # 전처리기 정의 추가
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        add_definitions(-D_UNICODE -DUNICODE)
    endif()
    
    # Windows용 vcpkg 설치 디렉토리 설정
    set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows")
else()
    # Linux/Unix 설정
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
    add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
    
    # Linux용 vcpkg 설치 디렉토리 설정 (필요시)
    set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux")
endif()

# 소스 디렉토리 설정
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
message(STATUS "Source directory: ${SRC_DIR}")

# 소스 파일과 헤더 파일 명시적으로 나열
set(SOURCE_FILES
    "${SRC_DIR}/main.cpp"
    "${SRC_DIR}/core/server.cpp"
    "${SRC_DIR}/core/session.cpp"
    "${SRC_DIR}/controller/auth_controller.cpp"
    "${SRC_DIR}/controller/room_controller.cpp"
    "${SRC_DIR}/service/auth_service.cpp"
    "${SRC_DIR}/service/room_service.cpp"
    "${SRC_DIR}/repository/user_repository.cpp"
    "${SRC_DIR}/repository/room_repository.cpp"
    "${SRC_DIR}/util/db_pool.cpp"
    "${SRC_DIR}/util/password_util.cpp"
    "${SRC_DIR}/util/session_pool.cpp"
)

# 실행 파일 추가
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# 인클루드 디렉토리 설정
if(EXISTS "${VCPKG_INSTALLED_DIR}/include")
    # vcpkg에서 설치된 라이브러리 사용
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${SRC_DIR}"
        "${SRC_DIR}/core"
        "${SRC_DIR}/controller"
        "${SRC_DIR}/service"
        "${SRC_DIR}/repository"
        "${SRC_DIR}/util"
        "${VCPKG_INSTALLED_DIR}/include"
    )
    
    # 라이브러리 디렉토리 설정
    if(EXISTS "${VCPKG_INSTALLED_DIR}/lib")
        target_link_directories(${PROJECT_NAME} PRIVATE "${VCPKG_INSTALLED_DIR}/lib")
    endif()
    
    # 플랫폼에 따른 라이브러리 파일 처리
    if(WIN32)
        # Windows용 라이브러리 파일 검색
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
        
        # 빌드 후 DLL 파일 복사 (Windows 전용)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            # pqxx.dll 복사
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_INSTALLED_DIR}/bin/pqxx.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/pqxx.dll
            
            # LIBPQ.dll 복사
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_INSTALLED_DIR}/bin/LIBPQ.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/LIBPQ.dll
            
            # libcrypto-3-x64.dll 복사
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_INSTALLED_DIR}/bin/libcrypto-3-x64.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/libcrypto-3-x64.dll
            
            # libssl-3-x64.dll 복사
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_INSTALLED_DIR}/bin/libssl-3-x64.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/libssl-3-x64.dll
            
            COMMENT "Copying required DLLs to output directory"
        )
    else()
        # Linux용 라이브러리 파일 검색
        file(GLOB BOOST_SYSTEM_LIBS "${VCPKG_INSTALLED_DIR}/lib/libboost_system*.a")
        file(GLOB PQXX_LIBS "${VCPKG_INSTALLED_DIR}/lib/libpqxx*.a")
        file(GLOB SSL_LIBS "${VCPKG_INSTALLED_DIR}/lib/libssl*.a")
        file(GLOB CRYPTO_LIBS "${VCPKG_INSTALLED_DIR}/lib/libcrypto*.a")
        
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
            pthread  # Linux에서 필요
            dl       # Linux에서 필요할 수 있음
        )
    endif()
else()
    # vcpkg가 설치되지 않았을 경우 시스템 라이브러리 사용
    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${SRC_DIR}"
        "${SRC_DIR}/core"
        "${SRC_DIR}/controller"
        "${SRC_DIR}/service"
        "${SRC_DIR}/repository"
        "${SRC_DIR}/util"
    )
    
    # 시스템 라이브러리 찾기
    find_package(Boost REQUIRED COMPONENTS system)
    find_package(OpenSSL REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBPQXX REQUIRED libpqxx)
    
    # 시스템 라이브러리 링크
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Boost::system
        OpenSSL::SSL
        OpenSSL::Crypto
        ${LIBPQXX_LIBRARIES}
        pthread
    )
    
    # 추가 인클루드 디렉토리
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${LIBPQXX_INCLUDE_DIRS}
    )
    
    # 추가 라이브러리 경로
    target_link_directories(${PROJECT_NAME} PRIVATE
        ${LIBPQXX_LIBRARY_DIRS}
    )
endif()

# 디버깅 정보 출력 - 파일 존재 확인
message(STATUS "Checking source files existence:")
foreach(SOURCE_FILE ${SOURCE_FILES})
    if(EXISTS "${SOURCE_FILE}")
        message(STATUS "  ${SOURCE_FILE} (exists)")
    else()
        message(STATUS "  ${SOURCE_FILE} (NOT FOUND)")
    endif()
endforeach()