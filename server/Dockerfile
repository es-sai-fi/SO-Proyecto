FROM ubuntu:latest

RUN apt-get update && apt-get install -y gcc && apt-get install -y sudo

WORKDIR /server

COPY server.c .

RUN gcc -o server server.c
