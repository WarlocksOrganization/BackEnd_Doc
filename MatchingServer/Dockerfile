# 1단계: vcpkg 빌드 및 의존성 설치
FROM ubuntu:22.04 AS vcpkg-builder

# 기본 환경 변수 설정
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Seoul

# vcpkg 빌드에 필요한 최소한의 패키지만 설치
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    git \
    ninja-build \
    cmake \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# vcpkg 설치 (소스 코드 없이)
WORKDIR /vcpkg-build
RUN git clone https://github.com/microsoft/vcpkg.git . \
    && ./bootstrap-vcpkg.sh

# vcpkg.json 파일만 복사하여 의존성 설치
COPY vcpkg.json .
RUN ./vcpkg install --triplet=x64-linux

# 2단계: 애플리케이션 빌드
FROM ubuntu:22.04 AS builder

# 기본 환경 변수 설정
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Seoul

# 빌드에 필요한 패키지 설치
RUN apt-get update && apt-get install -y \
    build-essential \
    pkg-config \
    bison \
    flex \
    libssl-dev \
    zlib1g-dev \
    libpq-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# 작업 디렉터리 설정
WORKDIR /app

# 첫 번째 단계에서 빌드한 vcpkg와 설치된 패키지를 복사
COPY --from=vcpkg-builder /vcpkg-build /app/vcpkg
COPY --from=vcpkg-builder /vcpkg-build/vcpkg_installed /app/vcpkg_installed

# 소스 코드 복사
COPY . .

# 빌드
RUN export LD_LIBRARY_PATH="$PWD/vcpkg_installed/x64-linux/lib:$LD_LIBRARY_PATH" \
    && make

# 3단계: 실행 환경 설정
FROM ubuntu:22.04

ENV TZ=Asia/Seoul

# 실행에 필요한 최소한의 패키지만 설치
RUN apt-get update && apt-get install -y \
    libssl-dev \
    libpq-dev \
    ca-certificates \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 빌드된 바이너리와 필요한 라이브러리만 복사
COPY --from=builder /app/build/bin/MatchingServer /app/
COPY --from=builder /app/vcpkg_installed/x64-linux/lib /app/lib
COPY --from=builder /app/vcpkg_installed/x64-linux/bin /app/bin

# 라이브러리 경로 설정
ENV LD_LIBRARY_PATH="/app/lib:${LD_LIBRARY_PATH}"

# 서버 포트 노출
EXPOSE 8080

# 컨테이너 실행 시 서버 시작
CMD ["./MatchingServer"]