cmake_minimum_required(VERSION 3.8)
project(nesEmulator)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp mos6502.cpp mos6502.h
                 Cartucho.cpp Cartucho.h Mapper.cpp
                 Mapper.h Mapper0.cpp Mapper0.h PPU.cpp
                 PPU.h pallete.inc Screen.cpp Screen.h CPU.cpp CPU.h Joypad.cpp Joypad.h APU.cpp APU.h Pulse.cpp Pulse.h Blip_Buffer.h
                 apu_snapshot.h Multi_Buffer.h Nes_Apu.h Nes_Namco.h Nes_Oscs.h Nes_Vrc6.h Sound_Queue.h Nonlinear_Buffer.h
                 Blip_Buffer.cpp apu_snapshot.cpp Multi_Buffer.cpp Nes_Apu.cpp Nes_Namco.h Nes_Oscs.cpp Nes_Vrc6.cpp Sound_Queue.cpp Nonlinear_Buffer.cpp
         Blip_Synth.h blargg_common.h blargg_source.h Nes_Namco.cpp config.hpp static_assert.hpp cstdint.hpp)
add_executable(nesEmulator ${SOURCE_FILES})

set(prefix "C:/Users/Ygor/CLionProjects/nesEmulator/i686-w64-mingw32")
set(exec_prefix "${prefix}")
set(libdir "${exec_prefix}/lib")
set(SDL2_PREFIX "C:/Users/Ygor/CLionProjects/nesEmulator/i686-w64-mingw32")
set(SDL2_EXEC_PREFIX "C:/Users/Ygor/CLionProjects/nesEmulator/i686-w64-mingw32")
set(SDL2_LIBDIR "${exec_prefix}/lib")
set(SDL2_INCLUDE_DIRS "${prefix}/include/SDL2")
set(SDL2_LIBRARIES "-L${SDL2_LIBDIR}  -lmingw32 -lSDL2main -lSDL2  -mwindows")
string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)

include_directories(${SDL2_INCLUDE_DIRS})
link_directories(${SDL2_LIBDIR})

target_link_libraries(nesEmulator ${SDL2_LIBRARIES})


