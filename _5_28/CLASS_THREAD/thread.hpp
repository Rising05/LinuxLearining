#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <pthread.h>
#include <unistd.h>

namespace ThreadModule {
    static int number = 1;

    enum class TSTATUS {
        NEW,
        RUNNING,
        STOPPED,
    };

    template <typename T>
    class Thread {
        using func_t = std::function<void(T)>;

        private:
            static void* Routine(void* arg) {
                Thread<T> *t = static_cast<Thread<T> *>(arg);
                t->_status = TSTATUS::RUNNING;
                t->_func(t->_data);
                return nullptr;
            }

        public:
            Thread(func_t func, T data) 
            : _func(func), 
              _data(data), 
              _status(TSTATUS::NEW),
              _joinable(true) {
                _name = "Thread-" + std::to_string(number++);
                _pid = getpid();
            }

            bool Start() {
                if (_status != TSTATUS::RUNNING) {
                    int n = pthread_create(&_tid, nullptr, Routine, this);
                    if (n != 0) {
                        return false;
                    }
                    pthread_setname_np(_tid, _name.c_str());
                    return true;
                }
                return false;
            }

            bool Stop() {
                if (_status == TSTATUS::RUNNING) {
                    int n = pthread_cancel(_tid);
                    if (n != 0) {
                        return false;
                    }
                    _status = TSTATUS::STOPPED;
                    return true;
                }
                return false;
            }

            bool Join() {
                if (_joinable) {
                    int n = pthread_join(_tid, nullptr);
                    if (n != 0) {
                        return false;
                    }
                    return true;
                }
                return false;
            }

            bool IsJoinable() const {
                return _joinable;
            }

            std::string GetName() const {
                return _name;
            }

            ~Thread() {}

        private:
            pthread_t _tid;
            pid_t _pid;
            std::string _name;
            func_t _func;
            T _data;
            TSTATUS _status;
            bool _joinable;
    };

    // void specialization — no data, func takes no arguments
    template <>
    class Thread<void> {
        using func_t = std::function<void()>;

        private:
            static void* Routine(void* arg) {
                Thread<void> *t = static_cast<Thread<void> *>(arg);
                t->_status = TSTATUS::RUNNING;
                t->_func();
                return nullptr;
            }

        public:
            Thread(func_t func)
            : _func(func),
              _status(TSTATUS::NEW),
              _joinable(true) {
                _name = "Thread-" + std::to_string(number++);
                _pid = getpid();
            }

            bool Start() {
                if (_status != TSTATUS::RUNNING) {
                    int n = pthread_create(&_tid, nullptr, Routine, this);
                    if (n != 0) {
                        return false;
                    }
                    pthread_setname_np(_tid, _name.c_str());
                    return true;
                }
                return false;
            }

            bool Stop() {
                if (_status == TSTATUS::RUNNING) {
                    int n = pthread_cancel(_tid);
                    if (n != 0) {
                        return false;
                    }
                    _status = TSTATUS::STOPPED;
                    return true;
                }
                return false;
            }

            bool Join() {
                if (_joinable) {
                    int n = pthread_join(_tid, nullptr);
                    if (n != 0) {
                        return false;
                    }
                    return true;
                }
                return false;
            }

            bool IsJoinable() const {
                return _joinable;
            }

            std::string GetName() const {
                return _name;
            }

            ~Thread() {}

        private:
            pthread_t _tid;
            pid_t _pid;
            std::string _name;
            func_t _func;
            TSTATUS _status;
            bool _joinable;
    };

}  // namespace ThreadModule
