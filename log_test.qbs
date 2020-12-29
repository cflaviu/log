import qbs

CppApplication {
    consoleApplication: true
    cpp.cxxLanguageVersion: "c++17"
    cpp.staticLibraries: ["boost_log", "boost_log_setup", "pthread", "boost_thread"]
    files: [
        "log.cpp",
        "log.h",
        "main.cpp",
        "this_thread_id.hpp",
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}
