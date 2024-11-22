cmake_minimum_required(VERSION 3.14...3.27)

project(
  doxy
  VERSION 1.0.0
  LANGUAGES CXX)

find_package(Doxygen REQUIRED)

# `doxygen -w html header.html delete_me.html delete_me.css`
set(DOXYGEN_HTML_HEADER "${CMAKE_CURRENT_LIST_DIR}/header.html")
set(DOXYGEN_HTML_EXTRA_FILES
    "${CMAKE_CURRENT_LIST_DIR}/doxygen-awesome-darkmode-toggle.js")
set(DOXYGEN_HTML_EXTRA_STYLESHEET
    "${CMAKE_CURRENT_LIST_DIR}/doxygen-awesome.css")
set(DOXYGEN_DISABLE_INDEX "NO")
set(DOXYGEN_FULL_SIDEBAR "NO")
set(HTML_COLORSTYLE "LIGHT")
set(DOXYGEN_GENERATE_TREEVIEW "YES")
set(DOXYGEN_TREEVIEW_WIDTH "335")
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "index.md")

set(DOXYGEN_HTML_OUTPUT "docs")

doxygen_add_docs(doxygen)
