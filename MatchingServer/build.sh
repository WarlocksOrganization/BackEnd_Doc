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

# 로그 디렉토리 생성
mkdir -p logs

# 날짜를 포함한 로그 파일명 생성
LOG_DATE=$(date +%Y%m%d_%H%M%S)
LOG_FILE="logs/server_${LOG_DATE}.log"

# 서버를 백그라운드로 실행하고 로그 저장
echo "서버를 백그라운드로 시작합니다..."
echo "로그 파일: $LOG_FILE"

nohup ./build/bin/MatchingServer >> "$LOG_FILE" 2>&1 &

# 새 프로세스 ID 출력
NEW_PID=$!
echo "서버가 PID $NEW_PID로 시작되었습니다."

# 최신 로그 파일에 대한 심볼릭 링크 생성/업데이트
ln -sf "$LOG_FILE" logs/server_latest.log

# 초기 로그 표시
echo "초기 서버 로그 (5초 동안):"
sleep 2  # 서버가 시작하는데 약간의 시간을 줍니다
tail -f "$LOG_FILE" & 
TAIL_PID=$!
sleep 5
kill $TAIL_PID 2>/dev/null

# 과거 로그 정리 (선택적)
# 30일 이상 된 로그 파일 삭제
find logs -name "server_*.log" -type f -mtime +30 -delete 2>/dev/null

echo -e "${GREEN}서버가 백그라운드에서 실행 중입니다.${NC}"
echo "실시간 로그 확인: tail -f logs/server_latest.log"
echo "전체 로그 목록 확인: ls -la logs/"