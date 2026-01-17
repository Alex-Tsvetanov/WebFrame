FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update
RUN apt-get install -y git cmake clang

RUN mkdir /src

COPY build_zstd_within_environment.sh /src

ENTRYPOINT /src/build_zstd_within_environment.sh /src /output