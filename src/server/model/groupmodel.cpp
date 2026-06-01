#include "groupmodel.hpp"
#include "dbpool_helper.hpp"


//创建群组
bool GroupModel::createGroup(Group &group)
{
    auto conn = getMysqlConn();
    //创建群组
    char sql[128] = {0};
    string name = conn->escapeString(group.getName());
    string desc = conn->escapeString(group.getDesc());
    sprintf(sql,"insert into AllGroup(groupname,groupdesc) values('%s','%s')",name.c_str(),desc.c_str());

    if(conn){
        if(conn->update(sql))
        {
            group.setId(mysql_insert_id(conn->getConnection()));
            return true;
        }
    }
    return false;
}

//加入群组
void GroupModel::addGroup(int userid,int groupid,string role)
{
    auto conn = getMysqlConn();
    //加入群组
    char sql[128] = {0};
    role = conn->escapeString(role);
    sprintf(sql,"insert into GroupUser values(%d,%d,'%s')",userid,groupid,role.c_str());

    if(conn){
        conn->update(sql);
    }
}

//查询用户所在的群组
vector<Group> GroupModel::queryGroups(int userid)
{
    //查询用户所在的群组
    char sql[128] = {0};
    //先根据userid在groupuser表中查询出该用户所属的群组信息
    //再根据群组信息，查询该组所有用户的userid和user表进行多播联合查询，查出用户的详细信息
    //能一句话写完的不要多次查询，因为sql是很低效的
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from AllGroup a inner join GroupUser b on a.id = b.groupid where b.userid = %d",userid);
    
    vector<Group> groupVec;
    auto conn = getMysqlConn();
    if(conn){
        MYSQL_RES *res = conn->query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            //查出userid所有的群组信息
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    //查询群组的用户信息
    for (Group &group : groupVec)
    {
        sprintf(sql,"select a.id,a.name,a.state from User a inner join GroupUser b on a.id = b.userid where b.groupid = %d",group.getId());
        MYSQL_RES *res = conn->query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

//根据指定的groupid查询群组用户id，除了userid自己，主要用户群聊业务给群组其他成员发送消息
vector<int> GroupModel::queryGroupUsers(int userid,int groupid)
{
    char sql[128] = {0};
    sprintf(sql,"select userid from GroupUser where groupid = %d and userid != %d",groupid,userid);
    vector<int> userVec;
    auto conn = getMysqlConn();
    if(conn){
        MYSQL_RES *res = conn->query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
               userVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return userVec;
}
