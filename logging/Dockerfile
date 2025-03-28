## ---- 1단계: Build Stage ----
#FROM gradle:8.5-jdk21 AS build
#WORKDIR /app
#
## 의존성 캐싱을 위해 gradle 관련 파일만 먼저 복사
##COPY build.gradle.kts .
##COPY settings.gradle.kts .
##COPY gradle ./gradle
#
## 의존성만 미리 다운로드 (캐싱)
##RUN gradle build --no-daemon --stacktrace || true
#
## 전체 소스 복사 후 빌드
#COPY . .
#RUN gradle clean build
#
#FROM gradle:8.5-jdk21
#
#WORKDIR /app
#
#COPY --from=build /app/build/libs/*.jar app.jar
#
#EXPOSE 8001
#
#ENTRYPOINT ["java","-jar","app.jar"]

FROM gradle:8.5-jdk21

COPY ./build/libs/logging-0.0.1-SNAPSHOT.jar app.jar

EXPOSE 8001

ENTRYPOINT ["java","-jar","app.jar"]
