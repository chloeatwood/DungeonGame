#include <string>
#include <vector>

#include "itemDescription.h"
#include "dungeon.h"

std::string itemDescription_t::getNAME(){
    return this->NAME;
}

void itemDescription_t::setNAME(std::string N){
    this->NAME = N;
}

void itemDescription_t::setCOLOR(std::string C){
    this->COLOR = C;
}

void itemDescription_t::setTYPE(std::string T){
    this->TYPE = T;
}

void itemDescription_t::setWEIGHT(std::vector<char> W){
    this->WEIGHT = W;
}

void itemDescription_t::setHIT(std::vector<char> H){
    this->HIT = H;
}

void itemDescription_t::setDAM(std::vector<char> D){
    this->DAM = D;
}

void itemDescription_t::setATTR(std::vector<char> A){
    this->ATTR = A;
}

void itemDescription_t::setVAL(std::vector<char> V){
    this->VAL = V;
}

void itemDescription_t::setDODGE(std::vector<char> D){
    this->DODGE = D;
}

void itemDescription_t::setDEF(std::vector<char> D){
    this->DEF = D;
}

void itemDescription_t::setSPEED(std::vector<char> S){
    this->SPEED = S;
}

void itemDescription_t::setDESC(std::string D){
    this->DESC = D;
}

void itemDescription_t::setRRTY(float RR){
    this->RRTY = RR;
}

void itemDescription_t::setART(bool A){
    this->ART = A;
}

int itemDescription_t::validItem(){

    if(NAME != "" && TYPE != "" && COLOR != "" && RRTY != -1 && !WEIGHT.empty() && !HIT.empty() 
    && !DAM.empty() && !ATTR.empty() && !VAL.empty() && !DODGE.empty() && !DEF.empty() && !SPEED.empty() 
    && DESC != "" && (ART == true || ART == false)){
        return 1;
    }
    return 0;
}

