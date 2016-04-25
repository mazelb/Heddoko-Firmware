cd /d "C:\Working Directory\Heddoko-src\Testing\PacketTester\PacketTester" &msbuild "PacketTester.csproj" /t:sdvViewer /p:configuration="Debug" /p:platform=Any CPU
exit %errorlevel% 