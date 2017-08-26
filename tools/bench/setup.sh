#!/bin/bash

# This setup script is intende for use on AWS EC2 machines running Ubuntu 16.04

OC_BRANCH=obliv-c
OBLIVC_COMMIT=4bac9b7858dcbfe0b78a0b5355d305dfc4bcd054
ACK_BRANCH=floram
ACK_COMMIT=aae6960127c7d8d84300e21b7df4eae436803aab
DEVICE=ens3
RATE=500mbit

sudo apt-get -y update
sudo apt-get -y install ocaml libgcrypt20-dev ocaml-findlib build-essential iproute iperf unzip libssl-dev iftop

git clone -b ${OC_BRANCH} https://github.com/samee/obliv-c.git
cd obliv-c; git checkout $OBLIVC_COMMIT; ./configure; make RELEASE=1; cd ../

echo "export OBLIVC_PATH=~/obliv-c" >> ~/.bashrc
chmod +x ~/.bashrc
export OBLIVC_PATH=~/obliv-c

git clone -b ${ACK_BRANCH} https://jackdoerner@bitbucket.org/jackdoerner/absentminded-crypto-kit.git
cd absentminded-crypto-kit; git checkout $ACK_COMMIT; make; mkdir -p tools/bench; cd ../

sudo tc qdisc add dev $DEVICE handle 1: root htb default 11
sudo tc class add dev $DEVICE parent 1: classid 1:1 htb rate $RATE
sudo tc class add dev $DEVICE parent 1:1 classid 1:11 htb rate $RATE

#note: the above must be rerun each time the machine is restarted.
#to clear the above:
#sudo tc qdisc del dev $DEVICE root