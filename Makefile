command:
	g++ -c listener.cpp -o listener
	g++ main.cpp listener -o main.exe -L"C:\raylib\w64devkit\x86_64-w64-mingw32\lib" -I"C:\raylib\w64devkit\x86_64-w64-mingw32\include" -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
