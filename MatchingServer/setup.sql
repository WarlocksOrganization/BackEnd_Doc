-- 테이블 삭제 (역순으로 삭제하여 참조 무결성 유지)
DROP TABLE IF EXISTS game_users CASCADE;
DROP TABLE IF EXISTS games CASCADE;
DROP TABLE IF EXISTS room_users CASCADE;
DROP TABLE IF EXISTS rooms CASCADE;
DROP TABLE IF EXISTS maps CASCADE;
DROP TABLE IF EXISTS classes CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- 사용자 테이블
CREATE TABLE users (
  user_id SERIAL PRIMARY KEY,
  user_name VARCHAR(50) NOT NULL UNIQUE,
  password_hash VARCHAR(255) NOT NULL,
  created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
  last_login TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- 맵 테이블
CREATE TABLE maps (
  map_id SERIAL PRIMARY KEY,
  map_name VARCHAR(50) NOT NULL UNIQUE
);

-- 클래스 테이블
CREATE TABLE classes (
  class_id SERIAL PRIMARY KEY,
  class_name VARCHAR(50) NOT NULL UNIQUE
);

-- 방 테이블 (컨테이너 기반)
CREATE TABLE rooms (
  room_id SERIAL PRIMARY KEY, -- 컨테이너 ID 또는 커스텀 인덱스
  room_name VARCHAR(100) NOT NULL,
  host_id INTEGER NOT NULL REFERENCES users(user_id),
  ip_address VARCHAR(45), -- IPv4/IPv6 주소
  port INTEGER, -- 포트 번호
  max_players INTEGER NOT NULL DEFAULT 8,
  status VARCHAR(20) NOT NULL DEFAULT 'WAITING',
  created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
  CHECK (status IN ('WAITING', 'GAME_IN_PROGRESS', 'TERMINATED'))
);

-- 방 참가자 테이블 (현재 상태만 관리)
CREATE TABLE room_users (
  room_id INTEGER NOT NULL REFERENCES rooms(room_id),
  user_id INTEGER NOT NULL REFERENCES users(user_id),
  joined_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (room_id, user_id)
);

-- 게임 테이블 (게임 결과 포함)
CREATE TABLE games (
  game_id SERIAL PRIMARY KEY, -- 컨테이너 ID와 연계 또는 고유 식별자
  room_id INTEGER NOT NULL REFERENCES rooms(room_id),
  map_id INTEGER NOT NULL REFERENCES maps(map_id),
  status VARCHAR(20) NOT NULL DEFAULT 'IN_PROGRESS',
  started_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
  completed_at TIMESTAMP WITH TIME ZONE,
  CHECK (status IN ('IN_PROGRESS', 'COMPLETED'))
);

CREATE TABLE game_users (
  game_id INTEGER NOT NULL REFERENCES games(game_id),
  user_id INTEGER NOT NULL REFERENCES users(user_id),
  class_id INTEGER NOT NULL REFERENCES classes(class_id),
  PRIMARY KEY (game_id, user_id)
);

-- 인덱스 생성 (자주 조회되는 필드)
CREATE INDEX idx_users_username ON users(user_name);
CREATE INDEX idx_rooms_status ON rooms(status);
CREATE INDEX idx_rooms_created_at ON rooms(created_at); -- 생성 시간 기준 정렬
CREATE INDEX idx_room_users_user_id ON room_users(user_id);
CREATE INDEX idx_games_room_id ON games(room_id);
CREATE INDEX idx_games_status ON games(status);
CREATE INDEX idx_game_users_game_id ON game_users(game_id);

-- 생성된 테이블에 대한 모든 권한 부여
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO admin;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO admin;

-- 향후 생성될 테이블과 시퀀스에 대한 권한 설정
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL PRIVILEGES ON TABLES TO admin;
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT ALL PRIVILEGES ON SEQUENCES TO admin;

-- 기본 맵 및 클래스 데이터 추가
INSERT INTO maps (map_name) VALUES 
  ('Forest'), 
  ('Desert'), 
  ('Ice Field'), 
  ('Volcano');

INSERT INTO classes (class_name) VALUES 
  ('Warrior'), 
  ('Mage'), 
  ('Archer'), 
  ('Healer');
