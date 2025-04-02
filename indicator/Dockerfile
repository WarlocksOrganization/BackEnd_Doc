FROM gradle:8.5-jdk21

COPY ./build/libs/indicator-0.0.1-SNAPSHOT.jar app.jar

EXPOSE 8085

ENTRYPOINT ["java","-jar","app.jar"]
