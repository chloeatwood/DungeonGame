#ifndef ITEMDESCRIPTION_H
#define ITEMDESCRIPTION_H


#include <string>
#include <vector>

#include "objects.h"


class itemDescription_t {
public:
    std::string NAME;
    std::string TYPE;
    std::string COLOR;
    std::vector<char> WEIGHT;
    std::vector<char> HIT;
    std::vector<char> DAM;
    std::vector<char> ATTR;
    std::vector<char> VAL;
    std::vector<char> DODGE;
    std::vector<char> DEF;
    std::vector<char> SPEED;
    std::string DESC;
    float RRTY;
    bool ART;

    itemDescription_t()
    : NAME(""), TYPE(""), COLOR(""),
      WEIGHT(), HIT(), DAM(), ATTR(), VAL(), DODGE(), DEF(), SPEED(),
      DESC(""), RRTY(-1.0), ART(false) {}


    ~itemDescription_t() {};

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

    std::string getNAME();




    void setDESC(std::string D);
    void setRRTY(float RR);
    void setART(bool A);


    int validItem();



};

#endif