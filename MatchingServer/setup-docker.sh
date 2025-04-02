#!/bin/bash

# 실패 시 스크립트 중단
set -e

echo "===== 게임 서버 Docker 환경 구성 시작 ====="

# 디렉토리 구조 생성
mkdir -p db_init

# DB 초기화 SQL 스크립트 복사
cp db-init.sql db_init/init.sql

# Docker가 설치되어 있는지 확인
if ! command -v docker &> /dev/null; then
    echo "Docker가 설치되어 있지 않습니다. 설치를 진행합니다..."
    chmod +x docker-install.sh
    ./docker-install.sh
    
    # Docker 그룹 적용 (현재 세션에)
    newgrp docker
else
    echo "Docker가 이미 설치되어 있습니다."
fi

# Docker Compose가 설치되어 있는지 확인
if ! command -v docker-compose &> /dev/null; then
    echo "Docker Compose가 설치되어 있지 않습니다. 설치를 진행합니다..."
    DOCKER_COMPOSE_VERSION=$(curl -s https://api.github.com/repos/docker/compose/releases/latest | grep 'tag_name' | cut -d\" -f4)
    sudo curl -L "https://github.com/docker/compose/releases/download/${DOCKER_COMPOSE_VERSION}/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
    sudo chmod +x /usr/local/bin/docker-compose
else
    echo "Docker Compose가 이미 설치되어 있습니다."
fi

# Docker 서비스 실행 중인지 확인
if ! systemctl is-active --quiet docker; then
    echo "Docker 서비스를 시작합니다..."
    sudo systemctl start docker
fi

# Docker Compose로 서버 빌드 및 실행
echo "Docker Compose로 서버 빌드 및 실행 중..."
docker-compose up -d --build

echo "===== 게임 서버 Docker 환경 구성 완료 ====="
echo "매칭 서버가 포트 8080에서 실행 중입니다."
echo "PostgreSQL 데이터베이스가 포트 5432에서 실행 중입니다."
echo "컨테이너 상태 확인:"
docker-compose ps

echo ""
echo "로그 확인 방법:"
echo "  매칭 서버: docker logs matching_server"
echo "  데이터베이스: docker logs game_db"
