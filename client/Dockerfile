FROM ubuntu:latest

RUN apt-get update && apt-get install -y gcc 

WORKDIR /client

COPY client.c .

RUN gcc -o client client.c