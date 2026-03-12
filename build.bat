@echo off
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" StereovisionHacks.sln /p:Configuration=Release /p:Platform=x64 /t:DirectX11 /m /v:minimal
