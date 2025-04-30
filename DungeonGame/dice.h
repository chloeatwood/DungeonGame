#ifndef DICE_H
#define DICE_H

#include <vector>


class dice_t {
    public:
        int base;
        int dice;
        int sides;



        //Constructor
        dice_t(int b, int d, int s){
            this->base = b;
            this->dice = d;
            this->sides = s;
        }

        //Destructor
        ~dice_t() {}

        //Function that rools the dice 
        int rollDaDice();

        void setBase(int b){
            this->base = b;
        };
        void setDice(int d){
            this->dice = d;
        };
        void setSides(int s){
            this->sides = s;
        };
        
};

dice_t parseDice(const std::vector<char>& vec);




#endif