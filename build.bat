@echo off
setlocal

:: TODO: allow release builds with a param
SET ScriptPath=%~dp0
SET BuildPath=%ScriptPath%\intermediate\

cmake -S %ScriptPath% -B %BuildPath%
cmake --build %BuildPath%

