#include "monsterDescription.h"



void monsterDescription_t::setNAME(std::string N){
    this->NAME = N;
}

void monsterDescription_t::setDESC(std::string D){
    this->DESC = D;
}

void monsterDescription_t::setCOLOR(std::string C){
    this->COLOR = C;
}

void monsterDescription_t::setSPEED(std::vector<char> S){
    this->SPEED = S;
}

void monsterDescription_t::setABIL(std::vector<std::string> A){
    this->ABIL = A;
}

void monsterDescription_t::setHP(std::vector<char> H){
    this->HP = H;
}

void monsterDescription_t::setDAM(std::vector<char> DA){
    this->DAM = DA;
}

void monsterDescription_t::setSYMB(char SYM){
    this->SYMB = SYM;
}

void monsterDescription_t::setRRTY(float RR){
    this->RRTY = RR;
}

void monsterDescription_t::setBoss(int b){
    this->BOSS = b;
}

int monsterDescription_t::isBoss(){
    return 1;
}

int monsterDescription_t::validMonster(){
    if(this->NAME != "" &&  this->DESC != "" && this->COLOR != "" && this->SYMB != '\0' 
        && this->RRTY != -1 && !this->SPEED.empty() && !this->ABIL.empty() && !this->HP.empty() && !this->DAM.empty()){
        //This is a valid monsters
        return 1;
    }
    //Not a valid monster
    return 0;
}