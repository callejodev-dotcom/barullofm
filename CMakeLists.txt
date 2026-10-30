cmake_minimum_required(VERSION 3.22)

# En Mac: binario universal (Apple Silicon + Intel), desde macOS 11. En Linux/Windows no hace nada.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS mínimo")
if(NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Arquitecturas de Mac")
endif()

project(BarulloFM VERSION 0.5.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: carpeta hermana ../JUCE, o la que se indique con -DJUCE_DIR=...
if(NOT JUCE_DIR)
  set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
endif()
get_filename_component(JUCE_DIR "${JUCE_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
  add_subdirectory("${JUCE_DIR}" JUCE)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

# Núcleo común de Escoita: a versión fixada ou, se existe, a carpeta irmá ../escoita-nucleo
# (para traballar no núcleo e probalo nos plugins á vez); tamén -DESCOITA_NUCLEO=<carpeta>
set(ESCOITA_NUCLEO_VERSION "v0.5.0")
if(NOT ESCOITA_NUCLEO AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../escoita-nucleo/escoita_nucleo")
  set(ESCOITA_NUCLEO "${CMAKE_CURRENT_SOURCE_DIR}/../escoita-nucleo")
endif()
if(ESCOITA_NUCLEO)
  add_subdirectory("${ESCOITA_NUCLEO}" escoita-nucleo)
else()
  include(FetchContent)
  FetchContent_Declare(escoita_nucleo GIT_REPOSITORY https://github.com/callejodev-dotcom/escoita-nucleo.git
                       GIT_TAG ${ESCOITA_NUCLEO_VERSION} GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(escoita_nucleo)
endif()

# Formatos por sistema: Mac → AU, VST3 y app; Windows → VST3 y app; Linux → VST3, LV2 y app
if(APPLE)
  set(FORMATS AU VST3 Standalone)
elseif(WIN32)
  set(FORMATS VST3 Standalone)
else()
  set(FORMATS VST3 LV2 Standalone)
endif()

juce_add_plugin(BarulloFM
  COMPANY_NAME "Escoita"
  COMPANY_WEBSITE "https://www.escoita.link"
  COMPANY_EMAIL "ola@escoita.link"
  BUNDLE_ID "link.escoita.barullofm"
  PLUGIN_MANUFACTURER_CODE Esco
  PLUGIN_CODE Bf71
  PRODUCT_NAME "Barullo FM"
  DESCRIPTION "Sintetizador FM de 2 operadores emulado rexistro a rexistro"
  FORMATS ${FORMATS}
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  VST3_CATEGORIES Instrument Synth
  AU_MAIN_TYPE kAudioUnitType_MusicDevice
  LV2URI "https://www.escoita.link/barullofm"
  LV2_SHARED_LIBRARY_NAME "BarulloFM"
  COPY_PLUGIN_AFTER_BUILD FALSE)

# tipografía Barlow (licencia OFL, ver Resources/Barlow-OFL.txt)
juce_add_binary_data(BarulloFonts SOURCES
  Resources/BarlowSemiCondensed-Medium.ttf
  Resources/BarlowSemiCondensed-SemiBold.ttf
  Resources/BarlowCondensed-SemiBold.ttf)

target_sources(BarulloFM PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_include_directories(BarulloFM PRIVATE Source)

# Windows: el código está en UTF-8 (tildes, ñ, ×, ·)
if(MSVC)
  target_compile_options(BarulloFM PRIVATE /utf-8 /bigobj)
endif()

target_compile_definitions(BarulloFM PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(BarulloFM
  PRIVATE
    BarulloFonts
    juce::juce_audio_utils
    juce::juce_audio_processors
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
escoita_produto(BarulloFM NOME "Barullo FM" VERSION ${PROJECT_VERSION} REPO callejodev-dotcom/barullofm)

# Prueba sin interfaz: toca unas notas por el procesador y guarda un WAV
option(BARULLO_TESTS "Compilar la prueba de render" OFF)
if(BARULLO_TESTS)
  juce_add_console_app(BarulloRenderTest PRODUCT_NAME "BarulloRenderTest")
  target_sources(BarulloRenderTest PRIVATE tools/RenderTest.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
  target_include_directories(BarulloRenderTest PRIVATE Source)
  if(MSVC)
    target_compile_options(BarulloRenderTest PRIVATE /utf-8 /bigobj)
  endif()
  target_compile_definitions(BarulloRenderTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 BARULLO_NO_PLUGIN_ENTRY=1)
  target_link_libraries(BarulloRenderTest PRIVATE BarulloFonts juce::juce_audio_utils juce::juce_audio_processors juce::juce_audio_formats
    PUBLIC juce::juce_recommended_config_flags)
  escoita_produto(BarulloRenderTest NOME "Barullo FM" VERSION ${PROJECT_VERSION} REPO callejodev-dotcom/barullofm PROBA)

  # Comparación co emulador web: toca un seq.txt co firmware e garda as escrituras ao chip e o
  # audio (o motor usa o pedal e o reparto de voces do núcleo, así que vai con JUCE)
  juce_add_console_app(BarulloSeqTest PRODUCT_NAME "BarulloSeqTest")
  target_sources(BarulloSeqTest PRIVATE tools/seqtest.cpp)
  target_include_directories(BarulloSeqTest PRIVATE Source)
  if(MSVC)
    target_compile_options(BarulloSeqTest PRIVATE /utf-8 /bigobj)
  endif()
  target_compile_definitions(BarulloSeqTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_link_libraries(BarulloSeqTest PRIVATE juce::juce_core PUBLIC juce::juce_recommended_config_flags)
  escoita_produto(BarulloSeqTest NOME "Barullo FM" VERSION ${PROJECT_VERSION} REPO callejodev-dotcom/barullofm PROBA)
endif()
