# Docker image for Sysadmin

FROM debian

MAINTAINER angelalonso <alonsofonseca.angel@gmail.com>

ENV HOME /home/aaf
#ENV HOME /root

RUN apt-get update \
&& apt-get install -y \
bash \
htop \
netcat \
nfs-common \
openssh-client \
openssh-server \
procps \
python \
python-pip \
sudo \
tcpdump \
telnet \
vim \
zsh \
&& apt-get clean \
&& pip install awscli boto3 \
&& useradd -ms /bin/zsh aaf \
&& passwd -d aaf \
&& usermod -aG sudo aaf



USER aaf
#USER root

WORKDIR /home/aaf
#WORKDIR /root

CMD [ "bash" ]
