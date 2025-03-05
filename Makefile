RAYLIB_FLAGS =  -lraylib -lgdi32 -lwinmm
PROJ_NAME = 20g_plank.exe

default:
	gcc -Wall -Wextra -std=c99 main.c $(RAYLIB_FLAGS) -o $(PROJ_NAME)

run:
	./$(PROJ_NAME)
