#ifndef GROUP_H
#define GROUP_H
#include <string>
#include <vector>
#include "groupuser.hpp"

using namespace std;

//Group表的ORM类
class Group
{
public:
    Group(int id=-1,string name="",string desc="")
    :id(id),name(name),desc(desc){};
    void setId(int id) {this->id = id;}
    int getId() {return this->id;}
    void setName(string name) {this->name = name;}
    string getName() {return this->name;}
    void setDesc(string desc) {this->desc = desc;}
    string getDesc() {return this->desc;}
    vector<GroupUser>& getUsers() {return this->users;}

private:
    int id;
    string name;
    string desc;
    //用户列表
    vector<GroupUser> users;
};


#endif
