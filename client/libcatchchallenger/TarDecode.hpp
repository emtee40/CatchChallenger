/** \file TarDecode.h
\brief To read a tar data block
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef TARDECODE_H
#define TARDECODE_H

#include <vector>
#include <string>
#include <cstdint>
#include "../../general/base/lib.h"

/// \brief read the raw tar data, and organize it into data structure
class DLL_PUBLIC TarDecode
{
    public:
        TarDecode();
        /// \brief to get the file list
        std::vector<std::string> getFileList();
        /// \brief to get the data of the file
        std::vector<std::vector<char> > getDataList();
        /// \brief to pass the raw tar data
        bool decodeData(const std::vector<char> &data);
        /// \brief to return error string
        std::string errorString();
        uint64_t stringtouint64(const std::string &string,bool *ok);
        uint64_t octaltouint64(const std::string &string,bool *ok);
    private:
        std::vector<std::string> fileList;
        std::vector<std::vector<char> > dataList;
        std::string error;
        void setErrorString(const std::string &error);
        bool stringStartWith(std::string const &fullString, std::string const &starting);
};

#endif // TarDecode_H
