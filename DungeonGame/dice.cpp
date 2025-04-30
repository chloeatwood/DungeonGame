#include "dice.h"

#include <stdlib.h>
#include <string>


//TODO: Implement at some point later on when dice is needed
// roll two 6 sided dice and add 12 to it -> 12+2d6
int dice_t::rollDaDice(){


    int i;

    int total = 0;

    for(i = 0; i < this->dice; i++){
        int randNum = rand() % (this->sides - 1 + 1) + 1;
        total = total + randNum;
    }

    total = total + this->base;

    return total;
}


dice_t parseDice(const std::vector<char>& vec){
    std::string input(vec.begin(), vec.end());

    int base = 0, dice = 0, sides = 0;
    size_t plusPos = input.find('+');
    size_t dPos = input.find('d');

    if (plusPos != std::string::npos && dPos != std::string::npos && dPos > plusPos) {
        base = std::stoi(input.substr(0, plusPos));
        dice = std::stoi(input.substr(plusPos + 1, dPos - plusPos - 1));
        sides = std::stoi(input.substr(dPos + 1));
    }

    return dice_t(base, dice, sides); 
}