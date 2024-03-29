// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "match_server/Match.h"
#include "save_client/Save.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace  ::match_service;
using namespace  ::save_service;
using namespace std;

struct Task
{
    User user;
    string type;
};

struct MessageQueue
{
    queue<Task> q;
    mutex m;
    condition_variable cv;
}messageQueue;

class Pool
{
    public:
        void saveResult(int a, int b)
        {
            printf("Match result: %d %d\n", a, b);
            std::shared_ptr<TTransport> socket(new TSocket("123.57.47.211", 9090));
            std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            SaveClient client(protocol);

            try {
                transport->open();

                client.save_data("acs_4898", "2a3c1a65", a, b);
                transport->close();
            } catch (TException& tx) {
                cout << "ERROR: " << tx.what() << endl;
            }
        }

        void match()
        {
            while(users.size() > 1)
            {
                auto a = users[0], b = users[1];
                users.erase(users.begin());
                users.erase(users.begin());
                saveResult(a.id, b.id);
            }
        }

        void add(User user)
        {
            users.push_back(user);
        }

        void remove(User user)
        {
            for (uint32_t i = 0; i < users.size(); i ++ )
            {
                if (users[i].id == user.id)
                {
                    users.erase(users.begin() + i);
                }
            }
        }
    private:
        vector<User> users;

}pool;

class MatchHandler : virtual public MatchIf {
    public:
        MatchHandler() {
            // Your initialization goes here
        }

        int32_t add_user(const User& user, const std::string& info) {
            // Your implementation goes here
            printf("add_user\n");
            // 加锁 不需要显式解锁
            unique_lock<mutex> lck(messageQueue.m);
            messageQueue.q.push({user, "add"});
            messageQueue.cv.notify_all(); // 唤醒所有阻塞的条件变量 其中随机一个会执行

            return 0;
        }

        int32_t remove_user(const User& user, const std::string& info) {
            // Your implementation goes here
            printf("remove_user\n");

            unique_lock<mutex> lck(messageQueue.m);
            messageQueue.q.push({user, "remove"});
            messageQueue.cv.notify_all();

            return 0;
        }

};

void consumeTask()
{
    while(true)
    {
        unique_lock<mutex> lck(messageQueue.m);
        if (messageQueue.q.empty())
        {
            // 条件变量控制让权等待 防止无休止循环
            messageQueue.cv.wait(lck);
        }
        else
        {
            auto task = messageQueue.q.front();
            messageQueue.q.pop();
            lck.unlock();   // 必须解锁 防止持有锁的时间过长
            // do task

            if (task.type == "add") pool.add(task.user);
            else if (task.type == "remove") pool.remove(task.user);
            pool.match();
        }
    }
}

int main(int argc, char **argv) {
    int port = 9090;
    ::std::shared_ptr<MatchHandler> handler(new MatchHandler());
    ::std::shared_ptr<TProcessor> processor(new MatchProcessor(handler));
    ::std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    ::std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    ::std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);

    cout << "start match server" << endl;

    thread matchingThread(consumeTask);

    server.serve();
    return 0;
}

