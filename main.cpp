#include <iostream>
#include "Cartucho.h"
#include "Screen.h"

using namespace std;

int main(int argc, char **argv)
{

    Cartucho::load("C:\\Users\\Ygor\\CLionProjects\\nesEmulator\\Super Mario.nes");

    Screen::initScreen();

    Screen::run();

    return 0;
}
