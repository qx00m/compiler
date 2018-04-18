@echo off

pushd ..\build

rem C4200: nonstandard extension used: zero-sized array in struct/union
rem C4201: nonstandard extension used: nameless struct/union
rem C4204: nonstandard extension used: non-constant aggregate initializer

cl /FC /nologo /Od /Oi /W4 /wd4200 /wd4201 /wd4204 /Zi ..\src\main.c

popd
