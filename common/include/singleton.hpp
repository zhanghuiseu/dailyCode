/***************************************************************************
*
* Copyright (c) 2020 jackszhang, All Rights Reserved
*
* @file    singleton.hpp
* @author  jackszhang
* @date    2020/10/25
* @brief   The interface of singleton.hpp
*
**************************************************************************/

#include <mutex>

template <class T>
class SingleTon {
 public:
    static T* Instance() {
        if (nullptr == m_instance) {
            std::lock_guard<std::mutex> lock(m_instancMutex);
            if (nullptr == m_instance) {
                m_instance = new T;
            }
        }
        return m_instance;
    }

    void Release() {
        if (nullptr != m_instance) {
            std::lock_guard<std::mutex> lock(m_instancMutex);
            if (nullptr != m_instance) {
                delete m_instance;
                m_instance = nullptr;
            }
        }
    }

 protected:
    SingleTon(void) {};
    ~SingleTon(void) {};

 private:
    static T* m_instance;
    static std::mutex m_instancMutex;
};

template <class T>
T* SingleTon<T>::m_instance = nullptr;

template <class T>
std::mutex SingleTon<T>::m_instancMutex;
