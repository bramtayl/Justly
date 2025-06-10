cmake_minimum_required(VERSION 3.24)

# on windows, add a path to Qt
list(APPEND CMAKE_PREFIX_PATH C:/Users/brand/6.5.1/msvc2019_64)

project(Justly VERSION 0.8.1 LANGUAGES CXX)

# download https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/MuseScore_General.sf2
# and put it into the share folder
# too big to commit to git

include(CTest)
include(InstallRequiredSystemLibraries)

option(BUILD_TESTS "Build tests" OFF)
option(NO_REALTIME_AUDIO "Do not start realtime audio" OFF)
option(INCLUDE_WHAT_YOU_USE "Run include-what-you-use" OFF)
option(TRACK_COVERAGE "Track coverage" OFF)

find_package(FluidSynth REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Qt6Test REQUIRED)
find_package(Qt6Widgets REQUIRED)

qt_standard_project_setup()

function(qt_deploy executable_name)
    if (WIN32)
        qt_generate_deploy_app_script(TARGET ${executable_name}
            OUTPUT_SCRIPT script_file
        )
        install(SCRIPT ${script_file})
    endif()
endfunction()

function(install_executable executable_name)
    install(TARGETS ${executable_name})
    qt_deploy(${executable_name})
    if (LINUX)
        set_target_properties(${executable_name} PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")  
    endif()
    if (APPLE)
        set_target_properties(${executable_name} PROPERTIES INSTALL_RPATH "@loader_path/../lib")  
    endif()
endfunction()

if (INCLUDE_WHAT_YOU_USE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE 
        include-what-you-use
        -Xiwyu
        --mapping_file=${Justly_SOURCE_DIR}/map.imp
    )
endif()

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic $<$<CONFIG:Debug>:-Og>)
endif()

if (TRACK_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

qt_add_library(JustlyLibrary SHARED)

target_compile_features(JustlyLibrary PRIVATE cxx_std_20)
target_link_libraries(JustlyLibrary PUBLIC Qt6::Core Qt6::Widgets)
target_link_libraries(JustlyLibrary PRIVATE
    FluidSynth::libfluidsynth
    LibXml2::LibXml2
    Qt6::Gui
)

add_compile_definitions(JustlyLibrary PUBLIC $<$<CONFIG:Release>:QT_NO_DEBUG=1>)
target_compile_definitions(JustlyLibrary PRIVATE JUSTLY_LIBRARY)
if (NO_REALTIME_AUDIO)
    target_compile_definitions(JustlyLibrary PRIVATE NO_REALTIME_AUDIO)
endif()

add_subdirectory(include)
target_include_directories(JustlyLibrary PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_sources(JustlyLibrary PRIVATE 
    "src/Cells.hpp"
    "src/ChangeId.hpp"
    "src/Chord.hpp"
    "src/ChordsModel.hpp"
    "src/ChordsTable.hpp"
    "src/ControlsColumn.hpp"
    "src/DeleteCells.hpp"
    "src/EditMenu.hpp"
    "src/FileMenu.hpp"
    "src/InsertMenu.hpp"
    "src/InsertRemoveRows.hpp"
    "src/InsertRow.hpp"
    "src/Interval.hpp"
    "src/IntervalEditor.hpp"
    "src/IntervalRow.hpp"
    "src/MostRecentIterator.hpp"
    "src/MusicXMLChord.hpp"
    "src/MusicXMLNote.hpp"
    "src/MyTable.hpp"
    "src/Note.hpp"
    "src/NotesModel.hpp"
    "src/PartInfo.hpp"
    "src/PasteMenu.hpp"
    "src/PercussionInstrument.hpp"
    "src/PercussionInstrumentEditor.hpp"
    "src/PitchedNote.hpp"
    "src/PitchedNotesModel.hpp"
    "src/PitchedNotesTable.hpp"
    "src/PlayMenu.hpp"
    "src/PlayState.hpp"
    "src/Player.hpp"
    "src/Program.hpp"
    "src/ProgramEditor.hpp"
    "src/Rational.hpp"
    "src/RationalEditor.hpp"
    "src/ReplaceTable.hpp"
    "src/Row.hpp"
    "src/RowType.hpp"
    "src/RowsModel.hpp"
    "src/SetCell.hpp"
    "src/SetCells.hpp"
    "src/SetDouble.hpp"
    "src/Song.hpp"
    "src/SongMenuBar.hpp"
    "src/SongMenuBar.cpp"
    "src/SongWidget.hpp"
    "src/SongWidget.cpp"
    "src/SpinBoxes.hpp"
    "src/SwitchColumn.hpp"
    "src/TimeIterator.hpp"
    "src/UndoRowsModel.hpp"
    "src/UnpitchedNote.hpp"
    "src/UnpitchedNotesModel.hpp"
    "src/UnpitchedNotesTable.hpp"
    "src/ViewMenu.hpp"
    "src/XMLDocument.hpp"
    "src/XMLValidator.hpp"
    "src/constants.hpp"
    "src/helpers.hpp"
    "src/justly.cpp"
)

install(TARGETS JustlyLibrary
    RUNTIME_DEPENDENCY_SET runtime_dependencies
    FILE_SET justly_public_headers
)
if (WIN32)
    install(RUNTIME_DEPENDENCY_SET runtime_dependencies
        PRE_INCLUDE_REGEXES
            glib
            iconv
            intl
            fluidsynth
            lzma
            mp3lame
            mpg123
            ogg
            opus
            pcre
            sndfile
            xml2
            vorbis
            vorbisenc
            zlib
        PRE_EXCLUDE_REGEXES .
        DIRECTORIES
            ${_VCPKG_INSTALLED_DIR}/{VCPKG_TARGET_TRIPLET}/lib
    )
endif()
qt_deploy(JustlyLibrary)
install(DIRECTORY ${Justly_SOURCE_DIR}/share/ TYPE DATA)

qt_add_executable(Justly)
target_sources(Justly PRIVATE bin/main.cpp)
target_link_libraries(Justly PRIVATE JustlyLibrary Qt6::Core Qt6::Widgets)
if (WIN32)
    set_target_properties(Justly PROPERTIES WIN32_EXECUTABLE ON)
endif()
install_executable(Justly)

if (BUILD_TESTS)
    target_compile_definitions(JustlyLibrary PUBLIC TEST_HOOKS)

    qt_add_executable(JustlyTests)
    target_sources(JustlyTests PRIVATE tests/test.cpp)
    target_include_directories(JustlyTests PRIVATE ${Justly_SOURCE_DIR})
    target_link_libraries(JustlyTests PRIVATE
        JustlyLibrary
        Qt6::Core
        Qt6::Gui
        Qt6::Test
        Qt6::Widgets
    )
    install_executable(JustlyTests)
    install(DIRECTORY ${Justly_SOURCE_DIR}/tests/share/ TYPE DATA)
    set(test_install_folder ${Justly_SOURCE_DIR}/test_install)
    add_custom_target(install_tests ALL
        COMMAND cmake --install ${CMAKE_CURRENT_BINARY_DIR}
        --prefix ${test_install_folder}
        --config $<CONFIG>
    )
    add_dependencies(install_tests JustlyLibrary Justly JustlyTests)
    add_test(NAME run_tests COMMAND ${test_install_folder}/bin/JustlyTests)
endif()

include(CPack)