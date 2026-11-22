@echo off
rem RONE release-check: what is missing, per plugin, for "it is out everywhere". See release-check.mjs.
"D:\RONE PLUGINS\tools\node-v24.20.0-win-x64\node.exe" "%~dp0release-check.mjs" %*
