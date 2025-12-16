from conan import ConanFile
from conan.tools.files import copy
import os


class LyraDBFormatsConan(ConanFile):
    name = "lyradb_formats"
    version = "1.0.0"
    
    # Metadata
    author = "LyraDB Team"
    description = "Production-grade file format library for LyraDB with 3 custom formats: .lyradb, .lyradbite, .lyra"
    homepage = "https://github.com/Seread335/LyraDB"
    url = "https://github.com/Seread335/LyraDB"
    topics = ("database", "file-formats", "c++17", "serialization")
    license = "MIT"
    
    # Package settings
    settings = "os", "compiler", "build_type", "arch"
    
    # Source files - just headers and source for header-only or to recompile
    exports_sources = (
        "include/lyradb/lyradb_formats.h",
        "src/formats/lyradb_formats.cpp",
    )
    
    def package(self):
        """Package headers and source"""
        # Copy headers
        copy(
            self,
            "lyradb_formats.h",
            src=os.path.join(self.source_folder, "include/lyradb"),
            dst=os.path.join(self.package_folder, "include/lyradb"),
            keep_path=False,
        )
        
        # Copy source for compilation if needed
        copy(
            self,
            "*.cpp",
            src=os.path.join(self.source_folder, "src/formats"),
            dst=os.path.join(self.package_folder, "src"),
            keep_path=False,
        )

    def package_info(self):
        """Define package information for consumers"""
        # Set include directory
        self.cpp_info.includedirs = ["include"]
        
        # Set source directory for header-only-like usage
        self.cpp_info.srcdirs = ["src"]
        
        # C++ standard requirements
        if self.settings.os == "Windows":
            self.cpp_info.cppflags = ["/std:c++17"]
        else:
            self.cpp_info.cppflags = ["-std=c++17"]
        
        # Define feature macros
        self.cpp_info.defines = [
            "LYRADB_FORMATS_LIBRARY=1",
            "LYRADB_ENABLE_FORMAT_VALIDATION=1",
        ]
        
        # Provide info message
        self.output.info("LyraDB Formats Library 1.0.0 is ready to use!")
        self.output.info("Include path: " + os.path.join(self.package_folder, "include"))
        self.output.info("Source path: " + os.path.join(self.package_folder, "src"))
