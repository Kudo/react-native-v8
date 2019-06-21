FROM ubuntu:18.04
MAINTAINER https://github.com/Kudo

RUN apt-get update
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get install -y sudo

RUN useradd -rm -d /home/ubuntu -s /bin/bash -g root -G sudo -u 1000 ubuntu
RUN echo "ubuntu ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
USER ubuntu
WORKDIR /home/ubuntu
