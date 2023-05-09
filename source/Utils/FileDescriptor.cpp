/*
** Project FileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue May  9 11:13:37 2023 Francois Michaut
** Last update Wed May 10 00:32:34 2023 Francois Michaut
**
** FileDescriptor.cpp : Helper wrapper class to auto close file descriptor
*/

#include "FileShare/Utils/FileDescriptor.hpp"

#include <iostream>
#include <sstream>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

namespace FileShare::Utils {
    FileHandleBase::FileHandleBase(std::string filename) :
        m_filename(filename)
    {}

    void FileHandleBase::report_error(const char *action, bool raise) const {
        std::ostringstream oss;

        oss << "failed to " << action << " file";
        if (!m_filename.empty())
            oss << " '" << m_filename << '\'';
        oss << ": " << strerror(errno);
        if (raise)
            throw std::runtime_error(oss.str());
        else
            std::cerr << oss.str() << std::endl;
    }

#ifndef OS_WINDOWS
    FileDescriptor::FileDescriptor(int fd, std::string filename)
        : FileHandleBase(std::move(filename)), m_fd(fd)
    {
        if (fd < 0) {
            report_error("open");
        }
    }

    FileDescriptor::FileDescriptor(const char *filename, int flags)
        : FileDescriptor(open(filename, flags), filename)
    {}

    FileDescriptor::FileDescriptor(std::string filename, int flags)
        : FileDescriptor(open(filename.c_str(), flags), filename)
    {}

    FileDescriptor::FileDescriptor(const std::filesystem::path &path, int flags)
        : FileDescriptor(open(path.c_str(), flags), path.string())
    {}

    FileDescriptor::~FileDescriptor()
    {
        if (m_fd != -1 && close(m_fd) == -1) {
            report_error("close", false);
        }
    }
#endif

    FileHandle::FileHandle(FILE *file, std::string filename)
        : FileHandleBase(std::move(filename)), m_file(file)
    {
        if (!file) {
            report_error("open");
        }
    }

    FileHandle::FileHandle(const char *filename, const char *mode)
        : FileHandle(fopen(filename, mode), filename)
    {}

    FileHandle::FileHandle(std::string filename, const char *mode)
        : FileHandle(fopen(filename.c_str(), mode), filename)
    {}

    FileHandle::FileHandle(const std::filesystem::path &path, const char *mode)
        : FileHandle(fopen(path.c_str(), mode), path.string())
    {}

    FileHandle::~FileHandle()
    {
        if (m_file != nullptr && fclose(m_file) != 0) {
            report_error("close", false);
        }
    }
#ifndef OS_WINDOWS
    int FileHandle::fd(bool raise) const {
        if (m_fd == -1) {
            m_fd = fileno(m_file);
            if (m_fd == -1) {
                report_error("fetch file desctriptor from", raise);
            }
        }
        return m_fd;
    }
#endif
}
