#include <string>
#include <vector>

#include "objects.h"
#include "dice.h"


std::string Objects::getNAME(){
    return this->NAME;
}

void Objects::setSYMB(){
    if(this->TYPE == "WEAPON"){
        this->SYMB = '|';
    }else if(this->TYPE == "OFFHAND"){
        this->SYMB = ')';
    }else if(this->TYPE == "RANGED"){
        this->SYMB = '}';
    }else if(this->TYPE == "ARMOR"){
        this->SYMB = '[';
    }else if(this->TYPE == "HELMET"){
        this->SYMB = ']';
    }else if(this->TYPE == "CLOAK"){
        this->SYMB = '(';
    }else if(this->TYPE == "GLOVES"){
        this->SYMB = '{';
    }else if(this->TYPE == "BOOTS"){
        this->SYMB = '\\';
    }else if(this->TYPE == "RING"){
        this->SYMB = '=';
    }else if(this->TYPE == "AMULET"){
        this->SYMB = '"';
    }else if(this->TYPE == "LIGHT"){
        this->SYMB = '_';
    }else if(this->TYPE == "SCROLL"){
        this->SYMB = '~';
    }else if(this->TYPE == "BOOK"){
        this->SYMB = '?';
    }else if(this->TYPE == "FLASK"){
        this->SYMB = '!';
    }else if(this->TYPE == "GOLD"){
        this->SYMB = '$';
    }else if(this->TYPE == "AMMUNITION"){
        this->SYMB = '/';
    }else if(this->TYPE == "FOOD"){
        this->SYMB = ',';
    }else if(this->TYPE == "WAND"){
        this->SYMB = '-';
    }else if(this->TYPE == "CONTAINER"){
        this->SYMB = '%';
    }else if(this->TYPE == "STACK"){
        this->SYMB = '&';
    }else{
        this->SYMB = '^';
    }
}

void Objects::setNAME(std::string N){
    this->NAME = N;
}

void Objects::setCOLOR(std::string C){
    this->COLOR = C;
}

void Objects::setTYPE(std::string T){
    this->TYPE = T;
    this->setSYMB();
}

void Objects::setWEIGHT(std::vector<char> W){
    this->WEIGHT = parseDice(W).rollDaDice();
}

void Objects::setHIT(std::vector<char> H){
    this->HIT = parseDice(H).rollDaDice();
}

void Objects::setDAM(std::vector<char> D){
    this->DAM = D;
}

void Objects::setATTR(std::vector<char> A){
    this->ATTR = parseDice(A).rollDaDice();
}

void Objects::setVAL(std::vector<char> V){
    this->VAL = parseDice(V).rollDaDice();
}

void Objects::setDODGE(std::vector<char> D){
    this->DODGE = parseDice(D).rollDaDice();
}

void Objects::setDEF(std::vector<char> D){
    this->DEF = parseDice(D).rollDaDice();
}

void Objects::setSPEED(std::vector<char> S){
    this->SPEED = parseDice(S).rollDaDice();
}

void Objects::setDESC(std::string D){
    this->DESC = D;
}

void Objects::setRRTY(float RR){
    this->RRTY = RR;
}

void Objects::setART(bool A){
    this->ART = A;
}
