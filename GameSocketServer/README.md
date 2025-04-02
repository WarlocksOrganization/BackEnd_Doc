# GameSocketServer

Boost.Asio로 구축된 고성능 C++ 게임 소켓 서버로, 인증, 세션 관리 및 실시간 게임플레이 통신을 처리합니다.

## 설정 및 설치

### 사전 요구사항

- Visual Studio 2019 이상
- CMake 3.12 이상
- PostgreSQL 데이터베이스 서버
- Git

### 1. vcpkg 설치

먼저 vcpkg 패키지 관리자를 설치합니다:

```powershell
# vcpkg 저장소 클론 및 부트스트랩 스크립트 실행
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

### 2. 의존성 설치

vcpkg를 사용하여 필요한 라이브러리를 설치합니다:

```powershell
# 루트 디렉토리로 이동 및 vcpkg.json에 정의된 의존성 설치
cd ..
\vcpkg\vcpkg install
```

다음 의존성이 설치됩니다:
- boost-system
- boost-asio
- libpqxx
- nlohmann-json
- spdlog
- fmt
- openssl

### 3. 프로젝트 빌드

#### 빌드 스크립트 사용

편의를 위해 PowerShell 빌드 스크립트를 제공합니다:

```powershell
# 루트 디렉토리에서 실행해 주면 됩니다.
.\rebuild.ps1
```

#### 수동 빌드

또는 CMake를 사용하여 수동으로 빌드할 수 있습니다:

```powershell
# 빌드 디렉토리 생성 및 이동
mkdir build
cd build

# CMake 구성
cmake -G "Visual Studio 17 2022" -A x64 ..

# 빌드
cmake --build . --config Release
```

### 4. 데이터베이스 설정

서버를 실행하기 전에 PostgreSQL 데이터베이스를 설정합니다:

1. 아직 설치되지 않은 경우 PostgreSQL 설치
2. "GameData"라는 이름의 데이터베이스 생성
3. `db/init_db.sql`의 SQL 스크립트를 실행하여 필요한 테이블 생성
4. 서버에서 데이터베이스 연결 문자열 구성(명령줄을 통해 또는 코드 편집)

## 서버 실행

다음 명령으로 서버를 실행합니다:

```powershell
.\build\Release\SocketServer.exe
```

## 아키텍처

서버는 MVC와 유사한 아키텍처를 따릅니다:

- **컨트롤러**: 들어오는 요청 처리
- **서비스**: 비즈니스 로직 구현
- **레포지토리**: 데이터 액세스 관리
- **DTO**: 계층 간 데이터 전송

## 기능

- 사용자 인증(회원가입, 로그인)
- 세션 관리
- 데이터베이스 연결 풀링
- JSON 기반 통신 프로토콜
- 비동기 소켓 처리

## 개발

### 프로젝트 구조

- `src/core/` - 코어 서버 컴포넌트
- `src/controller/` - 요청 처리 컨트롤러
- `src/service/` - 비즈니스 로직 구현
- `src/repository/` - 데이터 액세스 계층
- `src/dto/` - 데이터 전송 객체
- `src/entity/` - 도메인 엔티티
- `src/util/` - 유틸리티 클래스

### 개발용 빌드

디버깅 정보가 포함된 개발 빌드:

```powershell
cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug
```