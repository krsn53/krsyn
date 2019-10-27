
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRCS
    src-library/Options.cpp
    src-library/Binasc.cpp
    src-library/MidiEvent.cpp
    src-library/MidiEventList.cpp
    src-library/MidiFile.cpp
    src-library/MidiMessage.cpp
)

set(HDRS
    include/Binasc.h
    include/MidiEvent.h
    include/MidiEventList.h
    include/MidiFile.h
    include/MidiMessage.h
    include/Options.h
)

add_library(midifile STATIC ${SRCS} ${HDRS})
