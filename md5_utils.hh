#ifndef MD5_UTILS

#include <stdio.h>
#include <iostream>
#include <cstring>
#include <algorithm> 
#include <openssl/evp.h>


// trim blank space on the front of the string
void ftrim(std::string &s) {
    s.erase(s.begin(),
			std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        		return !std::isspace(ch);
    		}));
}

void bytes2md5(const char *data, int len, char *md5buf) {
	EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
	const EVP_MD *md = EVP_md5();
	unsigned char md_value[EVP_MAX_MD_SIZE];
	unsigned int md_len, i;
	EVP_DigestInit_ex(mdctx, md, NULL);
	EVP_DigestUpdate(mdctx, data, len);
	EVP_DigestFinal_ex(mdctx, md_value, &md_len);
	EVP_MD_CTX_free(mdctx);
	for (i = 0; i < md_len; i++) {
		snprintf(&(md5buf[i * 2]), 16 * 2, "%02x", md_value[i]);
	}
}

struct userData
{
	int id;
	std::string hash, email, nickname, password;
	bool isCracked;
};

std::ostream& operator<<(std::ostream& os, const userData& u)
{
    return os << u.id << " " << u.hash << " " << u.email << " " << u.nickname << " " << std::boolalpha << u.isCracked << " " << u.password; 
}

#endif // !MD5_UTILS
