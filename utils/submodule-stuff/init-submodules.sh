#!/bin/bash -x

pushd ../..
git submodule update --init --recursive -- miner/ethminer miner/xmr-stak miner/ccminer

pushd miner/ethminer
git apply ../../utils/submodule-stuff/patches/ethminer-patch-tag0.15.0.dev11.patch
popd

pushd miner/xmr-stak
git apply ../../utils/submodule-stuff/patches/xmr-stak-patch-tag2.5.1.patch
popd

pushd miner/ccminer
git apply ../../utils/submodule-stuff/patches/ccminer-patch-tag2.3.patch
popd

popd