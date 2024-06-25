@echo off
setlocal

SET ScriptPath=%~dp0
SET BuildPath=%ScriptPath%\intermediate\

cmake -S %ScriptPath% -B %BuildPath%
cmake --build %BuildPath%

