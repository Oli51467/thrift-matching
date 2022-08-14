namespace cpp match_service

struct User {
    1: i32 id,
    2: i32 score,
    3: string name
}

// 定义函数
service Match {
    i32 add_user(1: User user, 2: string info),

    i32 remove_user(1: User user, 2: string info),
}


