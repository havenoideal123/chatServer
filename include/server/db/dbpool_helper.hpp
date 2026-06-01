#ifndef DBPOOL_HELPER_HPP
#define DBPOOL_HELPER_HPP

#include "CommonConnectionPool.h"


inline std::shared_ptr<Connection> getMysqlConn()
{
    auto conn = ConnectionPool::getInstance()->getConnection();

    if (conn == nullptr)
    {
        LOG_INFO << "get mysql connection failed";
    }

    return conn;
}

#endif