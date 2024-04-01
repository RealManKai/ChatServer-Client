# 定义了一个SRC_LIST变量，包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)
aux_source_directory(./connectionpool CONPOOL_LIST)

# 指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST} ${CONPOOL_LIST})

# 指定可执行文件链接时需要依赖的库文件
find_library(MYSQLCLIENT_LIB mysqlclient PATHS /usr/lib64/mysql)

target_link_libraries(ChatServer muduo_net muduo_base ${MYSQLCLIENT_LIB} hiredis pthread)
