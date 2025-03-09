#include "sha256.h"
#include <string>
#include <stdio.h>
#include <openssl/evp.h>


std::string SHA256_File(const fs::path& file)
{
    std::string res = "";
    EVP_MD_CTX* context = nullptr;
    uint8_t buf[4096];
    size_t len = 0;
    uint8_t hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;

#ifdef _WIN32
    FILE* f = _wfopen(file.c_str(), L"rb");
#else
    FILE* f = fopen(file.c_str(), "rb");
#endif
    if (!f)
        goto err_fopen;
    context = EVP_MD_CTX_new();
    if (context == nullptr)
        goto err_alloc;
    if (!EVP_DigestInit_ex(context, EVP_sha256(), nullptr))
        goto err_hash;
    while (!feof(f)) {
        len = fread(buf, 1, sizeof(buf), f);
        if (ferror(f))
            goto err_read;
        if (!EVP_DigestUpdate(context, buf, len))
            goto err_hash;
    }
    if (!EVP_DigestFinal_ex(context, hash, &hashLen))
        goto err_hash;
    for (unsigned i=0; i<hashLen; i++) {
        char hex[] = "0123456789abcdef";
        res += hex[(hash[i]>>4)&0x0f];
        res += hex[hash[i]&0x0f];
    }

err_read:
err_hash:
    EVP_MD_CTX_free(context);
err_alloc:
    fclose(f);
err_fopen:
    return res;
}
