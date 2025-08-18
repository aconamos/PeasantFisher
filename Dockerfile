FROM rust:latest AS time_builder
WORKDIR /usr/src/peasantfisher
COPY . .
WORKDIR /usr/src/peasantfisher/peasant_time
RUN cargo build --release --lib

FROM debian:trixie-slim
WORKDIR /usr/src/peasantfisher
COPY . .
RUN apt update
RUN apt install build-essential git make libcurl4-openssl-dev libjsmn-dev -y
COPY --from=time_builder /usr/src/peasantfisher/peasant_time/target/release /usr/src/peasantfisher/peasant_time/target/release
RUN make

CMD [ "./bin/release"]