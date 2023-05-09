/*
** Project FileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue May  9 11:03:44 2023 Francois Michaut
** Last update Sun May 14 14:02:59 2023 Francois Michaut
**
** FileDescriptor.hpp : Helper wrapper class to auto close file descriptor
*/

#pragma once

#include <CppSockets/OSDetection.hpp>

#include <string>
#include <filesystem>

namespace FileShare::Utils {
    class FileHandleBase {
        protected:
            FileHandleBase(std::string filename);

            void report_error(const char *action, bool raise = true) const;

            std::string m_filename;
    };

#ifndef OS_WINDOWS
    class FileDescriptor : FileHandleBase {
        public:
            FileDescriptor(int fd, std::string filename = "");
            FileDescriptor(const char *filename, int flags);
            FileDescriptor(const std::filesystem::path &path, int flags);
            FileDescriptor(std::string filename, int flags);
            ~FileDescriptor();

            operator int() const { return m_fd; }

        private:
            int m_fd;
    };
#endif

    class FileHandle : FileHandleBase {
        public:
            FileHandle(FILE *file, std::string filename = "");
            FileHandle(const char *filename, const char *mode);
            FileHandle(const std::filesystem::path &path, const char *mode);
            FileHandle(std::string filename, const char *mode);
            ~FileHandle();

            operator FILE *() const { return m_file; }
#ifndef OS_WINDOWS
            int fd(bool raise = true) const;
#endif

        private:
#ifndef OS_WINDOWS
            mutable int m_fd = -1;
#endif
            FILE *m_file;
    };
}
