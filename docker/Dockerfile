# Docker image for Sysadmin

FROM debian

MAINTAINER angelalonso <alonsofonseca.angel@gmail.com>

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
&& usermod -aG sudo aaf \
&& chown -R aaf:aaf /home/aaf

USER aaf

ENV HOME /home/aaf

WORKDIR /home/aaf

CMD [ "bash" ]
