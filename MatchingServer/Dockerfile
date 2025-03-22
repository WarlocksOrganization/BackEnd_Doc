FROM ubuntu:22.04

# 기본 환경 변수 설정
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Seoul

# 필요한 패키지 설치
RUN apt-get update && apt-get install -y \
    build-essential \
    pkg-config \
    bison \
    flex \
    libssl-dev \
    zlib1g-dev \
    autoconf \
    automake \
    libtool \
    m4 \
    cmake \
    uuid-dev \
    libpq-dev \
    curl \
    unzip \
    tar \
    python3 \
    python3-pip \
    ninja-build \
    git \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# 작업 디렉터리 설정
WORKDIR /app

# 프로젝트 소스 코드 복사
COPY . .

# vcpkg 설치 및 라이브러리 빌드
RUN git clone https://github.com/microsoft/vcpkg.git \
    && cd vcpkg \
    && ./bootstrap-vcpkg.sh \
    && cd ..

# vcpkg로 패키지 설치
RUN ./vcpkg/vcpkg install --triplet=x64-linux

# 빌드
RUN export LD_LIBRARY_PATH="$PWD/vcpkg_installed/x64-linux/lib:$LD_LIBRARY_PATH" \
    && make

# PostgreSQL 클라이언트 라이브러리 경로 설정
ENV LD_LIBRARY_PATH="/app/vcpkg_installed/x64-linux/lib:${LD_LIBRARY_PATH}"

# 서버 포트 노출
EXPOSE 8080

# 컨테이너 실행 시 서버 시작
CMD ["./build/bin/MatchingServer"]
