# Docker image for Sysadmin

FROM debian

MAINTAINER angelalonso <alonsofonseca.angel@gmail.com>

RUN apt-get update \
&& apt-get install -y \
bash \
htop \
netcat \
procps \
python \
python-pip \
tcpdump \
telnet \
vim \
&& apt-get clean \
&& pip install awscli boto3

CMD [ "sh", "-c", "service ssh start; bash"]
