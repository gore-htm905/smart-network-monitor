FROM gcc:13 AS build

RUN apt-get update && apt-get install -y cmake make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend/CMakeLists.txt backend/CMakeLists.txt
COPY backend/include backend/include
COPY backend/src backend/src

RUN cmake -S backend -B build \
    && cmake --build build --config Release

FROM debian:bookworm-slim

WORKDIR /app

COPY --from=build /app/build/BackendServer /app/BackendServer

ENV PORT=8085

EXPOSE 8085

CMD ["/app/BackendServer"]
