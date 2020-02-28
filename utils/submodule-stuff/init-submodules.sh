#!/bin/bash -x

pushd ../..
git submodule update --init --recursive -- miner/ethminer miner/xmr-stak miner/ccminer

pushd miner/ethminer
git apply ../../utils/submodule-stuff/patches/ethminer-patch-tag0.18.0.patch
popd

pushd miner/xmr-stak
git apply ../../utils/submodule-stuff/patches/xmr-stak-patch-tag1.0.4-rx.patch
popd

pushd miner/ccminer
git apply ../../utils/submodule-stuff/patches/ccminer-patch-tag2.3.1.patch
popd

popd
