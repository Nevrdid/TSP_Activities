#!/usr/bin/env bash

sudo bash -c '
systemctl start docker
toolchain_image="$(docker image ls | grep trimui-smart-pro-toolchain)"

if [ -z "$toolchain_image" ]; then
  git clone https://github.com/s0ckz/trimui-smart-pro-toolchain.git
  cd trimui-smart-pro-toolchain
  git lfs pull
  make .build
  cd ..
fi

cd ..

docker run -v "$PWD/.:/root/workspace" -it trimui-smart-pro-toolchain bash -c "cd build && make GOAL=cross_tsp"
'
