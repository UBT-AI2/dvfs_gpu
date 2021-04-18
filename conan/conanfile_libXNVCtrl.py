from conans import ConanFile, AutoToolsBuildEnvironment, tools
from conans.errors import ConanInvalidConfiguration

class libXNVCtrlConan(ConanFile):
    name = "libXNVCtrl"
    version = "1.1"
    settings = {"os": None, "build_type": None, "compiler": None, "arch": None}
    url = "https://github.com/NVIDIA/nvidia-settings.git"
    generators = "cmake"

    def configure(self):
        if self.settings.os != "Linux":
            raise ConanInvalidConfiguration("This library is not compatible with os: " + str(self.settings.os))
	  
    def source(self):
        self.run("git clone " + self.url)
		
    def build(self):
        with tools.chdir("nvidia-settings/src/libXNVCtrl"):
            env_build = AutoToolsBuildEnvironment(self)
            env_build.make()
		
    def package(self):
        self.copy("*.h", "include/XNVCtrl", "nvidia-settings/src/libXNVCtrl", keep_path=False)
        self.copy("*.a", "lib", "nvidia-settings/src/libXNVCtrl", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["XNVCtrl", "X11", "Xext"]
