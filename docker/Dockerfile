# Docker image for Sysadmin

FROM debian

MAINTAINER angelalonso <alonsofonseca.angel@gmail.com>

ENV HOME /root

RUN apt-get update \
&& apt-get install -y \
bash \
htop \
netcat \
openssh-client \
openssh-server \
procps \
python \
python-pip \
tcpdump \
telnet \
vim \
&& apt-get clean \
&& pip install awscli boto3

WORKDIR /root

CMD [ "bash" ]
