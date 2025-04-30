#ifndef OBJECTS_H
#define OBJECTS_H

#include <string>
#include <vector>

#include "dice.h"
#include "dims.h"


class Objects {
    public:
        std::string NAME;
        std::string TYPE;
        std::string COLOR;
        int WEIGHT;
        int HIT;
        std::vector<char> DAM;
        int ATTR;
        int VAL;
        int DODGE;
        int DEF;
        int SPEED;
        std::string DESC;
        float RRTY;
        bool ART;
        char SYMB;

        pair_t position;



        Objects(){};

        ~Objects(){
            //delete this;
        };


        void setNAME(std::string N);
        void setCOLOR(std::string C);
        void setTYPE(std::string T);
    
        void setWEIGHT(std::vector<char> W);
        void setHIT(std::vector<char> H);
        void setDAM(std::vector<char> D);
        void setATTR(std::vector<char> A);
        void setVAL(std::vector<char> V);
        void setDODGE(std::vector<char> D);
        void setDEF(std::vector<char> D);
        void setSPEED(std::vector<char> S);
    
    
    
    
        void setDESC(std::string D);
        void setRRTY(float RR);
        void setART(bool A);

        std::string getNAME();

        int getSpeed(){
            return SPEED;
        }

        int getDefense(){
            return DEF;
        }

        



    private:
        // dice_t ATTR;
        // dice_t VAL;
        // dice_t DODGE;
        // dice_t DEF;
        // dice_t SPEED;
        // dice_t HIT;
        // dice_t WEIGHT;
        
        void setSYMB();



};

#endif