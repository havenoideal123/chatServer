#ifndef GROUPUSER_H
#define GROUPUSER_H
#include "user.hpp"



using namespace std;
//GroupUser表的ORM类
class GroupUser:public User
{
public:
    void setRole(string role) {this->role = role;}
    string getRole() {return this->role;}
private:
    string role;
};
#endif