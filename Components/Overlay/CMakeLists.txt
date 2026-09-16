#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

############################################################
# Overlay optional component
############################################################

# define header and source files for the library
file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
list(APPEND HEADER_FILES 
  ${PROJECT_BINARY_DIR}/include/OgreOverlayPrerequisites.h
  ${CMAKE_CURRENT_SOURCE_DIR}/include/OgreOverlay.i)
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

if(OGRE_BUILD_COMPONENT_OVERLAY_IMGUI)
  # Link the prebuilt ImGuiCore (shared with FatmapMobileSDK's WebGpu render
  # backend) instead of compiling our own copy -- see build_dependencies/imgui.
  find_library(IMGUICORE_LIBRARY ImGuiCore)
  find_path(IMGUICORE_INCLUDE_DIR imgui.h PATH_SUFFIXES imgui)
  if(NOT IMGUICORE_LIBRARY OR NOT IMGUICORE_INCLUDE_DIR)
    message(FATAL_ERROR "OGRE_BUILD_COMPONENT_OVERLAY_IMGUI requires the prebuilt ImGuiCore dependency")
  endif()
  if(FREETYPE_FOUND)
    # FreeType-based font rasterizer, opted into at runtime (OgreImGuiOverlay.cpp
    # calls SetFontLoader) rather than via IMGUI_ENABLE_FREETYPE, which would
    # require its own separately-compiled ImGuiCore -- see build_dependencies/imgui.
    find_library(IMGUIFREETYPE_LIBRARY ImGuiFreetypeBackend REQUIRED)
  endif()
else()
    list(REMOVE_ITEM SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/OgreImGuiOverlay.cpp")
endif()

# setup target
add_library(OgreOverlay ${OGRE_COMP_LIB_TYPE} ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(OgreOverlay PROPERTIES VERSION ${OGRE_SOVERSION} SOVERSION ${OGRE_SOVERSION})
target_link_libraries(OgreOverlay PUBLIC OgreMain)
target_include_directories(OgreOverlay PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  $<INSTALL_INTERFACE:include/OGRE/Overlay>)

if(FREETYPE_FOUND)
  target_compile_definitions(OgreOverlay PRIVATE HAVE_FREETYPE)
  target_link_libraries(OgreOverlay PRIVATE ${FREETYPE_LIBRARIES})
  target_include_directories(OgreOverlay PRIVATE ${FREETYPE_INCLUDE_DIRS})
  if(ZLIB_FOUND)
    target_link_libraries(OgreOverlay PRIVATE ZLIB::ZLIB)
  endif()
elseif(UNIX)
  set_source_files_properties(src/OgreFont.cpp PROPERTIES
    COMPILE_FLAGS "-Wno-cast-qual -Wno-unused-function")
endif()

if(OGRE_BUILD_COMPONENT_OVERLAY_IMGUI)
  target_include_directories(OgreOverlay PUBLIC "$<BUILD_INTERFACE:${IMGUICORE_INCLUDE_DIR}>")
  target_link_libraries(OgreOverlay PUBLIC ${IMGUICORE_LIBRARY})

  if(FREETYPE_FOUND)
    target_link_libraries(OgreOverlay PRIVATE ${IMGUIFREETYPE_LIBRARY})
  endif()

  if(MINGW)
    target_link_libraries(OgreOverlay PRIVATE imm32)
  endif()

  list(APPEND HEADER_FILES
    ${IMGUICORE_INCLUDE_DIR}/imgui.h
    ${IMGUICORE_INCLUDE_DIR}/imconfig.h)
  if(FREETYPE_FOUND)
    list(APPEND HEADER_FILES ${IMGUICORE_INCLUDE_DIR}/imgui_freetype.h)
  endif()
endif()

generate_export_header(OgreOverlay 
    EXPORT_MACRO_NAME _OgreOverlayExport
    EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/OgreOverlayPrerequisites.h)

# install 
ogre_config_framework(OgreOverlay)
ogre_config_component(OgreOverlay)

install(FILES ${HEADER_FILES}
  DESTINATION include/OGRE/Overlay
)
install(FILES "${PROJECT_SOURCE_DIR}/Media/packs/profiler.zip"
  DESTINATION "${OGRE_MEDIA_PATH}/packs/"
)
