#ifndef MONSTERDESCRIPTION_H
#define MONSTERDESCRIPTION_H


#include <string>
#include <vector>

class monsterDescription_t {
public:
    std::string NAME;
    std::string DESC;
    std::string COLOR;
    std::vector<char> SPEED;
    std::vector<std::string> ABIL;
    std::vector<char> HP;
    std::vector<char> DAM;
    char SYMB;
    float RRTY;
    int BOSS;

    monsterDescription_t() {
        NAME = "";
        DESC = "";
        COLOR = "";
        SYMB = '\0';
        RRTY = -1;
        BOSS = 0;
    };
    ~monsterDescription_t() {};

    void setNAME(std::string N);
    void setDESC(std::string D);
    void setCOLOR(std::string C);
    void setSPEED(std::vector<char> S);
    void setABIL(std::vector<std::string> A);
    void setHP(std::vector<char> HP);
    void setDAM(std::vector<char> DA);
    void setSYMB(char SYM);
    void setRRTY(float RR);
    void setBoss(int b);
    int isBoss();
    int validMonster();



};

#endif