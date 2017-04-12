cmake_minimum_required (VERSION 2.8)
project (leechcraft_monocle_epub)
include (InitLCPlugin OPTIONAL)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include_directories (
	${CMAKE_CURRENT_BINARY_DIR}
	${Boost_INCLUDE_DIR}
	${LEECHCRAFT_INCLUDE_DIR}
	)
set (SRCS
	epub.cpp
	)

#CreateTrs ("monocle_epub" "en;ru_RU" EPUB_COMPILED_TRANSLATIONS)
#CreateTrsUpTarget("monocle_epub" "en;ru_RU" "${SRCS}" "" "monocleepubsettings.xml")

add_library (leechcraft_monocle_epub SHARED
	${SRCS}
	${EPUB_COMPILED_TRANSLATIONS}
	)
target_link_libraries (leechcraft_monocle_epub
	${LEECHCRAFT_LIBRARIES}
	leechcraft_monocle_util
	)
install (TARGETS leechcraft_monocle_epub DESTINATION ${LC_PLUGINS_DEST})
#install (FILES monoclefxbsettings.xml DESTINATION ${LC_SETTINGS_DEST})
if (UNIX AND NOT APPLE)
	if (USE_QT5)
		install (FILES freedesktop/leechcraft-monocle-epub-qt5.desktop DESTINATION share/applications)
	else ()
		install (FILES freedesktop/leechcraft-monocle-epub.desktop DESTINATION share/applications)
	endif ()
endif ()

FindQtLibs (leechcraft_monocle_epub Xml)
