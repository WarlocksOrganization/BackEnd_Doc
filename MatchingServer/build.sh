#!/bin/bash

# 색상 코드
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
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

# 기존 서버 프로세스 종료
PID=$(pgrep -f "MatchingServer" || echo "")
if [ ! -z "$PID" ]; then
    echo "기존 서버 프로세스(PID: $PID) 종료 중..."
    kill $PID
    sleep 2
    
    # 프로세스가 종료되었는지 확인
    if ps -p $PID > /dev/null; then
        echo -e "${YELLOW}정상 종료 실패, 강제 종료합니다...${NC}"
        kill -9 $PID
        sleep 1
    fi
fi