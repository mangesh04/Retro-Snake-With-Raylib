command:
	g++ -c listener.cpp -o listener
	"C:\raylib\w64devkit\bin\g++.exe" main.cpp snake.cpp desktop_buddy.cpp pomodoro.cpp pong.cpp breakout.cpp listener -o main.exe -I"C:\raylib\w64devkit\x86_64-w64-mingw32\include" -L"C:\raylib\w64devkit\x86_64-w64-mingw32\lib"  -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows

debug:
	g++ -c listener.cpp -o listener
	"C:\raylib\w64devkit\bin\g++.exe" main.cpp snake.cpp desktop_buddy.cpp pomodoro.cpp pong.cpp breakout.cpp listener -o main.exe -L"C:\raylib\w64devkit\x86_64-w64-mingw32\lib" -I"C:\raylib\w64devkit\x86_64-w64-mingw32\include" -lraylib -lopengl32 -lgdi32 -lwinmm
	main.exe