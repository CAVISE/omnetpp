@echo off

: nedxml test: parse then regenerate .ned files
: note: this must be run with and without -e flag (expr parsing)

: FIXME turn into automated test!

mkdir work 2>nul
del /q work\*.* >nul

C:\home\omnetpp40\omnetpp\src\nedxml\tmp_nedtool.exe -V -n -Q -s .nd_out *.ned
move *.nd_out work >nul
ren work\*.nd_out *.ned

C:\home\omnetpp40\omnetpp\src\nedxml\tmp_nedtool.exe -x -s .xml_out *.ned
move *.xml_out work >nul
ren work\*.xml_out *.xml
