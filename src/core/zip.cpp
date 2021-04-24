#include "zip.h"

// as long as we only use miniz here, we can just #include it
extern "C" {
#include <miniz.c>
}

#include <algorithm>
#include "string.h"


Zip::Zip(const std::string& filename)
{
    _slashes = Slashes::UNKNOWN;
    mz_zip_zero_struct(&_zip);
    if (!mz_zip_reader_init_file(&_zip, filename.c_str(), 0)) {
        _valid = false;
        return;
    }
    _valid = true;
}

Zip::~Zip()
{
    if (_valid) {
        mz_zip_reader_end(&_zip);
        mz_zip_zero_struct(&_zip);
    }
    _valid = false;
}

void Zip::setDir(const std::string& dir)
{
    // remove leading /
    if (dir[0] == '/') _dir = dir.substr(1);
    else _dir = dir;
    
    // remove trailing \ (copied from Zip::list of bad archive)
    if (_dir.length()>0 && _dir.back() == '\\')
        _dir.pop_back();
    
    // add trailing /
    if (_dir.length()>0 && _dir.back() != '/')
        _dir += '/';
    
    while (true) {
        // remove trailing ./ (but not ../)
        if (_dir.length()>1 && (_dir.length()<3 || _dir[_dir.length()-3] == '/') && _dir.substr(_dir.length()-2) == "./")
            _dir = _dir.substr(0, _dir.length()-2);
        else
            break;
    }
}

std::vector< std::pair<Zip::EntryType,std::string> > Zip::list(bool recursive)
{
    std::vector< std::pair<EntryType,std::string> > res;
    if (!_valid) return res;
    
    std::string bsDir = _dir;
    std::replace(bsDir.begin(), bsDir.end(), '/', '\\'); // work around bad zip
    
    for (unsigned i=0; i<mz_zip_reader_get_num_files(&_zip); i++)
    {
        mz_zip_archive_file_stat st;
        if (mz_zip_reader_file_stat(&_zip, i, &st)) {
            if (!_dir.empty() && memcmp(st.m_filename, _dir.c_str(), _dir.length())!=0
                    && memcmp(st.m_filename, bsDir.c_str(), bsDir.length())!=0)
                continue; // directory does not match setDir()
            std::string name = st.m_filename + _dir.length();
            if (name.empty()) continue;
            auto pSlash = name.find('/');
            auto pBS = name.find('\\'); // work around bad zip
            if (pSlash != name.npos) _slashes |= Slashes::FORWARD;
            if (pBS    != name.npos) _slashes |= Slashes::BACKWARD;
            if (!recursive && (
                    (pSlash != name.npos && pSlash != name.length()-1) ||
                    (pBS != name.npos && pBS != name.length()-1)))
                continue;
            res.push_back( {
                mz_zip_reader_is_file_a_directory(&_zip, i) ? EntryType::DIR : EntryType::FILE,
                name
            } );
        }
    }
    
    return res;
}

bool Zip::readFile(const std::string& name, std::string& out)
{
    out.clear();
    if (!_valid) return false;
    
    std::string path = _dir + name;
    if (_slashes == Slashes::FORWARD) {
        std::replace(path.begin(), path.end(), '\\', '/');
    } else if (_slashes == Slashes::BACKWARD) {
        std::replace(path.begin(), path.end(), '/', '\\');
    }
    
    mz_uint32 index = 0;
    if (mz_zip_reader_locate_file_v2(&_zip, path.c_str(), nullptr, 0, &index)<1) {
        return false; // no such file
    }
    
    mz_zip_archive_file_stat st;
    if (!mz_zip_reader_file_stat(&_zip, index, &st)) {
        return false;
    }
    
    size_t sz = (size_t)st.m_uncomp_size;
    if (sz == 0) return true; // done
    
    out.resize(sz);
    if (mz_zip_reader_extract_to_mem(&_zip, index, out.data(), sz, 0)) {
        return true;
    } else {
        out.clear();
        return false;
    }
}