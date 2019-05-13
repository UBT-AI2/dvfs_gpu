from conans import ConanFile

class NumericalMethodsConan(ConanFile):
    name = "numerical_methods"
    version = "1.0"
    settings = {"os": None, "build_type": None, "compiler": None, "arch": None}
    url = "https://github.com/ethz-asl/numerical_methods.git"
    no_copy_source = True
    requires = "eigen/3.3.5@conan/stable", "glog/0.3.5@bincrafters/stable"
    generators = "cmake"

    def source(self):
        self.run("git clone " + self.url)
		
    def package(self):
        self.copy("*.h", "", "numerical_methods")
    
    def package_id(self):
        self.info.header_only()
