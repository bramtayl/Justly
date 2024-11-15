add_subdirectory(interval)
add_subdirectory(rational)

target_sources(JustlyLibrary PRIVATE
    "AbstractRational.cpp"
    "AbstractRational.hpp"
)