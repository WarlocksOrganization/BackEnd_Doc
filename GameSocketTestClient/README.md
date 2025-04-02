# GameSocketTestClient

GameSocketServer 테스트를 위한 클라이언트 애플리케이션입니다.

## 개발 환경 설정

### 사전 요구사항
- Visual Studio 2019 이상
- GameSocketServer 프로젝트의 vcpkg 라이브러리 (동일 레포지토리 내에 위치)

### 프로젝트 설정 방법

1. **프로젝트 열기**
   - Visual Studio에서 `GameSocketTestClient.sln` 파일을 엽니다.

2. **라이브러리 경로 설정**
   - 프로젝트 → 속성 → C/C++ → 일반 → 추가 포함 디렉터리에 다음을 추가:
     ```
     ..\GameSocketServer\vcpkg_installed\x64-windows\include
     ```
   
   - 프로젝트 → 속성 → 링커 → 일반 → 추가 라이브러리 디렉터리에 다음을 추가:
     ```
     ..\GameSocketServer\vcpkg_installed\x64-windows\lib
     ```
   
   - 프로젝트 → 속성 → 링커 → 입력 → 추가 종속성에 다음을 추가:
     ```
     boost_system-vc143-mt-x64-1_87.lib
     libssl.lib
     libcrypto.lib
     ```

3. **빌드 후 이벤트 설정 (필요한 DLL 파일 복사)**
   - 프로젝트 → 속성 → 빌드 이벤트 → 빌드 후 이벤트 → 명령줄에 다음을 추가:
     ```
     xcopy /y "$(ProjectDir)..\GameSocketServer\vcpkg_installed\x64-windows\bin\boost_system-vc143-mt-x64-1_87.dll" "$(OutDir)"
     xcopy /y "$(ProjectDir)..\GameSocketServer\vcpkg_installed\x64-windows\bin\libssl-3.dll" "$(OutDir)"
     xcopy /y "$(ProjectDir)..\GameSocketServer\vcpkg_installed\x64-windows\bin\libcrypto-3.dll" "$(OutDir)"
     ```
   - DLL 파일명은 실제 파일명과 일치하도록 수정하세요.

## 사용 방법

1. GameSocketServer를 먼저 실행합니다.
2. GameSocketTestClient를 실행합니다.
3. 서버 IP와 포트를 입력합니다 (기본값: 127.0.0.1:8080).
4. 회원가입 또는 로그인 작업을 선택하고 필요한 정보를 입력합니다.