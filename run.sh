#!/bin/bash

rm -f rays

g++ -std=c++14 -I/opt/homebrew/include -L/opt/homebrew/lib src/main.cpp -lsfml-graphics -lsfml-window -lsfml-system -lpng -o rays

# Check if compilation was successful
if [ $? -eq 0 ]; then
    ./rays
else
    echo "Compilation failed."
fi
