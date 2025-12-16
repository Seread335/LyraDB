from conan import ConanFile
from conan.tools.files import copy, save
from conan.tools.cmake import CMake, cmake_layout
import os


class LyraDBFormatsConan(ConanFile):
    name = "lyradb_formats"
    version = "1.0.0"
    
    # Metadata
    author = "LyraDB Team"
    description = "Production-grade file format library for LyraDB with 3 custom formats: .lyradb (database), .lyradbite (iterator), .lyra (archive)"
    homepage = "https://github.com/lyradb/lyradb"
    url = "https://github.com/lyradb/lyradb"
    topics = ("database", "file-formats", "c++17", "serialization")
    license = "MIT"
    
    # Package settings
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    defaults = {
        "shared": False,
        "fPIC": True,
    }
    
    # Source
    exports_sources = (
        "include/lyradb/*",
        "src/formats/*",
        "CMakeLists_formats.txt",
    )
    
    # Build requirements
    requires = ()
    build_requires = ()
    
    generators = "CMakeToolchain", "CMakeDeps"

    def layout(self):
        cmake_layout(self)

    def build(self):
        """Build the library using CMake"""
        cmake = CMake(self)
        cmake.configure(build_folder=self.build_folder)
        cmake.build()

    def package(self):
        """Package the built library"""
        # Copy headers
        copy(
            self,
            "*.h",
            src=os.path.join(self.source_folder, "include/lyradb"),
            dst=os.path.join(self.package_folder, "include/lyradb"),
            keep_path=False,
        )
        
        # Copy library files
        copy(
            self,
            "*.lib",
            src=self.build_folder,
            dst=os.path.join(self.package_folder, "lib"),
            keep_path=False,
        )
        copy(
            self,
            "*.a",
            src=self.build_folder,
            dst=os.path.join(self.package_folder, "lib"),
            keep_path=False,
        )
        copy(
            self,
            "*.so",
            src=self.build_folder,
            dst=os.path.join(self.package_folder, "lib"),
            keep_path=False,
        )
        copy(
            self,
            "*.dylib",
            src=self.build_folder,
            dst=os.path.join(self.package_folder, "lib"),
            keep_path=False,
        )

    def package_info(self):
        """Define package information for consumers"""
        self.cpp_info.libs = ["lyradb_formats"]
        
        # Set include directory
        self.cpp_info.includedirs = ["include"]
        
        # Set lib directory
        self.cpp_info.libdirs = ["lib"]
        
        # C++ standard requirements
        self.cpp_info.cppflags = ["/std:c++17"] if self.settings.os == "Windows" else ["-std=c++17"]
        
        # Define feature macros
        self.cpp_info.defines = [
            "LYRADB_FORMATS_LIBRARY=1",
            "LYRADB_ENABLE_FORMAT_VALIDATION=1",
        ]
