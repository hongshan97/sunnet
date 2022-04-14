#pragma once

#include <list>
#include <stdint.h>
#include <memory>

class WriteObject {
public:
    std::streamsize start;
    std::streamsize len;
    std::shared_ptr<char> buff;
};

class ConnWriter {
public:
    int fd;

    void EntireWrite(std::shared_ptr<char> buff, std::streamsize len);
    void LingerClose(); // 延迟close
    void OnWritable();

private:
    bool isClosing = false; // 是否正在关闭
    std::list<std::shared_ptr<WriteObject>> objs;

    void EntireWriteWhenEmpty(std::shared_ptr<char> buff, std::streamsize len);
    void EntireWriteWhenNotEmpty(std::shared_ptr<char> buff, std::streamsize len);
    bool WriteFrontObj();
};