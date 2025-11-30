FROM ubuntu:22.04

RUN apt update && apt install -y gcc make

WORKDIR /app

COPY . /app

RUN make

CMD ["bash"]

