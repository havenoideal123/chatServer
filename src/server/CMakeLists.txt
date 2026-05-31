#定义一个SRC_LIST变量，用于存储需要编译的源文件列表
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)



#指定生成的可执行文件的名称
add_executable(server ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

#指定链接时需要的依赖文件
target_link_libraries(server muduo_net muduo_base mysqlclient hiredis pthread)
