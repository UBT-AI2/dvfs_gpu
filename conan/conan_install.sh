#!/bin/bash

: ${1?"Illegal number of parameters. Usage $0 <conanfile_dir>."}

conanfile_dir=$1

if [[ "$OSTYPE" != "msys" ]]
then
	conan create ${conanfile_dir}/conanfile_libXNVCtrl.py libXNVCtrl/1.0@bincrafters/stable
fi

conan create ${conanfile_dir}/conanfile_numerical_methods.py numerical_methods/1.0@bincrafters/stable
conan install ${conanfile_dir} --build=missing