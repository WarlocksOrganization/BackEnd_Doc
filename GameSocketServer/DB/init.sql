-- 기존 인덱스 삭제
DROP INDEX IF EXISTS 
    idx_users_username,
    idx_users_rating,
    idx_gamesessions_lobby,
    idx_gamesessions_status_time,
    idx_rounds_session,
    idx_roundplayers_round_placement,
    idx_roundplayers_user,
    idx_sessionplayers_user,
    idx_sessionplayers_session,
    idx_gameevents_round_time,
    idx_gameevents_player_type,
    idx_gameevents_type_time,
    idx_lobbies_status,
    idx_lobbies_creator,
    idx_lobbyplayers_user,
    idx_lobbyevents_lobby_time,
    idx_lobbyevents_user_type,
    idx_gameevents_jsonb,
    idx_lobbyevents_jsonb;

-- 기존 테이블 삭제 (의존성 역순으로)
DROP TABLE IF EXISTS LobbyEvents CASCADE;
DROP TABLE IF EXISTS LobbyPlayers CASCADE;
DROP TABLE IF EXISTS GameEvents CASCADE;
DROP TABLE IF EXISTS RoundPlayers CASCADE;
DROP TABLE IF EXISTS SessionPlayers CASCADE;
DROP TABLE IF EXISTS GameRounds CASCADE;
DROP TABLE IF EXISTS GameSessions CASCADE;
DROP TABLE IF EXISTS Lobbies CASCADE;
DROP TABLE IF EXISTS Users CASCADE;

-- 테이블 생성
-- 사용자 테이블
CREATE TABLE Users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(100) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login TIMESTAMP,
    total_games INT DEFAULT 0,
    total_wins INT DEFAULT 0,
    rating INT DEFAULT 1000
);

-- 방 테이블 (기존 Lobbies 대체)
CREATE TABLE Rooms (
    room_id SERIAL PRIMARY KEY,
    room_name VARCHAR(40) NOT NULL,
    creator_id INT REFERENCES Users(user_id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    closed_at TIMESTAMP,
    max_players INT DEFAULT 6,
    map_id INT,
    game_mode VARCHAR(20),
    status VARCHAR(20) DEFAULT 'open'
);

-- 게임 세션 테이블 (Room 참조로 업데이트)
CREATE TABLE GameSessions (
    session_id SERIAL PRIMARY KEY,
    session_code VARCHAR(20) NOT NULL UNIQUE,
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP,
    map_id INT,
    game_mode VARCHAR(20),
    total_rounds INT DEFAULT 3,
    room_id INT REFERENCES Rooms(room_id),
    status VARCHAR(20) DEFAULT 'active'
);

-- 게임 라운드 테이블 (변경 없음)
CREATE TABLE GameRounds (
    round_id SERIAL PRIMARY KEY,
    session_id INT REFERENCES GameSessions(session_id),
    round_number INT NOT NULL,
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP,
    status VARCHAR(20) DEFAULT 'active',
    UNIQUE (session_id, round_number)
);

-- 세션 플레이어 테이블 (변경 없음)
CREATE TABLE SessionPlayers (
    id SERIAL PRIMARY KEY,
    session_id INT REFERENCES GameSessions(session_id),
    user_id INT REFERENCES Users(user_id),
    team_id INT NULL DEFAULT NULL,
    character_id INT NOT NULL,
    join_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    leave_time TIMESTAMP,
    final_score INT,
    final_placement INT,
    old_rating INT,
    new_rating INT,
    rating_change INT,
    UNIQUE (session_id, user_id)
);

-- 라운드 플레이어 테이블 (변경 없음)
CREATE TABLE RoundPlayers (
    id SERIAL PRIMARY KEY,
    round_id INT REFERENCES GameRounds(round_id),
    user_id INT REFERENCES Users(user_id),
    placement INT,
    survival_time INT,
    kills INT DEFAULT 0,
    damage_dealt INT DEFAULT 0,
    round_score INT DEFAULT 0,
    is_survived BOOLEAN DEFAULT false,
    UNIQUE (round_id, user_id)
);

-- 게임 이벤트 테이블 (변경 없음)
CREATE TABLE GameEvents (
    event_id SERIAL PRIMARY KEY,
    round_id INT REFERENCES GameRounds(round_id),
    event_type VARCHAR(20) NOT NULL,
    event_time TIMESTAMP NOT NULL,
    player_id INT REFERENCES Users(user_id),
    target_id INT REFERENCES Users(user_id) NULL,
    position_x FLOAT,
    position_y FLOAT,
    position_z FLOAT,
    value INT,
    additional_data JSONB
);

-- 방 플레이어 테이블 (LobbyPlayers 대체)
CREATE TABLE RoomPlayers (
    id SERIAL PRIMARY KEY,
    room_id INT REFERENCES Rooms(room_id),
    user_id INT REFERENCES Users(user_id),
    join_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    leave_time TIMESTAMP,
    UNIQUE (room_id, user_id)
);

-- 방 이벤트 테이블 (LobbyEvents 대체)
CREATE TABLE RoomEvents (
    event_id SERIAL PRIMARY KEY,
    room_id INT REFERENCES Rooms(room_id),
    event_type VARCHAR(30) NOT NULL,
    event_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    user_id INT REFERENCES Users(user_id),
    target_id INT REFERENCES Users(user_id) NULL,
    event_data JSONB
);

-- 인덱스 생성
-- 사용자 테이블 인덱스
CREATE INDEX idx_users_username ON Users(username);
CREATE INDEX idx_users_rating ON Users(rating DESC);

-- 게임 세션 테이블 인덱스
CREATE INDEX idx_gamesessions_room ON GameSessions(room_id);
CREATE INDEX idx_gamesessions_status_time ON GameSessions(status, start_time DESC);

-- 게임 라운드 테이블 인덱스
CREATE INDEX idx_rounds_session ON GameRounds(session_id);

-- 라운드 플레이어 테이블 인덱스
CREATE INDEX idx_roundplayers_round_placement ON RoundPlayers(round_id, placement);
CREATE INDEX idx_roundplayers_user ON RoundPlayers(user_id);

-- 세션 플레이어 테이블 인덱스
CREATE INDEX idx_sessionplayers_user ON SessionPlayers(user_id);
CREATE INDEX idx_sessionplayers_session ON SessionPlayers(session_id);

-- 게임 이벤트 테이블 인덱스
CREATE INDEX idx_gameevents_round_time ON GameEvents(round_id, event_time);
CREATE INDEX idx_gameevents_player_type ON GameEvents(player_id, event_type);
CREATE INDEX idx_gameevents_type_time ON GameEvents(event_type, event_time);

-- 방 테이블 인덱스 (Lobbies 인덱스 대체)
CREATE INDEX idx_rooms_status ON Rooms(status, created_at DESC);
CREATE INDEX idx_rooms_creator ON Rooms(creator_id);

-- 방 플레이어 테이블 인덱스 (LobbyPlayers 인덱스 대체)
CREATE INDEX idx_roomplayers_user ON RoomPlayers(user_id);

-- 방 이벤트 테이블 인덱스 (LobbyEvents 인덱스 대체)
CREATE INDEX idx_roomevents_room_time ON RoomEvents(room_id, event_time);
CREATE INDEX idx_roomevents_user_type ON RoomEvents(user_id, event_type);

-- JSONB 인덱스
CREATE INDEX idx_gameevents_jsonb ON GameEvents USING GIN (additional_data jsonb_path_ops);
CREATE INDEX idx_roomevents_jsonb ON RoomEvents USING GIN (event_data jsonb_path_ops);