from conan import ConanFile
from conan.tools.files import copy
import os
import glob

class fmodRecipe(ConanFile):
    name = "fmod"
    version = "2.03.12"
    settings = "os", "arch", "compiler", "build_type"

    options = {"shared": [True]}
    default_options = {"shared": True}

    def package(self):
        sdk = os.environ.get("FMOD_SDK_DIR")
        if not sdk:
            raise Exception("FMOD_SDK_DIR is not set")

        inc = os.path.join(sdk, "api", "core", "inc")
        lib_root = os.path.join(sdk, "api", "core", "lib")

        copy(self, "*.h", src=inc, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.hpp", src=inc, dst=os.path.join(self.package_folder, "include"))
        copy(self, "*.inl", src=inc, dst=os.path.join(self.package_folder, "include"))
        
        if str(self.settings.os) != "Windows":
            raise Exception("Unsupported OS in this recipe")

        arch_folder = "x64" if str(self.settings.arch) == "x86_64" else "x86"
        lib_dir = os.path.join(lib_root, arch_folder)

        copy(self, "*.lib", src=lib_dir, dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, "*.dll", src=lib_dir, dst=os.path.join(self.package_folder, "bin"), keep_path=False)

        libs = [os.path.splitext(os.path.basename(p))[0] for p in glob.glob(os.path.join(lib_dir, "*.lib"))]
        if not libs:
            raise Exception(f"No .lib files found in {lib_dir}")
        self.conf_info.define("fmod:detected_libs", ";".join(sorted(set(libs))))

    def package_info(self):
        lib_dir = os.path.join(self.package_folder, "lib")
        libs = [os.path.splitext(f)[0] for f in os.listdir(lib_dir) if f.lower().endswith(".lib")]

        preferred = None
        for candidate in ["fmod_vc", "fmod", "fmodL_vc", "fmodL"]:
            if candidate in libs:
                preferred = candidate
                break

        if not preferred:
            preferred = libs[0]

        self.cpp_info.libs = [preferred]
        self.cpp_info.includedirs = ["include"]

        self.cpp_info.set_property("cmake_file_name", "fmod")
        self.cpp_info.set_property("cmake_target_name", "fmod::fmod")