#include "friendmodel.hpp"
#include "dbpool_helper.hpp"


void FriendModel::insert(int userid,int friendid)
{
    //添加好友
    char sql[128] = {0};
    sprintf(sql,"insert into Friend(userid,friendid) values(%d,%d)",userid,friendid);
    auto conn = getMysqlConn();
    if(conn){
        conn->update(sql);
    }
}

vector<User> FriendModel::query(int userid)
{
    //返回用户的好友列表
    vector<User> friends;
    char sql[128] = {0};
    sprintf(sql,"select a.id,a.name,a.state from User a inner join Friend b on a.id = b.friendid where b.userid = %d",userid);
    vector<User> res;
    auto conn = getMysqlConn();
    if(conn){
        MYSQL_RES *res = conn->query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                friends.push_back(user);
            }
            mysql_free_result(res);
            return friends;
        }
    }
    return friends;
}