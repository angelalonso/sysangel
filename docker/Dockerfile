# Docker image for Sysadmin

FROM debian

MAINTAINER angelalonso <alonsofonseca.angel@gmail.com>

RUN apt-get update \
&& apt-get install -y \
bash \
htop \
netcat \
python \
python-pip \
tcpdump \
vim \
&& apt-get clean \
&& pip install awscli

CMD ['/bin/bash']
