
target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES 
    "Interval.hpp"
    "Program.hpp"
    "Rational.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "Interval.cpp"
    "Program.cpp"
    "Rational.cpp"
)
