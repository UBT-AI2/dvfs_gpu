from conans import ConanFile, CMake
from conans.errors import ConanInvalidConfiguration

class MiningConan(ConanFile):
   name = "mining"
   version = "1.0"
   exports_sources = "../CMakeLists.txt", "../src*", "../configs*", "../miner*", "../scripts*", "../conan*"
   settings = {"os": None, "build_type": None, "compiler": None, "arch": None}
   generators = "cmake"
   default_options = {"boost:header_only": True, "libcurl:shared": True, "glog:shared" : True, "glog:with_gflags" : False}
   
   def configure(self):
      if self.settings.arch != "x86_64":
         raise ConanInvalidConfiguration("This package is not compatible with arch: " + str(self.settings.arch))
      if self.settings.os != "Linux" and self.settings.os != "Windows":
         raise ConanInvalidConfiguration("This package is not compatible with os: " + str(self.settings.os))

   def requirements(self):
      self.requires.add("boost/1.69.0@conan/stable")
      self.requires.add("eigen/3.3.5@conan/stable")
      self.requires.add("glog/0.3.5@bincrafters/stable")
      self.requires.add("libcurl/7.64.1@bincrafters/stable")
      self.requires.add("numerical_methods/1.0@bincrafters/stable")
      if self.settings.os != "Windows":
         self.requires.add("libXNVCtrl/1.0@bincrafters/stable")
	   
   def imports(self):
      self.copy("*.dll", dst="bin", src="bin")
      self.copy("*.so*", dst="bin", src="lib")

   def build(self):
      cmake = CMake(self)
      cmake.configure()
      cmake.build()
	  
   def package(self):
      cmake = CMake(self)
      cmake.configure()
      cmake.install()
   
   def package_info(self):
      self.cpp_info.includedirs = ['include']
      self.cpp_info.libdirs = ['lib']
      self.cpp_info.bindirs = ['bin']
      self.cpp_info.resdirs = ['scripts', 'miner', 'configs']
      self.cpp_info.libs = ['freq_core_lib', 'freq_exhaustive_lib', 'freq_optimization_lib', 
      'nvapiOC.lib', 'nvmlOC_lib', 'profit_optimization_lib']

	  