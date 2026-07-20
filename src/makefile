teapot_sim : main.o
	g++ main.o line_maker.o -o teapot_sim -lsfml-graphics -lsfml-window -lsfml-system

main.o : main.cpp line_maker/item_loader.hpp line_maker/line_maker.hh line_maker/renderer.hpp line_maker.o
	g++ -c main.cpp
line_maker.o : line_maker/line_maker.cc
	g++ -c line_maker/line_make.cc
