#!/bin/bash

# 색상 코드
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "===== 빌드 시작 ====="

# 빌드 디렉토리 정리
if [ -d "build" ]; then
    echo "기존 빌드 디렉토리 정리 중..."
    make clean
fi

# 빌드 실행
echo "프로젝트 빌드 중..."
make -j$(nproc)

# 빌드 결과 확인
if [ $? -eq 0 ]; then
    echo -e "${GREEN}빌드 성공!${NC}"
    echo "실행 파일 위치: ./build/bin/MatchingServer"
else
    echo -e "${RED}빌드 실패${NC}"
    exit 1
fi

echo "===== 빌드 완료 ====="

# 선택적으로 서버 실행
read -p "서버를 실행하시겠습니까? (y/n): " run_server
if [ "$run_server" = "y" ]; then
    ./build/bin/MatchingServer
fi
